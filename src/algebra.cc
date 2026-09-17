#include "algebra.h"
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

  return Relation(relation.get_name(), std::move(rows), std::move(new_schema));
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
  return Relation(relation.get_name(), std::move(rows), std::move(new_schema));
}

Relation Algebra::times(const Relation &rel_1, const Relation &rel_2,
                        std::string modifier) {
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
    attribute_names[name_2 + "." + attr.first] = {.index = attr.second.index +
                                                           length_of_attrs_1,
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

  return Relation(name_1 + " " + modifier + " " + name_2, std::move(rows),
                  std::move(attribute_names));
}

Relation Algebra::join(const Relation &rel_1, const Relation &rel_2,
                       std::function<bool(Tuple, AttributeNames)> predicate) {
  return selection(times(rel_1, rel_2, "JOIN"), predicate);
}

Relation Algebra::intersect(const Relation &rel_1, const Relation &rel_2) {
  if (!compare_schemas(rel_1, rel_2))
    throw std::runtime_error("SCHEMA'S NOT COMPATIBLE");

  auto rows = rel_1.get_rows();
  std::set<Tuple> new_rows;

  for (const auto &tuple : rows) {
    if (rel_2.get_rows().find(tuple) != rel_2.get_rows().end()) {
      new_rows.insert(tuple);
    }
  }

  return Relation(rel_1.get_name() + " INTERSECT " + rel_2.get_name(),
                  std::move(new_rows), rel_1.get_schema());
}

Relation Algebra::minus(const Relation &rel_1, const Relation &rel_2) {
  if (!compare_schemas(rel_1, rel_2))
    throw std::runtime_error("SCHEMA'S NOT COMPATIBLE");

  auto rows = rel_1.get_rows();
  std::set<Tuple> new_rows;

  for (const auto &tuple : rows) {
    if (rel_2.get_rows().find(tuple) == rel_2.get_rows().end()) {
      new_rows.insert(tuple);
    }
  }

  return Relation(rel_1.get_name() + " MINUS " + rel_2.get_name(),
                  std::move(new_rows), rel_1.get_schema());
}

Relation Algebra::onion(const Relation &rel_1, const Relation &rel_2) {
  if (!compare_schemas(rel_1, rel_2))
    throw std::runtime_error("SCHEMA'S NOT COMPATIBLE");

  auto rows = rel_1.get_rows();
  const auto &rows_2 = rel_2.get_rows();
  rows.insert(rows_2.begin(), rows_2.end());

  return Relation(rel_1.get_name() + " UNION " + rel_2.get_name(),
                  std::move(rows), rel_1.get_schema());
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
