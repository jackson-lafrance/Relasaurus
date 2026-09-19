#ifndef RELATION_H
#define RELATION_H

#include "schema.h"
#include <set>
#include <string>
#include <variant>
#include <vector>

using Value = std::variant<double, std::string>;
using Tuple = std::vector<Value>;

class Relation {
public:
  Relation(std::string n, Schema schem);

  void insert_row(const Tuple &tuple);

  const std::set<Tuple> &tuples() const;
  const Schema &schema() const;
  const std::string &name() const;

private:
  std::string name_;
  Schema schema_;
  std::set<Tuple> tuples_;

  void validate_row(const Tuple &tuple);
};

#endif // RELATION_H
