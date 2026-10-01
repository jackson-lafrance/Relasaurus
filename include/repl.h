#ifndef REPL_H
#define REPL_H

#include "parser.h"

#include <optional>
#include <string>

enum class ParseStatus { Success, Incomplete, Error };

struct ParsedInput {
  ParseStatus status;
  Statement statement;
  std::optional<ParseError> incomplete_error;
};

ParsedInput parse_repl_source(const std::string &source);
void print_parse_error(const ParseError &error, const std::string &source);

#endif
