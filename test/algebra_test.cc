#include "algebra.h"
#include "test_support.h"
#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using test_support::Suite;
using test_support::expect;
using test_support::expect_throws;

Tuple student_row(std::string name, double grade, double id) {
  return {std::move(name), grade, id};
}

Tuple monkey_row(std::string species, double id, std::string fruit) {
  return {std::move(species), id, std::move(fruit)};
}

struct AlgebraFixture {
  Schema student_schema;
  Schema monkey_schema;
  Relation students;
  Relation monkeys;
  Relation other_students;

  AlgebraFixture()
      : student_schema(
            {{"Name", STRING}, {"Grade", NUMBER}, {"ID", NUMBER}}),
        monkey_schema(
            {{"Species", STRING}, {"ID", NUMBER}, {"Fruit", STRING}}),
        students("students", student_schema), monkeys("monkey", monkey_schema),
        other_students("other students", student_schema) {
    students.insert_row(student_row("Bobby", 99.0, 1.0));
    students.insert_row(student_row("Selsabeel", 88.0, 2.0));
    students.insert_row(student_row("Moses", 77.0, 3.0));

    monkeys.insert_row(monkey_row("chimp", 1.0, "banana"));
    monkeys.insert_row(monkey_row("orangutan", 2.0, "watermelon"));
    monkeys.insert_row(monkey_row("ape", 3.0, "coconut"));

    other_students.insert_row(student_row("Ann", 99.0, 1.0));
    other_students.insert_row(student_row("Soonwoo", 88.0, 2.0));
    other_students.insert_row(student_row("Moses", 77.0, 3.0));
  }
};

Predicate grade_above_80(const Schema &schema) {
  const std::size_t grade_index = schema.index_of("Grade");

  return [grade_index](const Tuple &tuple, const Schema &,
                       const std::string &) {
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
          monkey_id_index](const Tuple &tuple, const Schema &,
                           const std::string &) {
    return grade_index != Schema::npos &&
           student_id_index != Schema::npos &&
           monkey_id_index != Schema::npos &&
           std::get<double>(tuple.at(grade_index)) > 80.0 &&
           std::get<double>(tuple.at(student_id_index)) ==
               std::get<double>(tuple.at(monkey_id_index));
  };
}

} // namespace

