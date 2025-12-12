CC       = gcc
CFLAGS   = -O3 -Wall
SRC_DIR  = src

TARGET_PREP   = prep_group1
TARGET_SEARCH = search_group1

SRC_PREP   = $(SRC_DIR)/prep_group1.c
SRC_SEARCH = $(SRC_DIR)/search_group1.c

all: $(TARGET_PREP) $(TARGET_SEARCH)

$(TARGET_PREP): $(SRC_PREP)
	$(CC) $(CFLAGS) -o $(TARGET_PREP) $(SRC_PREP)

$(TARGET_SEARCH): $(SRC_SEARCH)
	$(CC) $(CFLAGS) -o $(TARGET_SEARCH) $(SRC_SEARCH)

clean:
	rm -f $(TARGET_PREP) $(TARGET_SEARCH) index.bin *.o

.PHONY: all clean