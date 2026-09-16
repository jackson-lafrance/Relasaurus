#include "../include/algebra.h"
#include <stdexcept>

Relation Algebra::selection(Relation &relation,
                            std::function<bool(Tuple)> predicate) {
  Relation out(relation.get_name(), relation.get_schema());

  for (const auto &tuple : relation.get_rows()) {
    if (predicate(tuple)) {
      out.insert_row(tuple);
    }
  }
  return out;
}

Relation Algebra::projection(Relation &relation,
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
