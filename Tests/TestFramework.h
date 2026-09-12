#pragma once
#include <cstdio>
#include <cmath>
#include <cstring>

// =====================================================================
//  ћини-фреймворк дл€ юнит-тестов без внешних зависимостей.
//  ¬озможности:
//    - CHECK(cond)           Ч провал теста, продолжить выполнение
//    - REQUIRE(cond)         Ч провал теста, прервать текущий тест
//    - CHECK_EQ(a, b)        Ч сравнить и напечатать значени€
//    - CHECK_NEAR(a, b, eps) Ч |a - b| <= eps
//    - RUN_TEST(fn)          Ч вызвать тест, посчитать success/fail
// =====================================================================

namespace TestFW {

    inline int& Passed() { static int v = 0; return v; }
    inline int& Failed() { static int v = 0; return v; }
    inline const char*& CurrentTest() { static const char* v = ""; return v; }

    struct TestGuard {
        bool aborted = false;
        ~TestGuard() = default;
    };

    // “екущий статус Ч прерывать ли текущий тест после REQUIRE.
    inline bool& AbortCurrent() { static bool v = false; return v; }

    inline void ResetAbort() { AbortCurrent() = false; }

} // namespace TestFW

#define TEST_CASE(name) \
    void name()

#define RUN_TEST(fn) do {                                        \
    TestFW::CurrentTest() = #fn;                                 \
    TestFW::ResetAbort();                                        \
    int __passedBefore = TestFW::Passed();                       \
    int __failedBefore = TestFW::Failed();                       \
    fn();                                                        \
    if (TestFW::Failed() == __failedBefore) {                    \
        TestFW::Passed()++;                                      \
        std::printf("  [OK]   %s\n", #fn);                       \
    } else {                                                     \
        std::printf("  [FAIL] %s\n", #fn);                       \
    }                                                            \
} while(0)

#define CHECK(cond) do {                                         \
    if (!(cond)) {                                               \
        std::printf("    CHECK failed: %s\n", #cond);            \
        std::printf("      at %s:%d\n", __FILE__, __LINE__);     \
        TestFW::Failed()++;                                      \
    }                                                            \
} while(0)

#define REQUIRE(cond) do {                                       \
    if (!(cond)) {                                               \
        std::printf("    REQUIRE failed: %s\n", #cond);          \
        std::printf("      at %s:%d (test aborted)\n",           \
                    __FILE__, __LINE__);                         \
        TestFW::Failed()++;                                      \
        return;                                                  \
    }                                                            \
} while(0)

#define CHECK_EQ(a, b) do {                                      \
    auto __a = (a); auto __b = (b);                              \
    if (!(__a == __b)) {                                         \
        std::printf("    CHECK_EQ failed: %s == %s\n", #a, #b);  \
        std::printf("      left  = %lld\n", (long long)__a);     \
        std::printf("      right = %lld\n", (long long)__b);     \
        std::printf("      at %s:%d\n", __FILE__, __LINE__);     \
        TestFW::Failed()++;                                      \
    }                                                            \
} while(0)

#define CHECK_NEAR(a, b, eps) do {                               \
    double __a = (double)(a); double __b = (double)(b);          \
    double __e = (double)(eps);                                  \
    if (std::fabs(__a - __b) > __e) {                            \
        std::printf("    CHECK_NEAR failed: |%s - %s| <= %s\n",  \
                    #a, #b, #eps);                               \
        std::printf("      left  = %.6f\n", __a);                \
        std::printf("      right = %.6f\n", __b);                \
        std::printf("      eps   = %.6f\n", __e);                \
        std::printf("      at %s:%d\n", __FILE__, __LINE__);     \
        TestFW::Failed()++;                                      \
    }                                                            \
} while(0)

#define CHECK_TRUE(cond)  CHECK((cond))
#define CHECK_FALSE(cond) CHECK(!(cond))