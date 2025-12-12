CC       = gcc
CFLAGS   = -O2

LIBS     = -lm

TARGET_PREP   = prep_group1
TARGET_SEARCH = search_group1

SRC_PREP   = src/prep_group1.c
SRC_SEARCH = src/search_group1.c

all: $(TARGET_PREP) $(TARGET_SEARCH)

$(TARGET_PREP): $(SRC_PREP)
	$(CC) $(CFLAGS) -o $(TARGET_PREP) $(SRC_PREP) $(LIBS)

$(TARGET_SEARCH): $(SRC_SEARCH)
	$(CC) $(CFLAGS) -o $(TARGET_SEARCH) $(SRC_SEARCH) $(LIBS)

clean:
	rm -f $(TARGET_PREP) $(TARGET_SEARCH) *.o

.PHONY: all clean