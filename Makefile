CC      = gcc
CFLAGS  = -O2
LIBS    = -lm

# グループ番号
GRP_ID  = 1

# 実行ファイル名 (prep_1, search_1)
TARGET_PREP   = prep_$(GRP_ID)
TARGET_SEARCH = search_$(GRP_ID)

# ソースファイル名 (src/prep_1.c, src/search_1.c)
SRC_PREP   = src/prep_$(GRP_ID).c
SRC_SEARCH = src/search_$(GRP_ID).c

all: $(TARGET_PREP) $(TARGET_SEARCH)

$(TARGET_PREP): $(SRC_PREP)
	$(CC) $(CFLAGS) -o $(TARGET_PREP) $(SRC_PREP) $(LIBS)

$(TARGET_SEARCH): $(SRC_SEARCH)
	$(CC) $(CFLAGS) -o $(TARGET_SEARCH) $(SRC_SEARCH) $(LIBS)

# テスト実行
test: all
	./$(TARGET_PREP) db_data > index_$(GRP_ID)
	./$(TARGET_SEARCH) query_data index_$(GRP_ID) > result_$(GRP_ID)

clean:
	rm -f $(TARGET_PREP) $(TARGET_SEARCH) index_* result_*

.PHONY: all clean test