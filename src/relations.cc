#include "../include/relations.h"
#include <initializer_list>
#include <stdexcept>

Relation::Relation(std::string n, AttributeNames attrs) {
  name = n;

  setup_schema(attrs);
};

Relation::Relation(std::string n, std::initializer_list<Tuple> tup,
                   AttributeNames attrs) {
  name = n;

  setup_schema(attrs);

  insert_rows(tup);
};

void Relation::insert_rows(std::initializer_list<Tuple> tup) {
  for (const auto &tuple : tup) {
    validate_schema(tuple);
  }

  for (const auto &tuple : tup) {
    insert_tuple(tuple);
  }
}

void Relation::insert_row(const Tuple &tuple) {
  validate_schema(tuple);
  insert_tuple(tuple);
}

std::set<Tuple> Relation::get_rows() const {
  return tuples;
};

std::string Relation::get_name() const {
  return name;
};

AttributeNames Relation::get_schema() const {
  return attributes;
};

AttributeIndexes Relation::get_schema_indexes() const {
  return attribute_indexes;
};

std::string Relation::toString() const {
  struct ValueFormatter {
    std::string operator()(int value) const { return std::to_string(value); }
    std::string operator()(double value) const { return std::to_string(value); }
    std::string operator()(const std::string &value) const { return value; }
  };

  std::vector<std::string> column_names(attribute_indexes.size());
  for (const auto &[attribute_name, attribute] : attributes) {
    column_names.at(attribute.index) = attribute_name;
  }

  std::vector<std::vector<std::string>> rows;
  for (const auto &tuple : tuples) {
    std::vector<std::string> row;
    for (const auto &value : tuple) {
      row.push_back(std::visit(ValueFormatter{}, value));
    }
    rows.push_back(row);
  }

  std::vector<std::size_t> widths(column_names.size(), 0);
  for (std::size_t i = 0; i < column_names.size(); ++i) {
    widths[i] = column_names[i].size();
  }
  for (const auto &row : rows) {
    for (std::size_t i = 0; i < row.size(); ++i) {
      if (row[i].size() > widths[i]) {
        widths[i] = row[i].size();
      }
    }
  }

  std::string output = "Relation: " + name + "\n";
  if (widths.empty()) {
    return output;
  }

  auto separator = [&]() {
    std::string line = "+";
    for (const auto width : widths) {
      line += std::string(width + 2, '-');
      line += "+";
    }
    return line + "\n";
  };

  auto format_row = [&](const std::vector<std::string> &row) {
    std::string line = "|";
    for (std::size_t i = 0; i < widths.size(); ++i) {
      const std::string &value = row[i];
      line += " " + value;
      line += std::string(widths[i] - value.size() + 1, ' ');
      line += "|";
    }
    return line + "\n";
  };

  output += separator();
  output += format_row(column_names);
  output += separator();
  for (const auto &row : rows) {
    output += format_row(row);
  }
  output += separator();

  return output;
}

void Relation::setup_schema(AttributeNames attrs) {
  attributes = attrs;
  attribute_indexes.resize(attributes.size());

  for (const auto &[name, attribute] : attributes) {
    attribute_indexes.at(attribute.index) = attribute.type;
  }
}

void Relation::validate_schema(const Tuple &tuple) {
  if (tuple.size() != attributes.size()) {
    throw std::runtime_error("TUPLE LENGTH != ATTRIBUTES LENGTH");
  }
  for (std::size_t i = 0; i < tuple.size(); i++) {
    bool matches = false;
    switch (attribute_indexes.at(i)) {
    case STRING:
      matches = std::holds_alternative<std::string>(tuple[i]);
      break;
    case INTEGER:
      matches = std::holds_alternative<int>(tuple[i]);
      break;
    case DOUBLE:
      matches = std::holds_alternative<double>(tuple[i]);
      break;
    }
    if (!matches)
      throw std::runtime_error("TUPLE TYPES DO NOT MATCH SCHEMA");
  }
}

void Relation::insert_tuple(const Tuple &tuple) { tuples.insert(tuple); }
