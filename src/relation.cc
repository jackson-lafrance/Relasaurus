#include "relation.h"
#include "schema.h"
#include <stdexcept>
#include <utility>

Relation::Relation(std::string n, Schema s)
    : name_(std::move(n)), schema_(std::move(s)) {}

void Relation::insert_row(const Tuple &tuple) {
  validate_row(tuple);
  tuples_.insert(tuple);
}

const std::set<Tuple> &Relation::tuples() const { return tuples_; }

const std::string &Relation::name() const { return name_; }
const Schema &Relation::schema() const { return schema_; }

void Relation::validate_row(const Tuple &tuple) {
  if (tuple.size() != schema_.columns().size()) {
    throw std::runtime_error("TUPLE LENGTH != ATTRIBUTES LENGTH");
  }
  for (std::size_t i{}; i < tuple.size(); ++i) {
    bool matches = false;
    switch (schema_.columns()[i].type) {
    case STRING:
      matches = std::holds_alternative<std::string>(tuple[i]);
      break;
    case NUMBER:
      matches = std::holds_alternative<double>(tuple[i]);
      break;
    }
    if (!matches)
      throw std::runtime_error("TUPLE TYPES DO NOT MATCH SCHEMA");
  }
}
