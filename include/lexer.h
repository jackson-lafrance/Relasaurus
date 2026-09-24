#ifndef ALGEBRA_H
#define ALGEBRA_H

#include <cstddef>
#include <optional>
#include <string>
#include <variant>
#include <vector>

enum class TType {
  // Values and names
  Identifier,
  Number,
  String,

  // Reserved words
  Relation,
  Insert,
  Select,
  Project,
  RenameTable,
  RenameAttribute,
  IString,
  INumber,

  // Delimiters
  LeftParen,
  RightParen,
  LeftBrace,
  RightBrace,
  Comma,
  Dot,
  Semicolon,

  // Relational operators
  Plus,
  Minus,
  Star,
  Ampersand,
  At,

  // Comparison and Boolean operators
  Equal,
  Bang,
  BangEqual,
  Less,
  LessEqual,
  Greater,
  GreaterEqual,
  AndAnd,
  OrOr,

  EndOfInput,
};

struct Position {
  std::size_t offset{0};
  std::size_t row{1};
  std::size_t col{1};
};

struct Span {
  Position begin;
  Position end;
};

using TokenValue = std::variant<std::monostate, std::string, double>;

struct Token {
  TType type;
  std::string lexeme;
  TokenValue value;
  Span span;
};

enum class EType {
  UnexpectedCharacter,
  LonePipe,
  UnterminatedString,
  NewlineInString,
  UnterminatedComment,
  InvalidNumber,
};

struct Error {
  EType kind;
  std::string message;
  Span span;
};

struct Result {
  std::vector<Token> tokens;
  std::optional<Error> error;
};

class Lexer {
public:
  static Result tokenize(std::string &source);

private:
  explicit Lexer(std::string &source);

  void scan_token();
  void scan_ident();
  void scan_number();
  void scan_string();
  void scan_comment();

  void emit(TType type);
  void emit(TType type, Token value);

  void fail(Error kind, std::string message, Position begin);

  bool at_end() const;
  char advance();
  char peek() const;
  char peek_next() const;
  bool match(char expected);

  static bool is_ascii_letter(char character) noexcept;
  static bool is_digit(char character) noexcept;
  static bool is_ident(char character);

  static bool equals_ignore_case(std::string &left, std::string &right);

  static std::optional<TType>
  keyword_kind(std::string_view identifier) noexcept;

  std::string source_;
  Position token_start_;
  Position current_;

  std::vector<Token> tokens_;
  std::optional<Error> error_;
};

#endif // ALGEBRA_H
