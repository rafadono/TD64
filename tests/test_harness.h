#ifndef TEST_HARNESS_H
#define TEST_HARNESS_H
#include <stdio.h>
#include <math.h>

// Minimal, dependency-free assert-and-count harness — no framework needed
// for this small a suite. Each test .c file has its own main() and its own
// static counters (fine: each compiles to its own binary, see tests/README.md).
static int th_run = 0;
static int th_failed = 0;

#define CHECK(cond) do { \
    th_run++; \
    if (!(cond)) { \
        th_failed++; \
        printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
    } \
} while (0)

#define CHECK_NEAR(a, b, eps) do { \
    th_run++; \
    double _a = (double)(a), _b = (double)(b), _e = (double)(eps); \
    if (fabs(_a - _b) > _e) { \
        th_failed++; \
        printf("  FAIL %s:%d: %s ~= %s (got %g, expected %g)\n", __FILE__, __LINE__, #a, #b, _a, _b); \
    } \
} while (0)

#define SECTION(name) printf("-- %s --\n", (name))

#define SUMMARY_AND_RETURN() do { \
    printf("\n%d/%d checks passed\n", th_run - th_failed, th_run); \
    return th_failed > 0 ? 1 : 0; \
} while (0)

#endif // TEST_HARNESS_H
