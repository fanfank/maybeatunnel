CC=gcc
LIBS="-lcrypto"
SRCS:=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)
TARGETS=$(SRCS:.c=)

all: $(TARGETS)

$(TARGETS): %: %.c
	$(CC) $^ -g -o $@ $(LIBS)

echo_server:
	cd echo_server && make

clean:
	rm -rf $(OBJS) $(TARGETS)

.PHONY : clean
.PHONY : echo_server
