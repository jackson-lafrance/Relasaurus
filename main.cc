#include "interpreter.h"
#include "lexer.h"
#include "parser.h"
#include <algorithm>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>
#include <variant>

void print_relation(const Relation &relation) {
  std::cout << relation.name() << std::endl;

  for (const Column &column : relation.schema().columns()) {
    std::cout << column.name << '\t';
  }
  std::cout << std::endl;

  for (const Tuple &tuple : relation.tuples()) {
    for (const Value &value : tuple) {
      std::visit([](const auto &item) { std::cout << item << '\t'; }, value);
    }
    std::cout << std::endl;
  }
}

enum class RunStatus { Success, Incomplete, Error };

bool has_final_semicolon(const std::string &source) {
  int brace_depth = 0;
  int parenthesis_depth = 0;
  bool in_string = false;
  bool in_comment = false;
  bool unmatched_closer = false;
  bool last_token_is_semicolon = false;

  for (std::size_t i = 0; i < source.size(); ++i) {
    const char current = source[i];
    const char next = i + 1 < source.size() ? source[i + 1] : '\0';

    if (in_comment) {
      if (current == '*' && next == '/') {
        in_comment = false;
        ++i;
      }
      continue;
    }

    if (in_string) {
      if (current == '\'' && next == '\'') {
        ++i;
      } else if (current == '\'') {
        in_string = false;
      }
      continue;
    }

    if (current == '/' && next == '*') {
      in_comment = true;
      ++i;
      continue;
    }

    if (current == '\'') {
      in_string = true;
      last_token_is_semicolon = false;
      continue;
    }

    if (current == ' ' || current == '\t' || current == '\r' ||
        current == '\n') {
      continue;
    }

    last_token_is_semicolon = current == ';';

    if (current == '{') {
      ++brace_depth;
    } else if (current == '}') {
      --brace_depth;
      unmatched_closer = unmatched_closer || brace_depth < 0;
    } else if (current == '(') {
      ++parenthesis_depth;
    } else if (current == ')') {
      --parenthesis_depth;
      unmatched_closer = unmatched_closer || parenthesis_depth < 0;
    }
  }

  const bool delimiters_closed = brace_depth == 0 && parenthesis_depth == 0;
  return last_token_is_semicolon && !in_string && !in_comment &&
         (delimiters_closed || unmatched_closer);
}

void print_diagnostic(std::string_view category, std::string_view message,
                      const Span &span, const std::string &source) {
  std::cerr << category << " at " << span.begin.row << ':' << span.begin.col
            << ": " << message << '\n';

  const std::size_t offset = std::min(span.begin.offset, source.size());
  std::size_t line_start = 0;
  if (offset > 0) {
    const std::size_t previous_newline = source.rfind('\n', offset - 1);
    if (previous_newline != std::string::npos) {
      line_start = previous_newline + 1;
    }
  }

  const std::size_t next_newline = source.find('\n', offset);
  const std::size_t line_end =
      next_newline == std::string::npos ? source.size() : next_newline;
  std::cerr << source.substr(line_start, line_end - line_start) << '\n'
            << std::string(span.begin.col - 1, ' ') << "^\n";
}

RunStatus run_source(Interpreter &interpreter, const std::string &source) {
  Result tokenized = Lexer::tokenize(source);

  if (tokenized.error.has_value()) {
    if (tokenized.error->type == EType::UnterminatedComment) {
      return RunStatus::Incomplete;
    }

    print_diagnostic("lexer error", tokenized.error->message,
                     tokenized.error->span, source);
    return RunStatus::Error;
  }

  std::variant<Program, ParseError> parsed =
      Parser::parse_tokens(tokenized.tokens);

  if (const auto *error = std::get_if<ParseError>(&parsed)) {
    if (error->actual == TType::EndOfInput) {
      return RunStatus::Incomplete;
    }

    print_diagnostic("syntax error", error->message, error->span, source);
    return RunStatus::Error;
  }

  const Program &program = std::get<Program>(parsed);
  bool printed_relation = false;

  try {
    for (const Statement &statement : program.statements) {
      if (auto result = interpreter.execute(statement)) {
        print_relation(*result);
        printed_relation = true;
      }
    }
  } catch (const std::exception &error) {
    std::cerr << "runtime error: " << error.what() << '\n';
    return RunStatus::Error;
  }

  if (!printed_relation) {
    std::cout << "OK\n";
  }

  return RunStatus::Success;
}

int main() {
  Interpreter interpreter;
  std::string source;
  std::string line;

  std::cout << "Relasaurus REPL (:quit to exit)\n";

  while (true) {
    std::cout << (source.empty() ? "> " : "... ") << std::flush;

    if (!std::getline(std::cin, line)) {
      if (!source.empty()) {
        std::cerr << "syntax error: incomplete input at end of file\n";
      }
      break;
    }

    if (source.empty() && (line == ":quit" || line == ":q")) {
      break;
    }

    if (source.empty() && line.empty()) {
      continue;
    }

    source += line;
    source += '\n';

    if (!has_final_semicolon(source)) {
      continue;
    }

    const RunStatus status = run_source(interpreter, source);
    if (status != RunStatus::Incomplete) {
      source.clear();
    }
  }

  return 0;
}