int main() {
  Suite suite("relational algebra tests");

  suite.run("schema resolves known and missing attributes", [] {
    const Schema schema(
        {{"Name", STRING}, {"Grade", NUMBER}, {"ID", NUMBER}});

    expect(schema.index_of("Name") == 0, "Name should be the first column");
    expect(schema.index_of("Grade") == 1,
           "Grade should be the second column");
    expect(schema.index_of("Missing") == Schema::npos,
           "missing attributes should return Schema::npos");
  });

  suite.run("relations collapse duplicate tuples", [] {
    AlgebraFixture fixture;
    fixture.students.insert_row(student_row("Bobby", 99.0, 1.0));

    expect(fixture.students.tuples().size() == 3,
           "inserting a duplicate should not increase relation size");
  });

  suite.run("relations reject rows with the wrong shape or types", [] {
    const Schema schema(
        {{"Name", STRING}, {"Grade", NUMBER}, {"ID", NUMBER}});
    Relation students("students", schema);

    expect_throws<std::runtime_error>(
        [&] { students.insert_row(Tuple{std::string{"Bobby"}}); },
        "a short tuple should be rejected");
    expect_throws<std::runtime_error>(
        [&] {
          students.insert_row(
              Tuple{std::string{"Bobby"}, std::string{"A"}, 1.0});
        },
        "a tuple with the wrong value type should be rejected");
  });

  suite.run("selection keeps only matching tuples", [] {
    AlgebraFixture fixture;
    const Relation selected = Algebra::selection(
        fixture.students, grade_above_80(fixture.students.schema()));

    expect(selected.name() == "students",
           "selection should preserve the relation name");
    expect(selected.tuples().size() == 2,
           "selection should keep the two grades above 80");
  });

  suite.run("projection keeps requested columns and collapses duplicate tuples",
            [] {
              AlgebraFixture fixture;
              const Relation projected = Algebra::projection(
                  fixture.students, std::vector<std::string>{"Name", "ID"});

              expect(projected.schema().columns().size() == 2,
                     "projection should contain two columns");
              expect(projected.schema().columns()[0].name == "Name",
                     "Name should be the first projected column");
              expect(projected.schema().columns()[1].name == "ID",
                     "ID should be the second projected column");
              expect(projected.tuples().size() == 3,
                     "projection should preserve distinct rows");

              const Schema score_schema({{"Name", STRING}, {"Score", NUMBER}});
              Relation scores("scores", score_schema);
              scores.insert_row(Tuple{std::string{"Alex"}, 90.0});
              scores.insert_row(Tuple{std::string{"Alex"}, 80.0});

              const Relation names = Algebra::projection(
                  scores, std::vector<std::string>{"Name"});
              expect(names.tuples().size() == 1,
                     "duplicate projected tuples should collapse");
            });

  suite.run("projection rejects unknown attributes", [] {
    AlgebraFixture fixture;
    expect_throws<std::runtime_error>(
        [&] {
          Algebra::projection(fixture.students,
                              std::vector<std::string>{"Missing"});
        },
        "projection should reject an unknown attribute");
  });

  suite.run("rename changes attribute and relation names", [] {
    AlgebraFixture fixture;
    const Relation renamed_attribute =
        Algebra::rename(fixture.students, "Grade", "Score");

    expect(renamed_attribute.schema().index_of("Grade") == Schema::npos,
           "the old attribute name should disappear");
    expect(renamed_attribute.schema().index_of("Score") == 1,
           "the new attribute name should keep the same position");
    expect(renamed_attribute.schema().columns()[1].type == NUMBER,
           "renaming should preserve the attribute type");

    const Relation renamed_relation =
        Algebra::rename(fixture.students, "pupils");
    expect(renamed_relation.name() == "pupils",
           "relation rename should change the name");
    expect(renamed_relation.tuples().size() == fixture.students.tuples().size(),
           "relation rename should preserve tuples");
  });

  suite.run("attribute rename rejects missing and duplicate names", [] {
    AlgebraFixture fixture;

    expect_throws<std::runtime_error>(
        [&] { Algebra::rename(fixture.students, "Missing", "Score"); },
        "rename should reject a missing source attribute");
    expect_throws<std::runtime_error>(
        [&] { Algebra::rename(fixture.students, "Grade", "Name"); },
        "rename should reject an existing destination attribute");
  });

  suite.run("times creates a Cartesian product with qualified columns", [] {
    AlgebraFixture fixture;
    const Relation product = Algebra::times(fixture.students, fixture.monkeys);

    expect(product.name() == "students TIMES monkey",
           "times should describe both input relations");
    expect(product.tuples().size() == 9,
           "three times three rows should produce nine rows");
    expect(product.schema().index_of("students.Name") == 0,
           "left columns should be qualified with the left relation name");
    expect(product.schema().index_of("monkey.Species") == 3,
           "right columns should follow the left columns");
    expect(product.schema().index_of("monkey.Fruit") == 5,
           "all right columns should be qualified");
  });

  suite.run("join filters the Cartesian product with its predicate", [] {
    AlgebraFixture fixture;
    const Relation joined = Algebra::join(
        fixture.students, fixture.monkeys,
        matching_high_grade(fixture.students.schema(),
                            fixture.monkeys.schema()));

    expect(joined.name() == "students JOIN monkey",
           "join should describe both input relations");
    expect(joined.tuples().size() == 2,
           "join should keep two matching high-grade rows");
  });

  suite.run("union combines compatible relations without duplicates", [] {
    AlgebraFixture fixture;
    const Relation unioned =
        Algebra::onion(fixture.students, fixture.other_students);

    expect(unioned.name() == "students UNION other students",
           "union should describe both input relations");
    expect(unioned.tuples().size() == 5,
           "the shared tuple should appear only once in the union");
  });

  suite.run("intersection keeps tuples shared by both relations", [] {
    AlgebraFixture fixture;
    const Relation intersected =
        Algebra::intersect(fixture.students, fixture.other_students);

    expect(intersected.tuples().size() == 1,
           "only one tuple should be shared by both relations");
  });

  suite.run("minus removes tuples found in the right relation", [] {
    AlgebraFixture fixture;
    const Relation difference =
        Algebra::minus(fixture.students, fixture.other_students);

    expect(difference.tuples().size() == 2,
           "minus should remove the one shared tuple");
  });

  suite.run("set operations reject incompatible schemas", [] {
    AlgebraFixture fixture;
    const Schema incompatible_schema({{"Name", STRING}});
    const Relation incompatible("incompatible", incompatible_schema);

    expect_throws<std::runtime_error>(
        [&] { Algebra::onion(fixture.students, incompatible); },
        "union should reject incompatible schemas");
    expect_throws<std::runtime_error>(
        [&] { Algebra::intersect(fixture.students, incompatible); },
        "intersection should reject incompatible schemas");
    expect_throws<std::runtime_error>(
        [&] { Algebra::minus(fixture.students, incompatible); },
        "minus should reject incompatible schemas");
  });

  return suite.finish();
}
