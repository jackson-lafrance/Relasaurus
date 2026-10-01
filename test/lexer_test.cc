#include "lexer.h"
#include "test_support.h"
#include <cmath>
#include <cstddef>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace {

using test_support::Suite;
using test_support::expect;

struct ExpectedToken {
  TType type;
  std::string_view lexeme;
};

const Token &expect_token(const Result &result, std::size_t index,
                          TType expected_type,
                          std::string_view expected_lexeme) {
  expect(!result.error.has_value(), "expected tokenization to succeed");
  expect(index < result.tokens.size(), "expected another token");

  const Token &token = result.tokens[index];
  expect(token.type == expected_type, "token has the wrong type");
  expect(token.lexeme == expected_lexeme, "token has the wrong lexeme");
  return token;
}

void expect_token_count(const Result &result, std::size_t expected_count) {
  expect(!result.error.has_value(), "expected tokenization to succeed");
  expect(result.tokens.size() == expected_count,
         "token stream has the wrong number of tokens");
}

void expect_error(const Result &result, EType expected_type) {
  expect(result.error.has_value(), "expected tokenization to fail");
  expect(result.error->type == expected_type, "lexer returned the wrong error");
}

void expect_number_value(const Token &token, double expected_value) {
  const double *value = std::get_if<double>(&token.value);
  expect(value != nullptr, "number token is missing its numeric value");
  expect(std::abs(*value - expected_value) < 0.000000001,
         "number token has the wrong numeric value");
}

void expect_string_value(const Token &token,
                         std::string_view expected_value) {
  const std::string *value = std::get_if<std::string>(&token.value);
  expect(value != nullptr, "token is missing its string value");
  expect(*value == expected_value, "token has the wrong string value");
}

void expect_position(const Position &position, std::size_t offset,
                     std::size_t row, std::size_t col) {
  expect(position.offset == offset, "position has the wrong offset");
  expect(position.row == row, "position has the wrong row");
  expect(position.col == col, "position has the wrong column");
}

} // namespace

