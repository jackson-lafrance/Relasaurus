#ifndef RELATIONS_H
#define RELATIONS_H

#include <set>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

enum Type { STRING, INTEGER, DOUBLE };

struct Attribute {
  int index;
  Type type;
};

using Value = std::variant<int, double, std::string>;
using AttributeNames = std::unordered_map<std::string, Attribute>;
using AttributeIndexes = std::vector<Type>;
using Tuple = std::vector<Value>;

class Relation {
public:
  Relation(std::string n, AttributeNames attrs);
  Relation(std::string n, std::set<Tuple> tup, AttributeNames attrs);

  void insert_row(const Tuple &tuple);
  void insert_rows(const std::set<Tuple> &tup);

  const std::set<Tuple> &get_rows() const;
  std::string get_name() const;
  AttributeNames get_schema() const;
  AttributeIndexes get_schema_indexes() const;

  std::string toString() const;

private:
  std::string name;
  AttributeNames attributes;
  AttributeIndexes attribute_indexes;
  std::set<Tuple> tuples;

  void setup_schema(AttributeNames attrs);
  void validate_schema(const Tuple &tuple);
  void validate_rows(const std::set<Tuple> &tup);
};

#endif // RELATIONS_H
