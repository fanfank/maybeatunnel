CC=gcc
UV_PATH=$(shell pwd)/../../third/libuv
UV_LIB=$(UV_PATH)/.libs/libuv.a
LIBS=-lcrypto -lpthread -lcurl -lrt -ldl -lm

CFLAGS=-g -Wall -I$(UV_PATH)/include

all: proxy

proxy: proxy.c
	$(CC) $(CFLAGS) -o $@ $^ $(UV_LIB) $(LIBS)

