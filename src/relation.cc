#include "relation.h"
#include "schema.h"
#include <stdexcept>
#include <utility>

bool TupleCollection::values_equal(const Value &left, const Value &right) {
  if (left.index() != right.index())
    return false;

  if (const auto *left_number = std::get_if<double>(&left))
    return *left_number == std::get<double>(right);

  return std::get<std::string>(left) == std::get<std::string>(right);
}

bool TupleCollection::tuples_equal(const Tuple &left, const Tuple &right) {
  if (left.size() != right.size())
    return false;

  for (std::size_t i = 0; i < left.size(); ++i)
    if (!values_equal(left[i], right[i]))
      return false;

  return true;
}

bool TupleCollection::contains(const Tuple &tuple) const {
  for (const Tuple &existing : tuples_)
    if (tuples_equal(existing, tuple))
      return true;

  return false;
}

void TupleCollection::insert_unique(const Tuple &tuple) {
  if (!contains(tuple))
    tuples_.push_back(tuple);
}

std::size_t TupleCollection::size() const { return tuples_.size(); }

TupleCollection::const_iterator TupleCollection::begin() const {
  return tuples_.begin();
}

TupleCollection::const_iterator TupleCollection::end() const {
  return tuples_.end();
}

Relation::Relation(std::string n, Schema s)
    : name_(std::move(n)), schema_(std::move(s)) {}

void Relation::insert_row(const Tuple &tuple) {
  validate_row(tuple);
  tuples_.insert_unique(tuple);
}

bool Relation::contains(const Tuple &tuple) const {
  return tuples_.contains(tuple);
}

const TupleCollection &Relation::tuples() const { return tuples_; }

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
