// kz_test.hpp -- a minimal test harness: plain programs that exit non-zero on failure.
//
// Values used in tests are unit-test values.  They are not research conditions and must
// not be quoted as experiment results (CONDITIONS_POLICY).
#pragma once

#include <cstdint>
#include <cstdio>
#include <sstream>
#include <string>
#include <type_traits>

#include "kz/core/status.hpp"

namespace kztest {

struct State {
  int checks = 0;
  int failures = 0;
};

inline State& state() {
  static State s;
  return s;
}

inline void fail(const char* file, int line, const std::string& what) {
  std::fprintf(stderr, "%s:%d: FAILED: %s\n", file, line, what.c_str());
  ++state().failures;
}

template <class T>
std::string show(const T& v) {
  std::ostringstream o;
  if constexpr (std::is_integral<T>::value && sizeof(T) == 1)
    o << static_cast<int>(v);
  else if constexpr (std::is_enum<T>::value)
    o << static_cast<long long>(v);
  else
    o << v;
  return o.str();
}

inline int finish(const char* name) {
  State& s = state();
  if (s.failures != 0) {
    std::fprintf(stderr, "%s: %d of %d checks FAILED\n", name, s.failures, s.checks);
    return 1;
  }
  std::printf("%s: %d checks passed\n", name, s.checks);
  return 0;
}

}  // namespace kztest

#define KZ_CHECK(cond)                                                      \
  do {                                                                      \
    ++kztest::state().checks;                                               \
    if (!(cond)) kztest::fail(__FILE__, __LINE__, #cond);                   \
  } while (0)

#define KZ_CHECK_EQ(a, b)                                                                              \
  do {                                                                                                 \
    ++kztest::state().checks;                                                                          \
    const auto kz_a_ = (a);                                                                            \
    const auto kz_b_ = (b);                                                                            \
    if (!(kz_a_ == kz_b_))                                                                             \
      kztest::fail(__FILE__, __LINE__, std::string(#a " == " #b "  [") + kztest::show(kz_a_) + " vs " + kztest::show(kz_b_) + "]"); \
  } while (0)

#define KZ_CHECK_OK(expr)                                                                       \
  do {                                                                                          \
    ++kztest::state().checks;                                                                   \
    const kz::Status kz_s_ = (expr);                                                            \
    if (!kz_s_.ok()) kztest::fail(__FILE__, __LINE__, std::string(#expr " -> ") + kz_s_.describe()); \
  } while (0)

// The statement must fail with the given error code.
#define KZ_CHECK_ERR(expr, expected)                                                                            \
  do {                                                                                                      \
    ++kztest::state().checks;                                                                               \
    const kz::Status kz_s_ = (expr);                                                                        \
    if (kz_s_.code != (expected))                                                                               \
      kztest::fail(__FILE__, __LINE__, std::string(#expr " expected " #expected " but got ") + kz_s_.describe()); \
  } while (0)
