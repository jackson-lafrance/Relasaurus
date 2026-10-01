#ifndef REPL_H
#define REPL_H

#include "parser.h"

#include <string>

enum class ParseStatus { Success, Incomplete, Error };

struct ParsedInput {
  ParseStatus status;
  Statement statement;
};

ParsedInput parse_repl_source(const std::string &source);

#endif
