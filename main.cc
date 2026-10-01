#include "interpreter.h"
#include "repl.h"

#include <exception>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>

void print_relation(const Relation &relation) {
  std::cout << relation.name() << '\n';

  for (const Column &column : relation.schema().columns()) {
    std::cout << column.name << '\t';
  }
  std::cout << '\n';

  for (const Tuple &tuple : relation.tuples()) {
    for (const Value &value : tuple) {
      std::visit([](const auto &item) { std::cout << item << '\t'; }, value);
    }
    std::cout << '\n';
  }
}

std::string attribute_name(const AttributeReference &attribute) {
  if (attribute.relation_name.has_value()) {
    return attribute.relation_name->name + "." + attribute.attribute_name.name;
  }
  return attribute.attribute_name.name;
}

std::string operand_text(const Operand &operand) {
  if (const auto *number = std::get_if<double>(&operand.value)) {
    std::ostringstream output;
    output << *number;
    return output.str();
  }

  if (const auto *string = std::get_if<std::string>(&operand.value)) {
    return "'" + *string + "'";
  }

  return attribute_name(std::get<AttributeReference>(operand.value));
}

std::string comparison_text(ComparisonOperator operation) {
  switch (operation) {
  case ComparisonOperator::Equal:
    return "=";
  case ComparisonOperator::NotEqual:
    return "!=";
  case ComparisonOperator::Less:
    return "<";
  case ComparisonOperator::LessEqual:
    return "<=";
  case ComparisonOperator::Greater:
    return ">";
  case ComparisonOperator::GreaterEqual:
    return ">=";
  }
  throw std::logic_error("UNKNOWN COMPARISON OPERATOR!!!!");
}

std::string condition_text(const Condition &condition) {
  if (const auto *comparison =
          std::get_if<ComparisonCondition>(&condition.node)) {
    return operand_text(comparison->left) + " " +
           comparison_text(comparison->lil_durk) + " " +
           operand_text(comparison->right);
  }

  if (const auto *logical =
          std::get_if<LogicalCondition>(&condition.node)) {
    const std::string operation =
        logical->king_von == LogicalOperator::And ? " && " : " || ";
    return "(" + condition_text(*logical->left) + operation +
           condition_text(*logical->right) + ")";
  }

  const auto &negation = std::get<NotCondition>(condition.node);
  return "!(" + condition_text(*negation.operand) + ")";
}

std::string binary_name(BinaryOperator operation) {
  switch (operation) {
  case BinaryOperator::UNION:
    return "Union";
  case BinaryOperator::INTERSECT:
    return "Intersect";
  case BinaryOperator::MINUS:
    return "Minus";
  case BinaryOperator::TIMES:
    return "Times";
  }
  throw std::logic_error("UNKNOWN BINARY OPERATOR!!!!");
}

std::string attributes_text(
    const std::vector<AttributeReference> &attributes) {
  std::string result;
  for (std::size_t i = 0; i < attributes.size(); ++i) {
    if (i > 0) {
      result += ", ";
    }
    result += attribute_name(attributes[i]);
  }
  return result;
}

void print_indent(int depth) {
  std::cout << std::string(depth * 2, ' ');
}

void print_tree(const REX &expression, int depth) {
  print_indent(depth);

  if (const auto *relation = std::get_if<NameWithSpan>(&expression.node)) {
    std::cout << "Relation(" << relation->name << ")\n";
    return;
  }

  if (const auto *binary =
          std::get_if<BinaryExpression>(&expression.node)) {
    std::cout << binary_name(binary->operation) << '\n';
    print_tree(*binary->left, depth + 1);
    print_tree(*binary->right, depth + 1);
    return;
  }

  if (const auto *join = std::get_if<JoinExpression>(&expression.node)) {
    std::cout << "Join(" << condition_text(join->condition) << ")\n";
    print_tree(*join->left, depth + 1);
    print_tree(*join->right, depth + 1);
    return;
  }

  if (const auto *selection =
          std::get_if<SelectExpression>(&expression.node)) {
    std::cout << "Select(" << condition_text(selection->condition) << ")\n";
    print_tree(*selection->input, depth + 1);
    return;
  }

  if (const auto *projection =
          std::get_if<ProjectExpression>(&expression.node)) {
    std::cout << "Project(" << attributes_text(projection->attributes)
              << ")\n";
    print_tree(*projection->input, depth + 1);
    return;
  }

  if (const auto *rename =
          std::get_if<RenameTableExpression>(&expression.node)) {
    std::cout << "RenameTable(" << rename->new_name.name << ")\n";
    print_tree(*rename->input, depth + 1);
    return;
  }

  if (const auto *rename =
          std::get_if<RenameAttributeExpression>(&expression.node)) {
    std::cout << "RenameAttribute(" << rename->new_name.name << ", "
              << attribute_name(rename->old_name) << ")\n";
    print_tree(*rename->input, depth + 1);
    return;
  }

  throw std::logic_error("UNKNOWN EXPRESSION!!!!");
}

void execute_statement(Interpreter &interpreter, const Statement &statement) {
  try {
    if (auto result = interpreter.execute(statement)) {
      print_relation(*result);
    } else {
      std::cout << "OK\n";
    }
  } catch (const std::exception &error) {
    std::cerr << "runtime error: " << error.what() << '\n';
  }
}

void print_statement_tree(const Statement &statement) {
  const auto *query = std::get_if<REX>(&statement.guy);
  if (query == nullptr) {
    std::cerr << "TREE ERROR: EXPECTED A QUERY STATEMENT!!!!\n";
    return;
  }
  print_tree(*query, 0);
}

int main() {
  std::cout << "Choose a mode:\n"
            << "1) live\n"
            << "2) tree\n"
            << "> " << std::flush;

  std::string choice;
  if (!std::getline(std::cin, choice)) {
    return 0;
  }

  if (choice != "1" && choice != "2") {
    std::cerr << "INVALID MODE!!!!\n";
    return 1;
  }

  const bool tree_mode = choice == "2";
  Interpreter interpreter;
  std::string source;
  std::string line;

  while (true) {
    std::cout << (source.empty() ? "> " : "... ") << std::flush;

    if (!std::getline(std::cin, line)) {
      if (!source.empty()) {
        std::cerr << "SYNTAX ERROR: INCOMPLETE INPUT AT END OF FILE!!!!\n";
      }
      break;
    }

    if (source.empty() && (line == ":quit" || line == ":q")) {
      break;
    }

    if (source.empty() && line.empty()) {
      continue;
    }

    source += line + '\n';
    ParsedInput parsed = parse_repl_source(source);

    if (parsed.status == ParseStatus::Incomplete) {
      continue;
    }

    if (parsed.status == ParseStatus::Success) {
      if (tree_mode) {
        print_statement_tree(parsed.statement);
      } else {
        execute_statement(interpreter, parsed.statement);
      }
    }

    source.clear();
  }

  return 0;
}
