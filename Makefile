CC = clang
CFLAGS = -Wall -Wformat -O3 -mavx -mavx2

build: cstem.c
	$(CC) $(CFLAGS) -o cstem cstem.c

build-debug: cstem.c
	$(CC) $(CFLAGS) -g -o cstem cstem.c

run: build
	@./cstem

gdb: build-debug
	@gdb -ex "set confirm off" -ex run cstem

valgrind: build-debug
	@valgrind --track-origins=yes --leak-check=full ./cstem

dump: build-debug
	@objdump \
		--visualize-jumps=color \
		--disassembler-color=on \
		--source \
		--source-comment="▌ " \
		--no-addresses \
		--no-show-raw-insn \
		--line-numbers \
		cstem | less -R

test: build
	@bloomon-challenge-tester --exec ./cstem

benchmark: build
	multitime -n100 -q -s0 -i "cat samples/1m.txt" ./cstem
