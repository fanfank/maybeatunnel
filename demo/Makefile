CC=gcc
LIBS="-lcrypto"
SRCS:=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)
TARGETS=$(SRCS:.c=)

all: $(TARGETS)

$(TARGETS): %: %.c
	$(CC) $^ -o $@ $(LIBS)

clean:
	rm -rf $(OBJS) $(TARGETS)

.PHONY : clean
