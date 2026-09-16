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

std::set<Tuple> Relation::get_rows() {
  return tuples;
};

std::string Relation::get_name() {
  return name;
};

AttributeNames Relation::get_schema() {
  return attributes;
};

AttributeIndexes Relation::get_schema_indexes() {
  return attribute_indexes;
};

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
