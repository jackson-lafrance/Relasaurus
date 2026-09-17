#ifndef ALGEBRA_H
#define ALGEBRA_H

#include "relations.h"
#include <functional>
#include <initializer_list>

class Algebra {
public:
  static Relation
  selection(const Relation &relation,
            std::function<bool(Tuple, AttributeNames)> predicate);
  static Relation projection(const Relation &relation,
                             std::initializer_list<std::string> attributes);

  static Relation rename(const Relation &relation, std::string new_name);
  static Relation rename(const Relation &relation, std::string old_name,
                         std::string new_name);

  static Relation times(const Relation &rel_1, const Relation &rel_2, std::string modifier = "TIMES");
  static Relation join(const Relation &rel_1, const Relation &rel_2,
                       std::function<bool(Tuple, AttributeNames)> predicate);

  static Relation onion(const Relation &rel_1, const Relation &rel_2);
  static Relation intersect(const Relation &rel_1, const Relation &rel_2);
  static Relation minus(const Relation &rel_1, const Relation &rel_2);
  
private:
  static bool compare_schemas(const Relation &rel_1, const Relation &rel_2);
};

#endif // ALGEBRA_H
