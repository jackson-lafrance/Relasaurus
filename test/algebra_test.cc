#include "algebra.h"
#include "relations.h"
#include <iostream>

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

  std::set<Tuple> tuples3 = {
      {"Ann", 99, 1}, {"Soonwoo", 88, 2}, {"Moses", 77, 3}};
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

  std::cout << std::endl << "Intersect test" << std::endl;
  std::cout << Algebra::intersect(relation1, relation3).toString();

  std::cout << std::endl << "Minus test" << std::endl;
  std::cout << Algebra::minus(relation1, relation3).toString();

  return 0;
}
