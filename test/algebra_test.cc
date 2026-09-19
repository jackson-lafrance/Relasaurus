#include "algebra.h"
#include <cassert>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

Tuple student_row(std::string name, double grade, double id) {
  return {std::move(name), grade, id};
}

Tuple monkey_row(std::string species, double id, std::string fruit) {
  return {std::move(species), id, std::move(fruit)};
}

Predicate grade_above_80(const Schema &schema) {
  const std::size_t grade_index = schema.index_of("Grade");

  return [grade_index](const Tuple &tuple, const Schema &) {
    return grade_index != Schema::npos &&
           std::get<double>(tuple.at(grade_index)) > 80.0;
  };
}

Predicate matching_high_grade(const Schema &student_schema,
                               const Schema &monkey_schema) {
  const std::size_t grade_index = student_schema.index_of("Grade");
  const std::size_t student_id_index = student_schema.index_of("ID");
  const std::size_t monkey_source_index = monkey_schema.index_of("ID");
  const std::size_t monkey_id_index =
      monkey_source_index == Schema::npos
          ? Schema::npos
          : student_schema.columns().size() + monkey_source_index;

  return [grade_index, student_id_index,
          monkey_id_index](const Tuple &tuple, const Schema &) {
    return grade_index != Schema::npos &&
           student_id_index != Schema::npos &&
           monkey_id_index != Schema::npos &&
           std::get<double>(tuple.at(grade_index)) > 80.0 &&
           std::get<double>(tuple.at(student_id_index)) ==
               std::get<double>(tuple.at(monkey_id_index));
  };
}

int main() {
  Schema student_schema({
      {"Name", STRING}, {"Grade", NUMBER}, {"ID", NUMBER}});
  Schema monkey_schema({
      {"Species", STRING}, {"ID", NUMBER}, {"Fruit", STRING}});

  assert(student_schema.index_of("Name") == 0);
  assert(student_schema.index_of("Missing") == Schema::npos);

  Relation students("students", student_schema);
  students.insert_row(student_row("Bobby", 99.0, 1.0));
  students.insert_row(student_row("Selsabeel", 88.0, 2.0));
  students.insert_row(student_row("Moses", 77.0, 3.0));

  Relation monkeys("monkey", monkey_schema);
  monkeys.insert_row(monkey_row("chimp", 1.0, "banana"));
  monkeys.insert_row(monkey_row("orangutan", 2.0, "watermelon"));
  monkeys.insert_row(monkey_row("ape", 3.0, "coconut"));

  Relation other_students("other students", student_schema);
  other_students.insert_row(student_row("Ann", 99.0, 1.0));
  other_students.insert_row(student_row("Soonwoo", 88.0, 2.0));
  other_students.insert_row(student_row("Moses", 77.0, 3.0));

  assert(students.tuples().size() == 3);

  const Relation selected =
      Algebra::selection(students, grade_above_80(students.schema()));
  assert(selected.name() == "students");
  assert(selected.tuples().size() == 2);

  const Relation projected =
      Algebra::projection(students, std::vector<std::string>{"Name", "ID"});
  assert(projected.schema().columns().size() == 2);
  assert(projected.schema().columns()[0].name == "Name");
  assert(projected.schema().columns()[1].name == "ID");
  assert(projected.tuples().size() == 3);

  const Relation selected_projected = Algebra::projection(
      selected, std::vector<std::string>{"Name", "ID"});
  assert(selected_projected.tuples().size() == 2);

  const Relation renamed_attribute =
      Algebra::rename(students, "Grade", "Score");
  assert(renamed_attribute.schema().index_of("Grade") == Schema::npos);
  assert(renamed_attribute.schema().index_of("Score") == 1);
  assert(renamed_attribute.schema().columns()[1].type == NUMBER);

  const Relation renamed_relation = Algebra::rename(students, "pupils");
  assert(renamed_relation.name() == "pupils");
  assert(renamed_relation.tuples().size() == students.tuples().size());

  const Relation product = Algebra::times(students, monkeys);
  assert(product.name() == "students TIMES monkey");
  assert(product.tuples().size() == 9);
  assert(product.schema().index_of("students.Name") == 0);
  assert(product.schema().index_of("monkey.Species") == 3);
  assert(product.schema().index_of("monkey.Fruit") == 5);

  const Relation joined =
      Algebra::join(students, monkeys,
                    matching_high_grade(students.schema(), monkeys.schema()));
  assert(joined.name() == "students JOIN monkey");
  assert(joined.tuples().size() == 2);

  const Relation unioned = Algebra::onion(students, other_students);
  assert(unioned.name() == "students UNION other students");
  assert(unioned.tuples().size() == 5);

  const Relation intersected = Algebra::intersect(students, other_students);
  assert(intersected.tuples().size() == 1);

  const Relation difference = Algebra::minus(students, other_students);
  assert(difference.tuples().size() == 2);

  bool threw = false;
  try {
    Algebra::projection(students, std::vector<std::string>{"Missing"});
  } catch (const std::runtime_error &) {
    threw = true;
  }
  assert(threw);

  return 0;
}