int main() {
  Suite suite("lexer tests");

  suite.run("empty input emits only end-of-input", [] {
    const Result result = Lexer::tokenize("");

    expect_token_count(result, 1);
    const Token &end = expect_token(result, 0, TType::EndOfInput, "");
    expect(std::holds_alternative<std::monostate>(end.value),
           "end-of-input should not have a value");
    expect_position(end.span.begin, 0, 1, 1);
    expect_position(end.span.end, 0, 1, 1);
  });

  suite.run("punctuation and operators use maximal munch", [] {
    const Result result = Lexer::tokenize(
        "( ) { } , . ; + - * & @ = ! != < <= > >= && ||");
    const std::vector<ExpectedToken> expected{
        {TType::LeftParen, "("},     {TType::RightParen, ")"},
        {TType::LeftBrace, "{"},     {TType::RightBrace, "}"},
        {TType::Comma, ","},         {TType::Dot, "."},
        {TType::Semicolon, ";"},     {TType::Plus, "+"},
        {TType::Minus, "-"},         {TType::Star, "*"},
        {TType::Ampersand, "&"},     {TType::At, "@"},
        {TType::Equal, "="},         {TType::Bang, "!"},
        {TType::BangEqual, "!="},    {TType::Less, "<"},
        {TType::LessEqual, "<="},    {TType::Greater, ">"},
        {TType::GreaterEqual, ">="}, {TType::AndAnd, "&&"},
        {TType::OrOr, "||"},         {TType::EndOfInput, ""},
    };

    expect_token_count(result, expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i)
      expect_token(result, i, expected[i].type, expected[i].lexeme);
  });

  suite.run("keywords are case-insensitive and identifiers preserve spelling",
            [] {
              const Result result = Lexer::tokenize(
                  "relation INSERT SeLeCt project renameTable "
                  "RENAMEATTRIBUTE string NUMBER selectValue union name_2");
              const std::vector<ExpectedToken> expected{
                  {TType::Relation, "relation"},
                  {TType::Insert, "INSERT"},
                  {TType::Select, "SeLeCt"},
                  {TType::Project, "project"},
                  {TType::RenameTable, "renameTable"},
                  {TType::RenameAttribute, "RENAMEATTRIBUTE"},
                  {TType::StringType, "string"},
                  {TType::NumberType, "NUMBER"},
                  {TType::Identifier, "selectValue"},
                  {TType::Identifier, "union"},
                  {TType::Identifier, "name_2"},
                  {TType::EndOfInput, ""},
              };

              expect_token_count(result, expected.size());
              for (std::size_t i = 0; i < expected.size(); ++i)
                expect_token(result, i, expected[i].type, expected[i].lexeme);

              expect_string_value(result.tokens[8], "selectValue");
              expect_string_value(result.tokens[9], "union");
              expect_string_value(result.tokens[10], "name_2");
            });

  suite.run("numbers include negatives and decimals", [] {
    const Result result =
        Lexer::tokenize("0 30 -30 6.2 -0.5 5. A-B Age>-30");

    expect_token_count(result, 13);
    const std::vector<double> expected_values{0.0, 30.0, -30.0,
                                               6.2, -0.5, 5.0};
    const std::vector<std::string_view> expected_lexemes{
        "0", "30", "-30", "6.2", "-0.5", "5."};

    for (std::size_t i = 0; i < expected_values.size(); ++i) {
      const Token &token =
          expect_token(result, i, TType::Number, expected_lexemes[i]);
      expect_number_value(token, expected_values[i]);
    }

    expect_token(result, 6, TType::Identifier, "A");
    expect_token(result, 7, TType::Minus, "-");
    expect_token(result, 8, TType::Identifier, "B");
    expect_token(result, 9, TType::Identifier, "Age");
    expect_token(result, 10, TType::Greater, ">");
    expect_number_value(expect_token(result, 11, TType::Number, "-30"),
                        -30.0);
    expect_token(result, 12, TType::EndOfInput, "");
  });

  suite.run("a number with two decimal points is rejected", [] {
    const Result result = Lexer::tokenize("1.2.3");
    expect_error(result, EType::InvalidNumber);
    expect_position(result.error->span.begin, 0, 1, 1);
  });

  suite.run("quoted strings decode doubled apostrophes", [] {
    const Result result =
        Lexer::tokenize("'Tom' '' 'O''Brien' '/* still text */'");

    expect_token_count(result, 5);
    expect_string_value(expect_token(result, 0, TType::String, "'Tom'"),
                        "Tom");
    expect_string_value(expect_token(result, 1, TType::String, "''"), "");
    expect_string_value(
        expect_token(result, 2, TType::String, "'O''Brien'"), "O'Brien");
    expect_string_value(
        expect_token(result, 3, TType::String, "'/* still text */'"),
        "/* still text */");
    expect_token(result, 4, TType::EndOfInput, "");
  });

  suite.run("newlines inside strings are rejected", [] {
    const Result result = Lexer::tokenize("'hello\nworld'");
    expect_error(result, EType::NewlineInString);
    expect_position(result.error->span.begin, 0, 1, 1);
  });

  suite.run("unterminated strings are rejected", [] {
    const Result result = Lexer::tokenize("'hello");
    expect_error(result, EType::UnterminatedString);
    expect_position(result.error->span.begin, 0, 1, 1);
  });

  suite.run("comments are ignored and positions cross lines", [] {
    const Result result =
        Lexer::tokenize("relation A/* first\nsecond */\n{\t};");

    expect_token_count(result, 6);
    const Token &relation =
        expect_token(result, 0, TType::Relation, "relation");
    const Token &name = expect_token(result, 1, TType::Identifier, "A");
    const Token &left_brace =
        expect_token(result, 2, TType::LeftBrace, "{");
    const Token &right_brace =
        expect_token(result, 3, TType::RightBrace, "}");
    const Token &semicolon =
        expect_token(result, 4, TType::Semicolon, ";");
    const Token &end = expect_token(result, 5, TType::EndOfInput, "");

    expect_position(relation.span.begin, 0, 1, 1);
    expect_position(relation.span.end, 8, 1, 9);
    expect_position(name.span.begin, 9, 1, 10);
    expect_position(left_brace.span.begin, 29, 3, 1);
    expect_position(right_brace.span.begin, 31, 3, 3);
    expect_position(semicolon.span.begin, 32, 3, 4);
    expect_position(end.span.begin, 33, 3, 5);
  });

  suite.run("unterminated comments are rejected", [] {
    const Result result = Lexer::tokenize("/* unfinished");
    expect_error(result, EType::UnterminatedComment);
    expect_position(result.error->span.begin, 0, 1, 1);
  });

  suite.run("a lone pipe is rejected", [] {
    const Result result = Lexer::tokenize("|");
    expect_error(result, EType::LonePipe);
  });

  suite.run("a slash that does not open a comment is rejected", [] {
    const Result result = Lexer::tokenize("/");
    expect_error(result, EType::UnexpectedCharacter);
  });

  suite.run("unexpected characters are rejected instead of skipped", [] {
    const Result result = Lexer::tokenize("_name");
    expect_error(result, EType::UnexpectedCharacter);
    expect(result.tokens.empty(), "lexer should stop before emitting tokens");
  });

  return suite.finish();
}
