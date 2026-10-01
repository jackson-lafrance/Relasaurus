#include "lexer.h"
#include "parser.h"
#include "test_support.h"
#include <cstddef>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace {

using test_support::Suite;
using test_support::expect;

Statement parse_success(std::string_view source) {
  Result lexed = Lexer::tokenize(std::string(source));
  expect(!lexed.error.has_value(), "expected tokenization to succeed");

  std::variant<Statement, ParseError> parsed =
      Parser::parse_tokens(lexed.tokens);
  expect(std::holds_alternative<Statement>(parsed),
         "expected parsing to succeed");
  return std::get<Statement>(std::move(parsed));
}

ParseError parse_failure(std::string_view source) {
  Result lexed = Lexer::tokenize(std::string(source));
  expect(!lexed.error.has_value(), "expected tokenization to succeed");

  std::variant<Statement, ParseError> parsed =
      Parser::parse_tokens(lexed.tokens);
  expect(std::holds_alternative<ParseError>(parsed),
         "expected parsing to fail");
  return std::get<ParseError>(std::move(parsed));
}

template <typename Node> const Node &expect_node(const auto &variant) {
  const Node *node = std::get_if<Node>(&variant);
  expect(node != nullptr, "AST node has the wrong type");
  return *node;
}

const REX &expect_query(const Statement &statement) {
  return expect_node<REX>(statement.guy);
}

const NameWithSpan &expect_name(const REX &expression,
                                std::string_view expected_name) {
  const NameWithSpan &name = expect_node<NameWithSpan>(expression.node);
  expect(name.name == expected_name, "relation reference has the wrong name");
  return name;
}

const ComparisonCondition &expect_comparison(const Condition &condition) {
  return expect_node<ComparisonCondition>(condition.node);
}

void expect_position(const Position &position, std::size_t offset,
                     std::size_t row, std::size_t col) {
  expect(position.offset == offset, "position has the wrong offset");
  expect(position.row == row, "position has the wrong row");
  expect(position.col == col, "position has the wrong column");
}

} // namespace

