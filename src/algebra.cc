#include "../include/algebra.h"
#include <iostream>
#include <stdexcept>

Relation
Algebra::selection(const Relation &relation,
                   std::function<bool(Tuple, AttributeNames)> predicate) {
  Relation out(relation.get_name(), relation.get_schema());

  for (const auto &tuple : relation.get_rows()) {
    if (predicate(tuple, relation.get_schema())) {
      out.insert_row(tuple);
    }
  }
  return out;
}

Relation Algebra::projection(const Relation &relation,
                             std::initializer_list<std::string> attributes) {
  AttributeNames new_schema;

  const auto schema = relation.get_schema();

  for (const std::string &attribute : attributes) {
    auto attr = schema.find(attribute);

    if (attr == schema.end()) {
      throw std::runtime_error("UNKNOWN ATTRIBUTE");
    }

    new_schema[attribute] = {static_cast<int>(new_schema.size()),
                             attr->second.type};
  }

  Relation out(relation.get_name(), new_schema);

  for (const auto &tuple : relation.get_rows()) {
    Tuple new_tuple;
    for (const std::string &attribute : attributes) {
      const auto &attr = schema.at(attribute);
      new_tuple.push_back(tuple.at(attr.index));
    }
    out.insert_row(new_tuple);
  }

  return out;
}

bool condition(Tuple tuple, AttributeNames schema) {
  return std::get<int>(tuple[schema["Grade"].index]) > 80;
}

int main() {
  AttributeNames attr1 = {
      {"Name", {0, STRING}}, {"Grade", {1, INTEGER}}, {"ID", {2, INTEGER}}};

  Relation relation1 = Relation(
      "students", {{"Bobby", 99, 1}, {"Selsabeel", 88, 2}, {"Moses", 77, 3}},
      attr1);

  std::cout << relation1.toString() << std::endl;

  std::cout << Algebra::selection(relation1, condition).toString();

  std::cout << Algebra::projection(relation1, {"Name", "ID"}).toString();
  std::cout << Algebra::projection(
      Algebra::selection(relation1, condition),
      {"Name", "ID"})
                   .toString();

  return 0;
}
