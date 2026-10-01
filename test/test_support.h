#ifndef TEST_SUPPORT_H
#define TEST_SUPPORT_H

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace test_support {

class Failure : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

inline void expect(bool condition, std::string_view message) {
  if (!condition)
    throw Failure(std::string(message));
}

template <typename Exception, typename Function>
void expect_throws(Function &&function, std::string_view message) {
  try {
    std::forward<Function>(function)();
  } catch (const Exception &) {
    return;
  } catch (const std::exception &error) {
    throw Failure(std::string(message) +
                  "; caught a different exception: " + error.what());
  } catch (...) {
    throw Failure(std::string(message) +
                  "; caught a different non-standard exception");
  }

  throw Failure(std::string(message) + "; no exception was thrown");
}

class Suite {
public:
  explicit Suite(std::string name) : name_(std::move(name)) {
    std::cout << "=== " << name_ << " ===\n";
  }

  template <typename Function>
  void run(std::string_view test_name, Function &&function) {
    std::cout << "[ RUN  ] " << test_name << '\n' << std::flush;

    try {
      std::forward<Function>(function)();
      ++passed_;
      std::cout << "[ PASS ] " << test_name << '\n';
    } catch (const std::exception &error) {
      ++failed_;
      std::cout << "[ FAIL ] " << test_name << "\n         " << error.what()
                << '\n';
    } catch (...) {
      ++failed_;
      std::cout << "[ FAIL ] " << test_name
                << "\n         unknown exception\n";
    }
  }

  int finish() const {
    std::cout << "=== " << name_ << ": " << passed_ << " passed, " << failed_
              << " failed ===\n";
    return failed_ == 0 ? 0 : 1;
  }

private:
  std::string name_;
  int passed_{0};
  int failed_{0};
};

} // namespace test_support

#endif // TEST_SUPPORT_H
