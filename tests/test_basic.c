#include "test_framework.h"

#include "config.h"
#include "midi.h"

int set_relay_map(MIDI *midi, int bit, int state);
int Bitmask_2_String(MIDI *midi);
int support_midi_byte_type(MIDI *midi, char type);
void process_midi_byte(MIDI *midi, char byte);
int flush_relay_batch(MIDI *midi);
int expire_relays(MIDI *midi);

#if !defined(WIN32)
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

// Receive one "set <bits>" datagram (non-blocking, a short wait for loopback).
// Returns the length, or -1 if nothing arrived.
static int recv_set(int rx, char *buf, int len)
{
    int i, n;
    for (i = 0; i < 100; i++)
    {
        n = (int)recv(rx, buf, len - 1, MSG_DONTWAIT);
        if (n > 0) { buf[n] = 0; return n; }
        usleep(1000);
    }
    return -1;
}

// Regression for the max on-time safety limit: a relay change whose UDP send fails
// must stay pending and be retried, not be forgotten. Uses a deliberately invalid
// socket to make the send fail, then a real loopback socket for the retry.
static void test_failed_send_is_retried(void)
{
    MIDI midi;
    char buf[256];
    struct sockaddr_in rx_addr;
    socklen_t alen = sizeof(rx_addr);
    int rx, tx;

    memset(&midi, 0, sizeof(midi));
    rx = socket(AF_INET, SOCK_DGRAM, 0);
    tx = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&rx_addr, 0, sizeof(rx_addr));
    rx_addr.sin_family = AF_INET;
    rx_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    TEST_ASSERT_INT_EQ(0, bind(rx, (struct sockaddr *)&rx_addr, sizeof(rx_addr)));
    TEST_ASSERT_INT_EQ(0, getsockname(rx, (struct sockaddr *)&rx_addr, &alen));
    midi.target_addr = rx_addr;
    midi.max_note_ms = 1000;

    // 1. note on, send fails: still pending, not timed, nothing sent
    midi.soc = -1;
    TEST_ASSERT_INT_EQ(1, set_relay_map(&midi, 5, 1));
    midi.dirty = 1;
    TEST_ASSERT_TRUE(flush_relay_batch(&midi) < 0);
    TEST_ASSERT_INT_EQ(1, midi.dirty);
    TEST_ASSERT_INT_EQ(0, (int)midi.on_ms[5]);
    TEST_ASSERT_INT_EQ(1, midi.on_pending[5]);
    TEST_ASSERT_INT_EQ(-1, recv_set(rx, buf, sizeof(buf)));

    // 2. retry with a working socket: the on state goes out and the timer starts
    midi.soc = tx;
    TEST_ASSERT_TRUE(flush_relay_batch(&midi) > 0);
    TEST_ASSERT_INT_EQ(0, midi.dirty);
    TEST_ASSERT_TRUE(midi.on_ms[5] != 0);
    TEST_ASSERT_TRUE(recv_set(rx, buf, sizeof(buf)) > 0);
    TEST_ASSERT_STR_EQ("10", buf + strlen(buf) - 2);          // relay 5 on

    // 3. the P1 case: the relay hits its max on-time and the off send fails
    midi.on_ms[5] = 1;                                          // long ago
    expire_relays(&midi);
    TEST_ASSERT_INT_EQ(1, midi.dirty);
    TEST_ASSERT_INT_EQ(0, midi.bitmask[0] & 0x10);
    midi.soc = -1;
    TEST_ASSERT_TRUE(flush_relay_batch(&midi) < 0);
    TEST_ASSERT_INT_EQ(1, midi.dirty);                         // still owed an off
    TEST_ASSERT_INT_EQ(-1, recv_set(rx, buf, sizeof(buf)));

    // 4. the retry turns the relay off
    midi.soc = tx;
    TEST_ASSERT_TRUE(flush_relay_batch(&midi) > 0);
    TEST_ASSERT_INT_EQ(0, midi.dirty);
    TEST_ASSERT_TRUE(recv_set(rx, buf, sizeof(buf)) > 0);
    TEST_ASSERT_STR_EQ("00", buf + strlen(buf) - 2);          // relay 5 off

    close(rx);
    close(tx);
}
#endif

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
#if !defined(WIN32)
    test_failed_send_is_retried();
#endif
}
