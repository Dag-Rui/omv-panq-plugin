# Copyright (C) 2020 Guillaume Valadon <guillaume@valadon.net>

CFLAGS=-Werror -Iinclude/
LD_FLAGS=-Llib/ -ldl
CC=gcc

# Ignore errors
.IGNORE: clean

# Project management targets
all: panq

panq: src/*.c
	$(CC) -o $@ $^ $(LD_FLAGS) $(CFLAGS)

capability: panq
	setcap cap_sys_rawio+ep panq

clean:
	@rm panq
