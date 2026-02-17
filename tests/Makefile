CC = gcc

SRC_DIR = ../src
CFLAGS = -I$(SRC_DIR) -DLINUX -D_GNU_SOURCE -DUNIT_TEST -Wall -Wextra -O0 -g
DEPFLAGS = -MMD -MP

TEST_SRCS = test_framework.c test_runner.c test_basic.c test_parse.c test_strings.c
APP_SRCS = $(SRC_DIR)/midi.c $(SRC_DIR)/load_map.c $(SRC_DIR)/arch.c $(SRC_DIR)/log.c

OBJS = $(TEST_SRCS:.c=.o) $(notdir $(APP_SRCS:.c=.o))
TARGET = unit_tests

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

midi.o: $(SRC_DIR)/midi.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

load_map.o: $(SRC_DIR)/load_map.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

arch.o: $(SRC_DIR)/arch.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

log.o: $(SRC_DIR)/log.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f *.o *.d $(TARGET)

-include $(OBJS:.o=.d)
