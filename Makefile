CC=gcc
CFLAGS=-I/opt/homebrew/opt/openssl@3/include
LDFLAGS=-L/opt/homebrew/opt/openssl@3/lib -lssl -lcrypto

# List of executables to build
EXECUTABLES=main create_test secure_test insecure_test find_gcd

all: $(EXECUTABLES)

main: main.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

create_test: create_test.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

secure_test: secure_test.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

insecure_test: insecure_test.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

find_gcd: find_gcd.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f $(EXECUTABLES)