#!/bin/sh

set -u

app=${1:-./build/relasaurus}
temp_dir=$(mktemp -d "${TMPDIR:-/tmp}/relasaurus-e2e.XXXXXX") || exit 1
trap 'rm -rf "$temp_dir"' EXIT HUP INT TERM

passed=0
failed=0
stdout_file=
stderr_file=

run_repl() {
  name=$1
  input=$2
  stdout_file="$temp_dir/stdout"
  stderr_file="$temp_dir/stderr"

  printf '1\n%s' "$input" | "$app" >"$stdout_file" 2>"$stderr_file"
  status=$?
  if [ "$status" -ne 0 ]; then
    echo "[ FAIL ] $name"
    echo "         REPL exited with status $status"
    failed=$((failed + 1))
    return 1
  fi

  current_test=$name
  return 0
}

expect_contains() {
  file=$1
  expected=$2
  message=$3

  if ! grep -Fq "$expected" "$file"; then
    echo "[ FAIL ] $current_test"
    echo "         $message"
    echo "         expected: $expected"
    echo "         stdout:"
    sed 's/^/           /' "$stdout_file"
    echo "         stderr:"
    sed 's/^/           /' "$stderr_file"
    failed=$((failed + 1))
    return 1
  fi

  return 0
}

pass_test() {
  echo "[ PASS ] $current_test"
  passed=$((passed + 1))
}

echo "=== end-to-end tests ==="

name="complete programs flow through the REPL"
echo "[ RUN  ] $name"
input="relation Students(ID=number,Name=string,Grade=number){1,'Ann Marie',92;2,Bob,75;};
relation Clubs(ID=number,Club=string){1,Chess;2,Music;};
project{Students.Name,Clubs.Club}(select{Students.Grade>=80}(Students@{Students.ID=Clubs.ID}Clubs));
:q
"
if run_repl "$name" "$input" &&
   expect_contains "$stdout_file" "Choose a mode:" "missing mode menu" &&
   expect_contains "$stdout_file" "Students JOIN Clubs" "missing query relation name" &&
   expect_contains "$stdout_file" "Students.Name" "missing projected student column" &&
   expect_contains "$stdout_file" "Clubs.Club" "missing projected club column" &&
   expect_contains "$stdout_file" "Ann Marie" "missing selected student" &&
   expect_contains "$stdout_file" "Chess" "missing joined club"; then
  pass_test
fi

name="state persists across definitions insertions and queries"
echo "[ RUN  ] $name"
input="relation Numbers(Value=number){1;};
insert Numbers {2;3;};
select{Value>1}(Numbers);
:q
"
if run_repl "$name" "$input" &&
   expect_contains "$stdout_file" "Numbers" "missing queried relation" &&
   expect_contains "$stdout_file" "2" "missing first inserted value" &&
   expect_contains "$stdout_file" "3" "missing second inserted value"; then
  pass_test
fi

name="multiline statements and comments are accumulated"
echo "[ RUN  ] $name"
input="relation R(ID=number){
1;
/* a comment
continued on another line */
2;
};
R;
:q
"
if run_repl "$name" "$input" &&
   expect_contains "$stdout_file" "... " "missing continuation prompt" &&
   expect_contains "$stdout_file" "R" "missing multiline relation result" &&
   expect_contains "$stdout_file" "1" "missing first multiline tuple" &&
   expect_contains "$stdout_file" "2" "missing second multiline tuple"; then
  pass_test
fi

name="lexer diagnostics reach stderr and the REPL recovers"
echo "[ RUN  ] $name"
input="_bad;
relation Good(ID=number){1;};
Good;
:q
"
if run_repl "$name" "$input" &&
   expect_contains "$stderr_file" "lexer error at 1:1" "missing lexer diagnostic location" &&
   expect_contains "$stderr_file" "There's so many characters to choose from" "missing lexer diagnostic message" &&
   expect_contains "$stdout_file" "Good" "REPL did not process input after lexer error"; then
  pass_test
fi

name="syntax diagnostics reach stderr and the REPL recovers"
echo "[ RUN  ] $name"
input="project{}(Missing);
relation Good(ID=number){1;};
Good;
:q
"
if run_repl "$name" "$input" &&
   expect_contains "$stderr_file" "syntax error at 1:9" "missing parser diagnostic location" &&
   expect_contains "$stderr_file" "Expected an attribute" "missing parser diagnostic message" &&
   expect_contains "$stdout_file" "Good" "REPL did not process input after syntax error"; then
  pass_test
fi

name="runtime diagnostics reach stderr and the REPL recovers"
echo "[ RUN  ] $name"
input="Missing;
relation Good(ID=number){1;};
Good;
:q
"
if run_repl "$name" "$input" &&
   expect_contains "$stderr_file" "runtime error:" "missing runtime error category" &&
   expect_contains "$stderr_file" "RELATION DOES NOT EXIST: Missing" "missing runtime error detail" &&
   expect_contains "$stdout_file" "Good" "REPL did not process input after runtime error"; then
  pass_test
fi

echo "=== end-to-end tests: $passed passed, $failed failed ==="
[ "$failed" -eq 0 ]
