#ifndef ALGEBRA_H
#define ALGEBRA_H

#include "relations.h"
#include <functional>
#include <initializer_list>

class Algebra {
public:
  static Relation selection(Relation relation,
                            std::function<bool(Tuple)> predicate);
  static Relation projection(Relation relation,
                             std::initializer_list<std::string> attributes);

  static Relation rename(Relation rel_1, std::string old_name,
                         std::string new_name);

  static Relation times(Relation rel_1, Relation rel_2);
  static Relation join(Relation rel_1, std::function<bool(Tuple)> predicate);

  static Relation onion(Relation rel_1, Relation rel_2);
  static Relation intersect(Relation rel_1, Relation rel_2);
  static Relation minus(Relation rel_1, Relation rel_2);
};

#endif // ALGEBRA_H
