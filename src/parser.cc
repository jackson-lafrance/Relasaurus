#include "parser.h"
#include "lexer.h"
#include "schema.h"
#include <cctype>

std::variant<Program, ParseError>
Parser::parse_tokens(std::vector<Token> &tokens) {
  Parser parser(tokens);

  try {
    Program program = parser.parse_program();

    return std::move(program);
  } catch (ParseError error) {
    return std::move(error);
  }
}

Parser::Parser(std::vector<Token> &tokens) : tokens_(tokens) {}

const Token &Parser::peek() const { return tokens_[current_]; }
const Token &Parser::prev() const { return tokens_[current_ - 1]; }
const Token &Parser::advance() {
  const Token &token = peek();

  if (!at_end())
    ++current_;

  return token;
}

bool Parser::at_end() const { return peek().type == TType::EndOfInput; }
bool Parser::check(TType type) const { return peek().type == type; }
bool Parser::match(TType type) {
  if (!check(type)) {
    return false;
  }

  advance();
  return true;
}

const Token &Parser::consume(TType type, std::string error) {
  if (check(type))
    return advance();

  fail(peek(), type, std::move(error));
}

Program Parser::parse_program() {
  Program program;

  while (!at_end()) {
    program.statements.push_back(parse_statement());
  }

  return program;
}

Statement Parser::parse_statement() {
  if (check(TType::Relation)) {
    return parse_relation_definition();
  }

  if (check(TType::Insert)) {
    return parse_relation_insertion();
  }

  return parse_query_statement();
}

Statement Parser::parse_relation_definition() {
  const Token &start =
      consume(TType::Relation, "I swear that was a 'relation'...");
  const Token &name = consume(TType::Identifier, "");

  consume(TType::LeftParen, "Expected '(' to open relation schema!!");
  std::vector<ColumnWithSpan> columns = parse_schema();
  consume(TType::RightParen, "Expected ')' to close relation schema!!");

  consume(TType::LeftBrace, "Expected '{' to open tuples definition!!");
  std::vector<TupleWithSpan> tuples;
  while (!check(TType::RightBrace)) {
    if (at_end()) {
      fail(peek(), TType::RightBrace, "Expected '}' after relation tuples!!");
    }
    tuples.push_back(parse_tuple());
  }
  consume(TType::RightBrace, "Expected '}' to close tuples definition!!");

  const Token &semicolon =
      consume(TType::Semicolon, "Expected ';' after relation definition!!");

  return Statement{
      .guy =
          RelationDefinition{
              .name = {.name = name.lexeme, .span = name.span},
              .columns = std::move(columns),
              .tuples = std::move(tuples),
          },
      .span = {.begin = start.span.begin, .end = semicolon.span.end}};
};

Statement Parser::parse_relation_insertion() {
  const Token &start =
      consume(TType::Insert, "Hey what'd you do with my insert statement??");
  const Token &name =
      consume(TType::Identifier, "Expected relation name after insert!!");

  consume(TType::LeftBrace, "Expected '{' after relation name!!");
  std::vector<TupleWithSpan> tuples;
  while (!check(TType::RightBrace)) {
    if (at_end()) {
      fail(peek(), TType::RightBrace, "Expected '}' after relation tuples!!");
    }
    tuples.push_back(parse_tuple());
  }
  consume(TType::RightBrace, "Expected '}' to close tuples definition!!");

  const Token &semicolon =
      consume(TType::Semicolon, "Expected ';' after relation definition!!");

  return Statement{
      .guy =
          RelationInsertion{
              .name = {.name = name.lexeme, .span = name.span},
              .tuples = std::move(tuples),
          },
      .span = {.begin = start.span.begin, .end = semicolon.span.end}};
};

Statement Parser::parse_query_statement() {
  const Position start = peek().span.begin;

  REX query = parse_rex();

  const Token &semicolon =
      consume(TType::Semicolon, "Expected ';' after query!!");

  return Statement{.guy = std::move(query),
                   .span = {.begin = start, .end = semicolon.span.end}};
}

