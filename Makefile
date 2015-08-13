CSRCS := $(wildcard *.c)

COBJS := $(CSRCS:%.c=%.o)

BIN := converter

CFLAGS := -Wall -Wextra -std=gnu99
LDFLAGS :=

.PHONY: all check clean

all: $(BIN)

check:
	$(CC) $(CFLAGS) -fsyntax-only $(CSRCS)

clean:
	-@rm -vf $(COBJS) $(BIN)

$(BIN): $(COBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
