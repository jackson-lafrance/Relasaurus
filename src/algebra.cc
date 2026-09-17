#include "../include/algebra.h"
#include <iostream>
#include <stdexcept>
#include <string>

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

Relation Algebra::rename(const Relation &relation, std::string new_name) {
  Relation out(new_name, relation.get_schema());

  for (const auto &tuple : relation.get_rows()) {
    out.insert_row(tuple);
  }

  return out;
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

  Relation out(relation.get_name(), new_schema);

  for (const auto &tuple : relation.get_rows()) {
    out.insert_row(tuple);
  }

  return out;
}

Relation Algebra::times(const Relation &rel_1, const Relation &rel_2) {
  AttributeNames attribute_names;

  int length_of_attrs_1 = 0;
  for (const auto &attr : rel_1.get_schema()) {
    length_of_attrs_1++;
    attribute_names[rel_1.get_name() + "." + attr.first] = attr.second;
  }

  for (const auto &attr : rel_2.get_schema()) {
    attribute_names[rel_2.get_name() + "." + attr.first] = {
        .index = attr.second.index + length_of_attrs_1,
        .type = attr.second.type};
  }

  Relation out =
      Relation(rel_1.get_name() + "_X_" + rel_2.get_name(), attribute_names);

  for (const auto &tuple_1 : rel_1.get_rows()) {
    for (const auto &tuple_2 : rel_2.get_rows()) {
      Tuple new_tuple;
      new_tuple.insert(new_tuple.end(), tuple_1.begin(), tuple_1.end());
      new_tuple.insert(new_tuple.end(), tuple_2.begin(), tuple_2.end());

      out.insert_row(new_tuple);
    }
  }

  return out;
}

Relation Algebra::join(const Relation &rel_1, const Relation &rel_2,
                       std::function<bool(Tuple, AttributeNames)> predicate) {
  Relation big_boy = times(rel_1, rel_2);
  return selection(big_boy, predicate);
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

  Relation relation1 = Relation(
      "students", {{"Bobby", 99, 1}, {"Selsabeel", 88, 2}, {"Moses", 77, 3}},
      attr1);

  AttributeNames attr2 = {
      {"Species", {0, STRING}}, {"ID", {1, INTEGER}}, {"Fruit", {2, STRING}}};

  Relation relation2 = Relation("monkey",
                                {
                                    {"chimp", 1, "banana"},
                                    {"orangutan", 2, "watermelon"},
                                    {"ape", 3, "coconut"},
                                },
                                attr2);

  std::cout << std::endl << "Relation test" << std::endl;
  std::cout << relation1.toString() << std::endl;
  std::cout << relation2.toString() << std::endl;

  std::cout << std::endl << "Selection test grade > 80" << std::endl;
  std::cout << Algebra::selection(relation1, condition).toString();

  std::cout << std::endl << "Projection test name and id" << std::endl;
  std::cout << Algebra::projection(relation1, {"Name", "ID"}).toString();

  std::cout << std::endl << "Projection and Selection test grade > 80 and name and id" << std::endl;
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

  return 0;
}
