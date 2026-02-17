#include "test_framework.h"

#include <unistd.h>

#include "config.h"
#include "midi.h"
#include "load_map.h"

static void write_map_file(const char *path, const char *body)
{
    FILE *fp = fopen(path, "w");
    TEST_ASSERT_TRUE(fp != NULL);
    if (fp != NULL) {
        fputs(body, fp);
        fclose(fp);
    }
}

static void test_load_map_parses_valid_lines(void)
{
    MIDI midi;
    char path[] = "/tmp/midi_map_testXXXXXX";
    int fd = mkstemp(path);
    TEST_ASSERT_TRUE(fd >= 0);
    if (fd < 0) return;
    close(fd);

    memset(&midi, 0, sizeof(midi));
    strncpy(midi.map_file, path, MAX_PATH - 1);
    midi.map_file[MAX_PATH - 1] = 0;

    write_map_file(
        path,
        "# note channel relay\n"
        "60 1 5\n"
        "61 2 127\n"
        "62 3 255\n"
        "63 4 0\n"     /* invalid relay */
        "200 1 4\n"    /* invalid note */
        "64 20 7\n");  /* invalid channel */

    TEST_ASSERT_INT_EQ(1, load_map(&midi));
    TEST_ASSERT_INT_EQ(5, midi.map[60][1]);
    TEST_ASSERT_INT_EQ(127, midi.map[61][2]);
    TEST_ASSERT_INT_EQ(255, midi.map[62][3]);
    TEST_ASSERT_INT_EQ(0, midi.map[63][4]);

    unlink(path);
}

static void test_load_map_if_new_reload(void)
{
    MIDI midi;
    char path[] = "/tmp/midi_map_reloadXXXXXX";
    int fd = mkstemp(path);
    TEST_ASSERT_TRUE(fd >= 0);
    if (fd < 0) return;
    close(fd);

    memset(&midi, 0, sizeof(midi));
    strncpy(midi.map_file, path, MAX_PATH - 1);
    midi.map_file[MAX_PATH - 1] = 0;

    write_map_file(path, "60 1 8\n");
    TEST_ASSERT_INT_EQ(1, load_map(&midi));
    TEST_ASSERT_INT_EQ(8, midi.map[60][1]);

    sleep(1);
    write_map_file(path, "60 1 12\n");
    TEST_ASSERT_INT_EQ(1, load_map_if_new(&midi));
    TEST_ASSERT_INT_EQ(12, midi.map[60][1]);

    unlink(path);
}

void run_test_parse(void)
{
    test_load_map_parses_valid_lines();
    test_load_map_if_new_reload();
}
