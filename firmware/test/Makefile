CC = cc
CFLAGS = -Wall -Wextra -std=c11 -g -I../src -I../lib/cfx/include

BUILD = build
TESTS = test_entropy

all: $(TESTS:%=$(BUILD)/%) run

$(BUILD)/test_entropy: test_entropy.c ../src/entropy/entropy.c | $(BUILD)
	$(CC) $(CFLAGS) $^ -lm -o $@

$(BUILD):
	mkdir -p $@

run: $(TESTS:%=$(BUILD)/%)
	@for t in $^; do ./$$t || exit 1; done

clean:
	rm -rf $(BUILD)

.PHONY: all run clean
