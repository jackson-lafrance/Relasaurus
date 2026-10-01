#include "repl.h"

#include "lexer.h"

#include <iostream>
#include <utility>
#include <variant>

std::string source_line(const std::string &source, std::size_t wanted_row) {
  std::size_t line_start = 0;

  for (std::size_t row = 1; row < wanted_row; ++row) {
    const std::size_t newline = source.find('\n', line_start);
    if (newline == std::string::npos) {
      return "";
    }
    line_start = newline + 1;
  }

  const std::size_t line_end = source.find('\n', line_start);
  if (line_end == std::string::npos) {
    return source.substr(line_start);
  }
  return source.substr(line_start, line_end - line_start);
}

void print_diagnostic(const std::string &category, const std::string &message,
                      const Span &span, const std::string &source) {
  std::cerr << category << " at " << span.begin.row << ':' << span.begin.col
            << ": " << message << '\n'
            << source_line(source, span.begin.row) << '\n'
            << std::string(span.begin.col - 1, ' ') << "^\n";
}

ParsedInput parse_repl_source(const std::string &source) {
  Result tokenized = Lexer::tokenize(source);

  if (tokenized.error.has_value()) {
    if (tokenized.error->type == EType::UnterminatedComment) {
      return {.status = ParseStatus::Incomplete, .program = {}};
    }

    print_diagnostic("lexer error", tokenized.error->message,
                     tokenized.error->span, source);
    return {.status = ParseStatus::Error, .program = {}};
  }

  auto parsed = Parser::parse_tokens(tokenized.tokens);

  if (const auto *error = std::get_if<ParseError>(&parsed)) {
    if (error->actual == TType::EndOfInput) {
      return {.status = ParseStatus::Incomplete, .program = {}};
    }

    print_diagnostic("syntax error", error->message, error->span, source);
    return {.status = ParseStatus::Error, .program = {}};
  }

  return {.status = ParseStatus::Success,
          .program = std::get<Program>(std::move(parsed))};
}
