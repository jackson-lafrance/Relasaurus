#include "../include/relations.h"
#include <stdexcept>

Relation::Relation(std::string n, AttributeNames attrs) {
  name = n;

  setupAttributes(attrs);
};

Relation::Relation(std::string n, std::initializer_list<Tuple> tup,
                   AttributeNames attrs) {
  name = n;

  setupAttributes(attrs);

  for (const auto &tuple : tup) {
    validateSchema(tuple);
  }

  tuples = tup;
};

void Relation::setupAttributes(AttributeNames attrs) {
  attributes = attrs;
  attribute_indexes.resize(attributes.size());

  for (const auto &[name, attribute] : attributes) {
    attribute_indexes.at(attribute.index) = attribute.type;
  }
}

void Relation::validateSchema(const Tuple &tuple) {
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
