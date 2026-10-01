#include "interpreter.h"
#include "lexer.h"
#include "parser.h"
#include "test_support.h"
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace {

using test_support::Suite;
using test_support::expect;
using test_support::expect_throws;

Statement parse(std::string_view source) {
  Result lexed = Lexer::tokenize(std::string(source));
  expect(!lexed.error.has_value(), "test source should tokenize");

  std::variant<Statement, ParseError> parsed =
      Parser::parse_tokens(lexed.tokens);
  expect(std::holds_alternative<Statement>(parsed),
         "test source should parse");
  return std::get<Statement>(std::move(parsed));
}

std::optional<Relation> execute(Interpreter &interpreter,
                                std::string_view source) {
  return interpreter.execute(parse(source));
}

Relation execute_query(Interpreter &interpreter, std::string_view source) {
  std::optional<Relation> result = execute(interpreter, source);
  expect(result.has_value(), "expected a query result");
  return std::move(*result);
}

bool has_tuple(const Relation &relation, const Tuple &tuple) {
  return relation.tuples().contains(tuple);
}

} // namespace

int main() {
  Suite suite("interpreter tests");

  suite.run("definitions insertions and relation queries share state", [] {
    Interpreter interpreter;
    const std::optional<Relation> definition_result = execute(
        interpreter,
        "relation People(Name=string,Age=number){Ann,20;Bob,17;Ann,20;};");
    const std::optional<Relation> insertion_result =
        execute(interpreter, "insert People {Cara,30;Bob,17;};");

    expect(!definition_result.has_value() && !insertion_result.has_value(),
           "definitions and insertions should not return relations");
    const Relation people = execute_query(interpreter, "People;");
    expect(people.name() == "People", "query should preserve relation name");
    expect(people.schema().columns().size() == 2,
           "query should preserve the schema");
    expect(people.tuples().size() == 3,
           "duplicate definition and insertion tuples should collapse");
    expect(has_tuple(people, Tuple{std::string{"Cara"}, 30.0}),
           "inserted tuple should be available to later queries");
  });

  suite.run("invalid definitions insertions and references are rejected", [] {
    Interpreter interpreter;
    execute(interpreter, "relation R(ID=number){1;};");

    expect_throws<std::runtime_error>(
        [&] { execute(interpreter, "relation R(ID=number){};"); },
        "duplicate relation definitions should fail");
    expect_throws<std::runtime_error>(
        [&] { execute(interpreter, "insert Missing {1;};"); },
        "insertion into an unknown relation should fail");
    expect_throws<std::runtime_error>(
        [&] { execute(interpreter, "Missing;"); },
        "querying an unknown relation should fail");
    expect_throws<std::runtime_error>(
        [&] { execute(interpreter, "insert R {wrong;};"); },
        "inserting a value of the wrong type should fail");
  });

  suite.run("failed multi-tuple insertions do not partially update state", [] {
    Interpreter interpreter;
    execute(interpreter, "relation R(ID=number){1;};");

    expect_throws<std::runtime_error>(
        [&] { execute(interpreter, "insert R {2;wrong;};"); },
        "invalid insertion should fail");
    const Relation relation = execute_query(interpreter, "R;");
    expect(relation.tuples().size() == 1,
           "failed insertion should leave the stored relation unchanged");
    expect(has_tuple(relation, Tuple{1.0}),
           "failed insertion should preserve the original tuple");
  });

  suite.run("selection evaluates comparisons strings and negation", [] {
    Interpreter interpreter;
    execute(interpreter,
            "relation People(Name=string,Age=number)"
            "{Ann,20;Bob,17;Cara,30;};");

    const Relation adults = execute_query(
        interpreter, "select{Age>=18}(People);");
    expect(adults.tuples().size() == 2,
           "numeric comparison should retain both adults");

    const Relation not_ann = execute_query(
        interpreter, "select{!(Name='Ann')}(People);");
    expect(not_ann.tuples().size() == 2,
           "negated string comparison should exclude Ann");
    expect(!has_tuple(not_ann, Tuple{std::string{"Ann"}, 20.0}),
           "negated result should not contain Ann");
  });

  suite.run("logical AND evaluates both operands", [] {
    Interpreter interpreter;
    execute(interpreter,
            "relation People(Name=string,Age=number,Active=number)"
            "{Ann,20,1;Bob,17,1;Cara,20,0;};");

    const Relation result = execute_query(
        interpreter, "select{Age>=18&&Active=1}(People);");
    expect(result.tuples().size() == 1,
           "AND should require its right operand to be true");
    expect(has_tuple(result, Tuple{std::string{"Ann"}, 20.0, 1.0}),
           "Ann should satisfy both conditions");
  });

  suite.run("logical OR evaluates its right operand", [] {
    Interpreter interpreter;
    execute(interpreter,
            "relation People(Name=string,Age=number,Active=number)"
            "{Ann,20,1;Bob,17,1;Cara,20,0;};");

    const Relation result = execute_query(
        interpreter, "select{Age<18||Active=0}(People);");
    expect(result.tuples().size() == 2,
           "OR should retain tuples matching either operand");
    expect(has_tuple(result, Tuple{std::string{"Bob"}, 17.0, 1.0}),
           "left side of OR should retain Bob");
    expect(has_tuple(result, Tuple{std::string{"Cara"}, 20.0, 0.0}),
           "right side of OR should retain Cara");
  });

  suite.run("projection and renames evaluate nested expressions", [] {
    Interpreter interpreter;
    execute(interpreter,
            "relation Students(Name=string,Grade=number)"
            "{Ann,90;Bob,80;};");

    const Relation projected = execute_query(
        interpreter,
        "project{Score}(renameAttribute{Score,Grade}(Students));");
    expect(projected.schema().columns().size() == 1,
           "projection should return one column");
    expect(projected.schema().columns()[0].name == "Score",
           "attribute rename should feed its new name into projection");
    expect(projected.tuples().size() == 2,
           "nested operations should preserve matching tuples");

    const Relation renamed = execute_query(
        interpreter, "renameTable{Pupils}(Students);");
    expect(renamed.name() == "Pupils",
           "table rename should change the result name");
  });

  suite.run("set operations dispatch to relational algebra", [] {
    Interpreter interpreter;
    execute(interpreter, "relation A(ID=number){1;2;};");
    execute(interpreter, "relation B(ID=number){2;3;};");

    const Relation unioned = execute_query(interpreter, "A+B;");
    const Relation intersected = execute_query(interpreter, "A&B;");
    const Relation difference = execute_query(interpreter, "A-B;");

    expect(unioned.tuples().size() == 3,
           "union should contain every distinct tuple");
    expect(intersected.tuples().size() == 1 &&
               has_tuple(intersected, Tuple{2.0}),
           "intersection should contain the shared tuple");
    expect(difference.tuples().size() == 1 &&
               has_tuple(difference, Tuple{1.0}),
           "minus should contain only the left-only tuple");
  });

  suite.run("times and join resolve qualified attributes", [] {
    Interpreter interpreter;
    execute(interpreter,
            "relation A(ID=number,Name=string){1,Ann;2,Bob;};");
    execute(interpreter,
            "relation B(ID=number,Colour=string){2,Blue;3,Red;};");

    const Relation product = execute_query(interpreter, "A*B;");
    expect(product.tuples().size() == 4,
           "times should return the Cartesian product");
    expect(product.schema().index_of("A.ID") == 0 &&
               product.schema().index_of("B.ID") == 2,
           "times should qualify columns from both relations");

    const Relation joined = execute_query(
        interpreter, "A@{A.ID=B.ID}B;");
    expect(joined.tuples().size() == 1,
           "join should retain the matching ID pair");
    expect(has_tuple(joined,
                     Tuple{2.0, std::string{"Bob"}, 2.0,
                           std::string{"Blue"}}),
           "join should preserve values from both matching tuples");
  });

  suite.run("unknown and incorrectly qualified attributes are rejected", [] {
    Interpreter interpreter;
    execute(interpreter,
            "relation People(Name=string,Age=number){Ann,20;};");

    expect_throws<std::runtime_error>(
        [&] { execute(interpreter, "select{Missing=1}(People);"); },
        "selection should reject an unknown attribute");
    expect_throws<std::runtime_error>(
        [&] { execute(interpreter, "project{Other.Name}(People);"); },
        "projection should reject the wrong relation qualifier");
    expect_throws<std::runtime_error>(
        [&] {
          execute(interpreter,
                  "renameAttribute{Years,Other.Age}(People);");
        },
        "attribute rename should reject the wrong relation qualifier");
  });

  return suite.finish();
}
