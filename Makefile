CC = clang
CFLAGS = -Wall -Wformat -O3 -mavx -mavx2

build: cstem.c
	$(CC) $(CFLAGS) -g -o cstem cstem.c

build-release: cstem.c
	$(CC) $(CFLAGS) -o cstem cstem.c

run: build
	@./cstem

gdb: build
	@gdb -ex "set confirm off" -ex run cstem

valgrind: build
	@valgrind --track-origins=yes --leak-check=full ./cstem

.PHONY: clean

clean:
	rm -f cstem

dump: build
	@objdump \
		--visualize-jumps=color \
		--disassembler-color=on \
		--source \
		--source-comment="▌ " \
		--no-addresses \
		--no-show-raw-insn \
		--line-numbers \
		cstem | less -R
