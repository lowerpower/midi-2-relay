#include "test_framework.h"

#include "config.h"
#include "arch.h"

static void test_hexascii_to_bin(void)
{
    U8 out[8];
    int len;

    memset(out, 0, sizeof(out));
    len = hexascii_2_bin(out, (int)sizeof(out), "AB:CD:01");
    TEST_ASSERT_INT_EQ(3, len);
    TEST_ASSERT_INT_EQ(0xAB, out[0]);
    TEST_ASSERT_INT_EQ(0xCD, out[1]);
    TEST_ASSERT_INT_EQ(0x01, out[2]);
}

static void test_str_char_replace(void)
{
    char s[32];
    strcpy(s, "a-b-c");
    TEST_ASSERT_INT_EQ(2, str_char_replace(s, '-', '_'));
    TEST_ASSERT_STR_EQ("a_b_c", s);
}

static void test_readln_from_buffer(void)
{
    char input[] = "# comment\nalpha\r\nbeta\n";
    char line[32];
    char *next;

    next = readln_from_a_buffer(input, line, (int)sizeof(line));
    TEST_ASSERT_STR_EQ("alpha", line);
    TEST_ASSERT_TRUE(next != NULL);

    next = readln_from_a_buffer(next, line, (int)sizeof(line));
    TEST_ASSERT_STR_EQ("beta", line);
    TEST_ASSERT_TRUE(next == NULL);
}

void run_test_strings(void)
{
    test_hexascii_to_bin();
    test_str_char_replace();
    test_readln_from_buffer();
}
