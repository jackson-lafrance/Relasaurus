CXX := c++

CPPFLAGS := -Iinclude
CXXFLAGS := -std=c++20 -Wall -Wextra -Wpedantic -MMD -MP

SOURCES := src/algebra.cc src/relation.cc src/schema.cc src/lexer.cc
OBJECTS := $(SOURCES:src/%.cc=build/%.o)

TEST_SOURCES := test/algebra_test.cc test/lexer_test.cc
TEST_OBJECTS := $(TEST_SOURCES:test/%.cc=build/test/%.o)

ALGEBRA_TEST_TARGET := build/algebra_test
LEXER_TEST_TARGET := build/lexer_test
TEST_TARGETS := $(ALGEBRA_TEST_TARGET) $(LEXER_TEST_TARGET)

.PHONY: all test run clean

all: $(TEST_TARGETS)

$(ALGEBRA_TEST_TARGET): build/algebra.o build/relation.o build/schema.o \
	build/test/algebra_test.o
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(LEXER_TEST_TARGET): build/lexer.o build/test/lexer_test.o
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

build/%.o: src/%.cc
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

build/test/%.o: test/%.cc
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

test: $(TEST_TARGETS)
	@status=0; for target in $(TEST_TARGETS); do \
		echo "==> $$target"; \
		./$$target || status=1; \
		echo; \
	done; \
	exit $$status

run: test

clean:
	rm -rf build

-include $(OBJECTS:.o=.d) $(TEST_OBJECTS:.o=.d)
