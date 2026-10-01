CXX := c++

CPPFLAGS := -Iinclude
OPTFLAGS ?= -O3 -DNDEBUG
CXXFLAGS := -std=c++20 $(OPTFLAGS) -Wall -Wextra -Wpedantic -MMD -MP

SOURCES := src/algebra.cc src/relation.cc src/schema.cc src/lexer.cc src/parser.cc src/interpreter.cc src/repl.cc
OBJECTS := $(SOURCES:src/%.cc=build/%.o)

TEST_SOURCES := test/algebra_test.cc test/lexer_test.cc test/parser_test.cc \
	test/interpreter_test.cc
TEST_OBJECTS := $(TEST_SOURCES:test/%.cc=build/test/%.o)

ALGEBRA_TEST_TARGET := build/algebra_test
LEXER_TEST_TARGET := build/lexer_test
PARSER_TEST_TARGET := build/parser_test
INTERPRETER_TEST_TARGET := build/interpreter_test
TEST_TARGETS := $(ALGEBRA_TEST_TARGET) $(LEXER_TEST_TARGET) \
	$(PARSER_TEST_TARGET) $(INTERPRETER_TEST_TARGET)
APP_TARGET := build/relasaurus

.PHONY: all test run clean

all: $(APP_TARGET) $(TEST_TARGETS)

$(APP_TARGET): main.cc $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) main.cc $(OBJECTS) \
		$(LDFLAGS) $(LDLIBS) -o $@

$(ALGEBRA_TEST_TARGET): build/algebra.o build/relation.o build/schema.o \
	build/test/algebra_test.o
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(LEXER_TEST_TARGET): build/lexer.o build/test/lexer_test.o
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(PARSER_TEST_TARGET): build/parser.o build/lexer.o build/test/parser_test.o
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(INTERPRETER_TEST_TARGET): build/interpreter.o build/parser.o build/lexer.o \
	build/algebra.o build/relation.o build/schema.o \
	build/test/interpreter_test.o
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

build/%.o: src/%.cc
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

build/test/%.o: test/%.cc
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

test: $(APP_TARGET) $(TEST_TARGETS)
	@status=0; for target in $(TEST_TARGETS); do \
		echo "==> $$target"; \
		./$$target || status=1; \
		echo; \
	done; \
	echo "==> test/end_to_end_test.sh"; \
	sh test/end_to_end_test.sh ./$(APP_TARGET) || status=1; \
	echo; \
	exit $$status

run: $(APP_TARGET)
	./$(APP_TARGET)

clean:
	rm -rf build

-include $(OBJECTS:.o=.d) $(TEST_OBJECTS:.o=.d)
