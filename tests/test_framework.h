#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern int g_tests_run;
extern int g_tests_failed;

#define TEST_ASSERT_TRUE(expr) \
    do { \
        g_tests_run++; \
        if (!(expr)) { \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); \
            g_tests_failed++; \
        } \
    } while (0)

#define TEST_ASSERT_INT_EQ(expected, actual) \
    do { \
        int _exp = (expected); \
        int _act = (actual); \
        g_tests_run++; \
        if (_exp != _act) { \
            fprintf(stderr, "FAIL %s:%d: expected %d got %d\n", __FILE__, __LINE__, _exp, _act); \
            g_tests_failed++; \
        } \
    } while (0)

#define TEST_ASSERT_STR_EQ(expected, actual) \
    do { \
        const char *_exp = (expected); \
        const char *_act = (actual); \
        g_tests_run++; \
        if (strcmp(_exp, _act) != 0) { \
            fprintf(stderr, "FAIL %s:%d: expected '%s' got '%s'\n", __FILE__, __LINE__, _exp, _act); \
            g_tests_failed++; \
        } \
    } while (0)

#endif