int main() {
  Suite suite("parser tests");

  suite.run("empty input is incomplete", [] {
    const ParseError error = parse_failure("");
    expect(error.actual == TType::EndOfInput,
           "empty input should fail at end-of-input");
  });

  suite.run("relation definitions preserve schema tuples and spans", [] {
    const Statement statement = parse_success(
        "relation Dinosaurs (ID=number,Name=string,) "
        "{1,Rex;2,'O''Brien';};");

    const RelationDefinition &definition =
        expect_node<RelationDefinition>(statement.guy);

    expect(definition.name.name == "Dinosaurs",
           "definition should preserve the relation name");
    expect(definition.columns.size() == 2,
           "definition should contain both columns");
    expect(definition.columns[0].column.name == "ID",
           "first column should preserve its name");
    expect(definition.columns[0].column.type == NUMBER,
           "first column should be numeric");
    expect(definition.columns[1].column.name == "Name",
           "second column should preserve its name");
    expect(definition.columns[1].column.type == STRING,
           "second column should be a string");

    expect(definition.tuples.size() == 2,
           "definition should contain both tuples");
    expect(std::get<double>(definition.tuples[0].tuple[0]) == 1.0,
           "numeric tuple values should remain numbers");
    expect(std::get<std::string>(definition.tuples[0].tuple[1]) == "Rex",
           "bare identifiers should become tuple strings");
    expect(std::get<std::string>(definition.tuples[1].tuple[1]) == "O'Brien",
           "quoted tuple strings should be decoded");

    expect_position(statement.span.begin, 0, 1, 1);
    expect_position(statement.span.end, 66, 1, 67);
    expect_position(definition.name.span.begin, 9, 1, 10);
    expect_position(definition.name.span.end, 18, 1, 19);
  });

  suite.run("insertions allow populated and empty tuple lists", [] {
    const Statement populated_statement =
        parse_success("insert Dinosaurs {3,Blue;};");
    const Statement empty_statement =
        parse_success("insert Dinosaurs {};");

    const RelationInsertion &populated =
        expect_node<RelationInsertion>(populated_statement.guy);
    const RelationInsertion &empty =
        expect_node<RelationInsertion>(empty_statement.guy);

    expect(populated.name.name == "Dinosaurs",
           "insertion should preserve its relation name");
    expect(populated.tuples.size() == 1,
           "populated insertion should contain one tuple");
    expect(std::get<double>(populated.tuples[0].tuple[0]) == 3.0,
           "insertion should preserve numeric values");
    expect(std::get<std::string>(populated.tuples[0].tuple[1]) == "Blue",
           "insertion should turn a bare identifier into a string");
    expect(empty.tuples.empty(), "empty insertion should contain no tuples");
  });

  suite.run("binary expressions are left associative at one precedence", [] {
    const Statement statement = parse_success("A+B-C*D&E;");
    const REX &root = expect_query(statement);
    const BinaryExpression &intersection =
        expect_node<BinaryExpression>(root.node);
    expect(intersection.operation == BinaryOperator::INTERSECT,
           "the last operator should be the root");
    expect_name(*intersection.right, "E");

    const BinaryExpression &times =
        expect_node<BinaryExpression>(intersection.left->node);
    expect(times.operation == BinaryOperator::TIMES,
           "times should not bind more tightly than minus");
    expect_name(*times.right, "D");

    const BinaryExpression &minus =
        expect_node<BinaryExpression>(times.left->node);
    expect(minus.operation == BinaryOperator::MINUS,
           "minus should follow the preceding union");
    expect_name(*minus.right, "C");

    const BinaryExpression &unioned =
        expect_node<BinaryExpression>(minus.left->node);
    expect(unioned.operation == BinaryOperator::UNION,
           "plus should produce a union expression");
    expect_name(*unioned.left, "A");
    expect_name(*unioned.right, "B");
  });

  suite.run("parentheses override relational expression associativity", [] {
    const Statement statement = parse_success("A+(B-C);");
    const BinaryExpression &unioned =
        expect_node<BinaryExpression>(expect_query(statement).node);

    expect(unioned.operation == BinaryOperator::UNION,
           "outer expression should be a union");
    expect_name(*unioned.left, "A");
    const BinaryExpression &minus =
        expect_node<BinaryExpression>(unioned.right->node);
    expect(minus.operation == BinaryOperator::MINUS,
           "parenthesized expression should remain on the right");
    expect_name(*minus.left, "B");
    expect_name(*minus.right, "C");
  });

  suite.run("join conditions honor boolean precedence and qualification", [] {
    const Statement statement = parse_success(
        "A@{A.ID=B.ID&&A.Score>=90||!A.Active='no'}B;");
    const JoinExpression &join =
        expect_node<JoinExpression>(expect_query(statement).node);

    expect_name(*join.left, "A");
    expect_name(*join.right, "B");
    const LogicalCondition &or_condition =
        expect_node<LogicalCondition>(join.condition.node);
    expect(or_condition.king_von == LogicalOperator::Or,
           "OR should be the condition root");

    const LogicalCondition &and_condition =
        expect_node<LogicalCondition>(or_condition.left->node);
    expect(and_condition.king_von == LogicalOperator::And,
           "AND should bind more tightly than OR");
    const ComparisonCondition &ids = expect_comparison(*and_condition.left);
    expect(ids.lil_durk == ComparisonOperator::Equal,
           "ID comparison should use equality");
    const AttributeReference &left_id =
        expect_node<AttributeReference>(ids.left.value);
    expect(left_id.relation_name.has_value() &&
               left_id.relation_name->name == "A" &&
               left_id.attribute_name.name == "ID",
           "left join attribute should remain qualified");

    const ComparisonCondition &score = expect_comparison(*and_condition.right);
    expect(score.lil_durk == ComparisonOperator::GreaterEqual,
           "score comparison should preserve >=");
    expect(std::get<double>(score.right.value) == 90.0,
           "score comparison should preserve its number");

    const NotCondition &negated =
        expect_node<NotCondition>(or_condition.right->node);
    const ComparisonCondition &active = expect_comparison(*negated.operand);
    expect(std::get<std::string>(active.right.value) == "no",
           "condition strings should preserve their decoded value");
  });

  suite.run("unary expressions preserve their arguments", [] {
    const Statement project_statement = parse_success(
        "project{People.Name,Age}(select{Age>=18}(People));");
    const Statement table_rename_statement =
        parse_success("renameTable{Adults}(People);");
    const Statement attribute_rename_statement =
        parse_success("renameAttribute{Score,People.Grade}(People);");

    const ProjectExpression &project = expect_node<ProjectExpression>(
        expect_query(project_statement).node);
    expect(project.attributes.size() == 2,
           "project should preserve both attributes");
    expect(project.attributes[0].relation_name.has_value() &&
               project.attributes[0].relation_name->name == "People" &&
               project.attributes[0].attribute_name.name == "Name",
           "project should preserve a qualified attribute");
    expect(!project.attributes[1].relation_name.has_value() &&
               project.attributes[1].attribute_name.name == "Age",
           "project should preserve an unqualified attribute");

    const SelectExpression &select =
        expect_node<SelectExpression>(project.input->node);
    expect_comparison(select.condition);
    expect_name(*select.input, "People");

    const RenameTableExpression &rename_table =
        expect_node<RenameTableExpression>(
            expect_query(table_rename_statement).node);
    expect(rename_table.new_name.name == "Adults",
           "table rename should preserve its new name");
    expect_name(*rename_table.input, "People");

    const RenameAttributeExpression &rename_attribute =
        expect_node<RenameAttributeExpression>(
            expect_query(attribute_rename_statement).node);
    expect(rename_attribute.new_name.name == "Score",
           "attribute rename should preserve its new name");
    expect(rename_attribute.old_name.relation_name.has_value() &&
               rename_attribute.old_name.relation_name->name == "People" &&
               rename_attribute.old_name.attribute_name.name == "Grade",
           "attribute rename should preserve its qualified old name");
    expect_name(*rename_attribute.input, "People");
  });

  suite.run("all comparison operators map to the expected AST values", [] {
    const std::vector<std::string_view> sources{
        "select{A=1}(R);",  "select{A!=1}(R);", "select{A<1}(R);",
        "select{A<=1}(R);", "select{A>1}(R);",  "select{A>=1}(R);",
    };
    const std::vector<ComparisonOperator> expected{
        ComparisonOperator::Equal,        ComparisonOperator::NotEqual,
        ComparisonOperator::Less,         ComparisonOperator::LessEqual,
        ComparisonOperator::Greater,      ComparisonOperator::GreaterEqual,
    };

    for (std::size_t i = 0; i < expected.size(); ++i) {
      const Statement statement = parse_success(sources[i]);
      const SelectExpression &select =
          expect_node<SelectExpression>(expect_query(statement).node);
      expect(expect_comparison(select.condition).lil_durk == expected[i],
             "comparison operator has the wrong AST value");
    }
  });

  suite.run("multiple statements are rejected", [] {
    const ParseError error = parse_failure("A;B;");
    expect(error.actual == TType::Identifier,
           "the second statement should be unexpected");
    expect(error.expected.has_value() &&
               *error.expected == TType::EndOfInput,
           "the parser should expect input to end after one statement");
  });

  suite.run("missing statement semicolons report expected token metadata", [] {
    const ParseError error = parse_failure("A");

    expect(error.actual == TType::EndOfInput,
           "error should point at end-of-input");
    expect(error.expected.has_value() &&
               *error.expected == TType::Semicolon,
           "error should expect a semicolon");
    expect_position(error.span.begin, 1, 1, 2);
  });

  suite.run("malformed schemas and expressions are rejected", [] {
    const ParseError schema_error =
        parse_failure("relation R (Name=) {};");
    expect(schema_error.actual == TType::RightParen,
           "schema error should point at the unexpected right parenthesis");
    expect(!schema_error.expected.has_value(),
           "schema type error should not claim one specific token");

    const ParseError project_error = parse_failure("project{}(R);");
    expect(project_error.actual == TType::RightBrace,
           "empty projection should fail at its closing brace");
    expect(project_error.expected.has_value() &&
               *project_error.expected == TType::Identifier,
           "empty projection should expect an attribute");

    const ParseError relation_error =
        parse_failure("relation R (Name=string) {Alice;");
    expect(relation_error.actual == TType::EndOfInput,
           "unterminated relation should fail at end-of-input");
    expect(relation_error.expected.has_value() &&
               *relation_error.expected == TType::RightBrace,
           "unterminated relation should expect a closing brace");
  });

  return suite.finish();
}
