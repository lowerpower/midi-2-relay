#include "test_framework.h"

#include "config.h"
#include "midi.h"

int set_relay_map(MIDI *midi, int bit, int state);
int Bitmask_2_String(MIDI *midi);
int support_midi_byte_type(MIDI *midi, char type);
void process_midi_byte(MIDI *midi, char byte);

static void test_set_relay_map_bounds(void)
{
    MIDI midi;
    memset(&midi, 0, sizeof(midi));

    TEST_ASSERT_INT_EQ(0, set_relay_map(&midi, 0, 1));
    TEST_ASSERT_INT_EQ(0, set_relay_map(&midi, -1, 1));
    TEST_ASSERT_INT_EQ(0, set_relay_map(&midi, 257, 1));
    TEST_ASSERT_INT_EQ(1, set_relay_map(&midi, 1, 1));
    TEST_ASSERT_INT_EQ(1, (midi.bitmask[0] & 0x01));
}

static void test_bitmask_to_string(void)
{
    MIDI midi;
    memset(&midi, 0, sizeof(midi));

    TEST_ASSERT_INT_EQ(1, set_relay_map(&midi, 1, 1));
    TEST_ASSERT_INT_EQ(1, set_relay_map(&midi, 16, 1));
    TEST_ASSERT_INT_EQ(1, Bitmask_2_String(&midi));
    TEST_ASSERT_INT_EQ(64, (int)strlen(midi.bit_string));
    TEST_ASSERT_STR_EQ("8001", midi.bit_string + 60);
}

static void test_supported_midi_types(void)
{
    MIDI midi;
    memset(&midi, 0, sizeof(midi));

    TEST_ASSERT_INT_EQ(1, support_midi_byte_type(&midi, (char)0x80));
    TEST_ASSERT_INT_EQ(1, support_midi_byte_type(&midi, (char)0x90));
    TEST_ASSERT_INT_EQ(1, support_midi_byte_type(&midi, (char)0xF0));
    TEST_ASSERT_INT_EQ(0, support_midi_byte_type(&midi, (char)0xC0));
}

static void test_process_midi_byte_state_machine(void)
{
    MIDI midi;
    memset(&midi, 0, sizeof(midi));

    process_midi_byte(&midi, (char)0x91);
    TEST_ASSERT_INT_EQ(1, midi.counter);

    process_midi_byte(&midi, (char)60);
    TEST_ASSERT_INT_EQ(2, midi.counter);
    TEST_ASSERT_INT_EQ(60, midi.data1);

    process_midi_byte(&midi, (char)100);
    TEST_ASSERT_INT_EQ(1, midi.counter);
    TEST_ASSERT_INT_EQ(100, midi.data2);
}

void run_test_basic(void)
{
    test_set_relay_map_bounds();
    test_bitmask_to_string();
    test_supported_midi_types();
    test_process_midi_byte_state_machine();
}
