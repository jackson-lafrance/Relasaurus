#include "../include/algebra.h"
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

Relation
Algebra::selection(const Relation &relation,
                   std::function<bool(Tuple, AttributeNames)> predicate) {
  std::set<Tuple> rows;
  auto schema = relation.get_schema();

  for (const auto &tuple : relation.get_rows()) {
    if (predicate(tuple, schema)) {
      rows.insert(tuple);
    }
  }

  return Relation(relation.get_name(), std::move(rows), std::move(schema));
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

  std::set<Tuple> rows;
  for (const auto &tuple : relation.get_rows()) {
    Tuple new_tuple;
    new_tuple.reserve(attributes.size());
    for (const std::string &attribute : attributes) {
      const auto &attr = schema.at(attribute);
      new_tuple.push_back(tuple.at(attr.index));
    }
    rows.insert(std::move(new_tuple));
  }

  return Relation(relation.get_name(), std::move(rows),
                  std::move(new_schema));
}

Relation Algebra::rename(const Relation &relation, std::string new_name) {
  auto rows = relation.get_rows();
  auto schema = relation.get_schema();
  return Relation(std::move(new_name), std::move(rows), std::move(schema));
}

Relation Algebra::rename(const Relation &relation, std::string old_attr,
                         std::string new_attr) {
  AttributeNames new_schema = relation.get_schema();

  auto old_attribute = new_schema.find(old_attr);

  if (old_attribute == new_schema.end()) {
    throw std::runtime_error("UNKNOWN ATTRIBUTE: " + old_attr);
  }

  if (old_attr != new_attr && new_schema.find(new_attr) != new_schema.end()) {
    throw std::runtime_error("ATTRIBUTE ALREADY EXISTS: " + new_attr);
  }

  new_schema[new_attr] = old_attribute->second;
  new_schema.erase(old_attr);

  auto rows = relation.get_rows();
  return Relation(relation.get_name(), std::move(rows),
                  std::move(new_schema));
}

Relation Algebra::times(const Relation &rel_1, const Relation &rel_2) {
  AttributeNames attribute_names;
  const auto schema_1 = rel_1.get_schema();
  const auto schema_2 = rel_2.get_schema();
  const auto name_1 = rel_1.get_name();
  const auto name_2 = rel_2.get_name();

  int length_of_attrs_1 = 0;
  for (const auto &attr : schema_1) {
    length_of_attrs_1++;
    attribute_names[name_1 + "." + attr.first] = attr.second;
  }

  for (const auto &attr : schema_2) {
    attribute_names[name_2 + "." + attr.first] = {
        .index = attr.second.index + length_of_attrs_1,
        .type = attr.second.type};
  }

  std::set<Tuple> rows;
  const auto &rows_1 = rel_1.get_rows();
  const auto &rows_2 = rel_2.get_rows();
  for (const auto &tuple_1 : rows_1) {
    for (const auto &tuple_2 : rows_2) {
      Tuple new_tuple;
      new_tuple.reserve(tuple_1.size() + tuple_2.size());
      new_tuple.insert(new_tuple.end(), tuple_1.begin(), tuple_1.end());
      new_tuple.insert(new_tuple.end(), tuple_2.begin(), tuple_2.end());
      rows.insert(std::move(new_tuple));
    }
  }

  return Relation(name_1 + "_X_" + name_2, std::move(rows),
                  std::move(attribute_names));
}

Relation Algebra::join(const Relation &rel_1, const Relation &rel_2,
                       std::function<bool(Tuple, AttributeNames)> predicate) {
  return selection(times(rel_1, rel_2), predicate);
}

Relation Algebra::onion(const Relation &rel_1, const Relation &rel_2) {
  if (!compare_schemas(rel_1, rel_2))
    throw std::runtime_error("SCHEMA'S NOT COMPATIBLE");

  auto rows = rel_1.get_rows();
  const auto &rows_2 = rel_2.get_rows();
  rows.insert(rows_2.begin(), rows_2.end());
  auto schema = rel_1.get_schema();
  return Relation(rel_1.get_name(), std::move(rows), std::move(schema));
}

bool Algebra::compare_schemas(const Relation &rel_1, const Relation &rel_2) {
  const auto schema_1 = rel_1.get_schema();
  const auto schema_2 = rel_2.get_schema();

  if (schema_1.size() != schema_2.size()) {
    return false;
  }

  for (const auto &[name, attr_1] : schema_1) {
    const auto it = schema_2.find(name);

    if (it == schema_2.end()) {
      return false;
    }

    const auto &attr_2 = it->second;

    if (attr_1.index != attr_2.index || attr_1.type != attr_2.type) {
      return false;
    }
  }

  return true;
}

bool condition(Tuple tuple, AttributeNames schema) {
  return std::get<int>(tuple[schema["Grade"].index]) > 80;
}

bool join_condition(Tuple tuple, AttributeNames schema) {
  return std::get<int>(tuple[schema["students.Grade"].index]) > 80;
}

int main() {
  AttributeNames attr1 = {
      {"Name", {0, STRING}}, {"Grade", {1, INTEGER}}, {"ID", {2, INTEGER}}};

  std::set<Tuple> tuples1 = {
      {"Bobby", 99, 1}, {"Selsabeel", 88, 2}, {"Moses", 77, 3}};
  Relation relation1("students", std::move(tuples1), attr1);

  AttributeNames attr2 = {
      {"Species", {0, STRING}}, {"ID", {1, INTEGER}}, {"Fruit", {2, STRING}}};

  std::set<Tuple> tuples2 = {
      {"chimp", 1, "banana"},
      {"orangutan", 2, "watermelon"},
      {"ape", 3, "coconut"},
  };
  Relation relation2("monkey", std::move(tuples2), attr2);

  std::set<Tuple> tuples3 = {{"Ann", 99, 1}, {"Soonwoo", 88, 2}};
  Relation relation3("other students", std::move(tuples3), attr1);

  std::cout << std::endl << "Relation test" << std::endl;
  std::cout << relation1.toString() << std::endl;
  std::cout << relation2.toString() << std::endl;
  std::cout << relation3.toString() << std::endl;

  std::cout << std::endl << "Selection test grade > 80" << std::endl;
  std::cout << Algebra::selection(relation1, condition).toString();

  std::cout << std::endl << "Projection test name and id" << std::endl;
  std::cout << Algebra::projection(relation1, {"Name", "ID"}).toString();

  std::cout << std::endl
            << "Projection and Selection test grade > 80 and name and id"
            << std::endl;
  std::cout << Algebra::projection(Algebra::selection(relation1, condition),
                                   {"Name", "ID"})
                   .toString();

  std::cout << std::endl << "Rename attribute test" << std::endl;
  std::cout << Algebra::rename(relation1, "Name", "SOONWOO").toString();

  std::cout << std::endl << "Rename relation test" << std::endl;
  std::cout << Algebra::rename(relation1, "SOONWOO").toString();

  std::cout << std::endl << "Times test" << std::endl;
  std::cout << Algebra::times(relation1, relation2).toString();

  std::cout << std::endl << "Join test grade > 80" << std::endl;
  std::cout << Algebra::join(relation1, relation2, join_condition).toString();

  std::cout << std::endl << "Onion test" << std::endl;
  std::cout << Algebra::onion(relation1, relation3).toString();

  return 0;
}
