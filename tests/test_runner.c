#include "test_framework.h"

void run_test_basic(void);
void run_test_parse(void);
void run_test_strings(void);

int main(void)
{
    run_test_basic();
    run_test_parse();
    run_test_strings();

    if (g_tests_failed == 0) {
        printf("PASS: %d assertions\n", g_tests_run);
        return 0;
    }

    printf("FAIL: %d of %d assertions failed\n", g_tests_failed, g_tests_run);
    return 1;
}
