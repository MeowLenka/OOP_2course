#pragma once

#include <cmath>
#include <exception>
#include <iostream>

inline int test_failures = 0;

#define CHECK(condition)                                                \
    do                                                                  \
    {                                                                   \
        if (!(condition))                                               \
        {                                                               \
            std::cerr << __FILE__ << ':' << __LINE__                    \
                      << ": НЕ ВЫПОЛНЕНО: " #condition << '\n';         \
            ++test_failures;                                            \
        }                                                               \
    } while (false)

#define CHECK_NEAR(actual, expected, eps)                               \
    do                                                                  \
    {                                                                   \
        const double a_ = (actual);                                     \
        const double e_ = (expected);                                   \
        if (!(std::fabs(a_ - e_) <= (eps)))                             \
        {                                                               \
            std::cerr << __FILE__ << ':' << __LINE__                    \
                      << ": " #actual " = " << a_                       \
                      << ", ожидалось " << e_ << '\n';                  \
            ++test_failures;                                            \
        }                                                               \
    } while (false)

#define CHECK_THROWS(expression)                                        \
    do                                                                  \
    {                                                                   \
        bool thrown_ = false;                                           \
        try                                                             \
        {                                                               \
            (void)(expression);                                         \
        }                                                               \
        catch (const std::exception &)                                  \
        {                                                               \
            thrown_ = true;                                             \
        }                                                               \
        if (!thrown_)                                                   \
        {                                                               \
            std::cerr << __FILE__ << ':' << __LINE__                    \
                      << ": ожидалось исключение: " #expression << '\n';\
            ++test_failures;                                            \
        }                                                               \
    } while (false)

template <typename Function>
void run_test(const char *name, Function function)
{
    try
    {
        function();
    }
    catch (const std::exception &error)
    {
        std::cerr << name << ": неожиданное исключение: " << error.what() << '\n';
        ++test_failures;
    }
}

inline int finish_tests(const char *name)
{
    if (test_failures == 0)
    {
        std::cout << name << ": OK\n";
        return 0;
    }
    std::cerr << name << ": число ошибок: " << test_failures << '\n';
    return 1;
}
