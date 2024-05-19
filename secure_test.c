#include <stdio.h>
#include <stdlib.h>
#include <openssl/bn.h>
#include <openssl/rand.h>
#include <time.h>

#define K 256  // Bit length of the numbers to generate
#define N 10000 // Number of numbers to generate
#define G 1000000 // Number of prime numbers in provided text files

// Function Prototypes
int load_primes(char *filename, BIGNUM **primes, int max_count);
void generate_secure_numbers(char *output_file, BIGNUM **primes1, int count1, BIGNUM **primes2, int count2);

int main() {
    BIGNUM **primes1 = malloc(G * sizeof(BIGNUM *));
    BIGNUM **primes2 = malloc(G * sizeof(BIGNUM *));
    if (!primes1 || !primes2) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    int count1 = load_primes("primes1.txt", primes1, G);
    int count2 = load_primes("primes2.txt", primes2, G);

    if (count1 < G || count2 < G) {
        fprintf(stderr, "Could not load enough primes\n");
        return 1;
    }

    generate_secure_numbers("secure_numbers.txt", primes1, count1, primes2, count2);

    // Cleanup
    for (int i = 0; i < count1; i++) BN_free(primes1[i]);
    for (int i = 0; i < count2; i++) BN_free(primes2[i]);
    free(primes1);
    free(primes2);

    return 0;
}

int load_primes(char *filename, BIGNUM **primes, int max_count) {
    FILE *file = fopen(filename, "r");
    char line[1024];
    int count = 0;

    if (!file) {
        fprintf(stderr, "Failed to open %s\n", filename);
        return 0;
    }

    while (fgets(line, sizeof(line), file) && count < max_count) {
        primes[count] = BN_new();
        BN_dec2bn(&primes[count], line);
        count++;
    }

    fclose(file);
    return count;
}

void generate_secure_numbers(char *output_file, BIGNUM **primes1, int count1, BIGNUM **primes2, int count2) {
    FILE *file = fopen(output_file, "w");
    if (!file) {
        fprintf(stderr, "Failed to open %s\n", output_file);
        return;
    }

    BN_CTX *ctx = BN_CTX_new();
    srand(time(NULL)); // Seed for random number generation

    for (int i = 0; i < N; i++) {
        BIGNUM *result = BN_new();
        BN_copy(result, primes1[i]);

        // Multiply by random primes from primes2.txt until it reaches the desired bit length
        while (BN_num_bits(result) < K) {
            int index = rand() % count2;
            BN_mul(result, result, primes2[index], ctx);
        }

        char *num_str = BN_bn2dec(result);
        fprintf(file, "%s\n", num_str);
        OPENSSL_free(num_str);
        BN_free(result);
    }

    BN_CTX_free(ctx);
    fclose(file);
}