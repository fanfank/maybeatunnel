CC=gcc
OPENSSL_PATH=$(shell pwd)/../../third/openssl
OPENSSL_LIB=$(OPENSSL_PATH)/libssl.a $(OPENSSL_PATH)/libcrypto.a

LIBS=-lcrypto -lpthread -lcurl -lrt -ldl -lm -lssl

CFLAGS=-g -Wall -I$(OPENSSL_PATH)/include

all: openssl

openssl: openssl.c
	$(CC) $(CFLAGS) -o $@ openssl.c $(OPENSSL_LIB) $(LIBS)

