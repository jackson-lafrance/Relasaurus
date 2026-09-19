CXX := c++

CPPFLAGS := -Iinclude
CXXFLAGS := -std=c++20 -Wall -Wextra -Wpedantic -MMD -MP

TARGET := build/relasaurus

SOURCES := src/algebra.cc src/relation.cc src/schema.cc
TEST_SOURCES := test/algebra_test.cc

OBJECTS := $(SOURCES:src/%.cc=build/%.o)
TEST_OBJECTS := $(TEST_SOURCES:test/%.cc=build/test/%.o)

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJECTS) $(TEST_OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

build/%.o: src/%.cc
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

build/test/%.o: test/%.cc
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf build

-include $(OBJECTS:.o=.d) $(TEST_OBJECTS:.o=.d)
