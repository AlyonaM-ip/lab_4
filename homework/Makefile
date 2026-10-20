CC = gcc
CFLAGS = -Wall -Wextra -O2

all: my_alloc fault_types

my_alloc: my_alloc.c
	$(CC) $(CFLAGS) my_alloc.c -o my_alloc

fault_types: fault_types.c
	$(CC) $(CFLAGS) fault_types.c -o fault_types

clean:
	rm -f my_alloc fault_types testfile.bin perf.data*