REX Parser::parse_rex() {
  REX left = parse_primary();

  while (true) {
    if (match(TType::At)) {
      consume(TType::LeftBrace, "Expected '{' after '@'!!");
      Condition condition = parse_condition();
      consume(TType::RightBrace, "Expected '}' after join condition!!");

      REX right = parse_primary();

      const Position begin = left.span.begin;
      const Position end = right.span.end;

      left = REX{
          .node =
              JoinExpression{.condition = std::move(condition),
                             .left = std::make_unique<REX>(std::move(left)),
                             .right = std::make_unique<REX>(std::move(right))},
          .span = {.begin = begin, .end = end}};

      continue;
    };

    BinaryOperator operation;

    if (match(TType::Plus)) {
      operation = BinaryOperator::UNION;
    } else if (match(TType::Ampersand)) {
      operation = BinaryOperator::INTERSECT;
    } else if (match(TType::Minus)) {
      operation = BinaryOperator::MINUS;
    } else if (match(TType::Star)) {
      operation = BinaryOperator::TIMES;
    } else {
      break;
    }

    REX right = parse_primary();

    const Position begin = left.span.begin;
    const Position end = right.span.end;

    left = REX{
        .node =
            BinaryExpression{.operation = operation,
                             .left = std::make_unique<REX>(std::move(left)),
                             .right = std::make_unique<REX>(std::move(right))},
        .span = {.begin = begin, .end = end}};
  }

  return left;
}

REX Parser::parse_primary() {
  if (check(TType::Identifier)) {
    const Token &name = advance();

    return REX{.node = NameWithSpan{.name = name.lexeme, .span = name.span},
               .span = name.span};
  }

  if (check(TType::LeftParen)) {
    const Position begin = advance().span.begin;

    REX expression = parse_rex();

    const Position end =
        consume(TType::RightParen, "Expected ')' after expression!!").span.end;

    expression.span = {.begin = begin, .end = end};

    return expression;
  }

  if (check(TType::Select)) {
    return parse_select();
  }

  if (check(TType::Project)) {
    return parse_project();
  }

  if (check(TType::RenameTable)) {
    return parse_rename_table();
  }

  if (check(TType::RenameAttribute)) {
    return parse_rename_attribute();
  }

  fail(peek(), "Expected a relation or relational expression!!");
};

REX parse_select();
REX parse_project();
REX parse_rename_table();
REX parse_rename_attribute();

Condition parse_condition();
Condition parse_or();
Condition parse_and();
Condition parse_not();
Condition parse_comparison();

Operand parse_operand();
AttributeReference parse_attribute_reference();

std::vector<ColumnWithSpan> Parser::parse_schema() {
  std::vector<ColumnWithSpan> columns;
  columns.push_back(parse_column());

  while (match(TType::Comma)) {
    if (check(TType::RightParen))
      break;

    columns.push_back(parse_column());
  }

  return columns;
}

ColumnWithSpan Parser::parse_column() {
  const Token &name =
      consume(TType::Identifier, "Expected an attribute name!!");

  consume(TType::Equal, "Expected '=' after attribute name!!");

  Type type;
  if (match(TType::StringType)) {
    type = STRING;
  } else if (match(TType::NumberType)) {
    type = NUMBER;
  } else {
    fail(peek(), "Expected 'string' or 'number' after '='!!");
  }

  const Token &type_token = prev();

  return ColumnWithSpan{
      .column = {.name = name.lexeme, .type = type},
      .span = {.begin = name.span.begin, .end = type_token.span.end},
  };
}

TupleWithSpan Parser::parse_tuple() {
  Position position = peek().span.begin;
  Tuple values;
  values.push_back(parse_value());

  while (match(TType::Comma))
    values.push_back(parse_value());

  const Token &semicolon =
      consume(TType::Semicolon, "Expected ';' after tuple!!");

  return TupleWithSpan{.tuple = values,
                       .span = {.begin = position, .end = semicolon.span.end}};
}

Value Parser::parse_value() {
  if (match(TType::Number))
    return std::get<double>(prev().value);

  if (match(TType::String))
    return std::get<std::string>(prev().value);

  if (match(TType::Identifier))
    return prev().lexeme;

  fail(peek(), "Expected a tuple value.");
}

[[noreturn]]
void Parser::fail(const Token &token, std::string message) {
  throw ParseError{
      .message = std::move(message),
      .span = token.span,
      .actual = token.type,
      .expected = std::nullopt,
  };
}

[[noreturn]]
void Parser::fail(const Token &token, TType expected, std::string message) {
  throw ParseError{
      .message = std::move(message),
      .span = token.span,
      .actual = token.type,
      .expected = expected,
  };
}
