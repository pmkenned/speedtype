CC = gcc
CPPFLAGS =
CFLAGS = -Wall -Wextra -std=c89
LDFLAGS =
LDLIBS = -lncurses

TARGET = speedtype
BUILD_DIR =./build
DEBUG_DIR = $(BUILD_DIR)/debug
RELEASE_DIR = $(BUILD_DIR)/release
SRC_DIR = ./src
SRC = $(wildcard $(SRC_DIR)/*.c)
ASM_R = $(SRC:$(SRC_DIR)/%.c=$(RELEASE_DIR)/%.s)
ASM_D = $(SRC:$(SRC_DIR)/%.c=$(DEBUG_DIR)/%.s)
OBJ_R = $(SRC:$(SRC_DIR)/%.c=$(RELEASE_DIR)/%.o)
DEP_R = $(OBJ_R:%.o=%.d)
OBJ_D = $(SRC:$(SRC_DIR)/%.c=$(DEBUG_DIR)/%.o)
DEP_D = $(OBJ_D:%.o=%.d)

# TODO: test

.PHONY: all run asm clean
.PHONY: release  debug
.PHONY: run_release run_debug
.PHONY: asm_r asm_d

all: release

release: CFLAGS += -O3
release: CPPFLAGS += -DNDEBUG
release: $(RELEASE_DIR)/$(TARGET)

debug: CFLAGS += -DDEBUG -g
debug: $(DEBUG_DIR)/$(TARGET)

run: run_release

run_release: release
	$(RELEASE_DIR)/$(TARGET)

run_debug: debug
	$(DEBUG_DIR)/$(TARGET)

asm: asm_r

asm_r: CFLAGS += -O3
asm_r: CPPFLAGS += -DNDEBUG
asm_r: $(ASM_R)

asm_d: CFLAGS += -DDEBUG -g
asm_d: $(ASM_D)

clean:
	rm -rf $(BUILD_DIR)

-include $(DEP_R)
-include $(DEP_D)

# TODO: find a way if this can be simplified

# RELEASE:

$(RELEASE_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(RELEASE_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -MMD -o $@ $<

$(RELEASE_DIR)/$(TARGET): $(OBJ_R)
	mkdir -p $(RELEASE_DIR)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(RELEASE_DIR)/%.s: $(SRC_DIR)/%.c
	mkdir -p $(RELEASE_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -S -o $@ $<

# DEBUG:

$(DEBUG_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(DEBUG_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -MMD -o $@ $<

$(DEBUG_DIR)/$(TARGET): $(OBJ_D)
	mkdir -p $(DEBUG_DIR)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(DEBUG_DIR)/%.s: $(SRC_DIR)/%.c
	mkdir -p $(DEBUG_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -S -o $@ $<
