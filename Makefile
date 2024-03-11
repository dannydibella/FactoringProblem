CC=gcc
CFLAGS=-I/opt/homebrew/opt/openssl@3/include
LDFLAGS=-L/opt/homebrew/opt/openssl@3/lib -lssl -lcrypto

# List of executables to build
EXECUTABLES=main create_test test_main

all: $(EXECUTABLES)

main: main.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

create_test: create_test.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

test_main: test_main.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f $(EXECUTABLES)