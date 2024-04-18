#include <stdio.h>
#include <stdlib.h>
#include <openssl/bn.h>
#include <openssl/rand.h>
#include <time.h>

#define K 256  // Bit length of the numbers to generate
#define N 1000 // Number of numbers to generate
#define G 1000000 // Number of prime numbers in provided text files

// Function Prototypes
int load_primes(char *filename, BIGNUM **primes, int max_count);
void generate_insecure_numbers(char *output_file, BIGNUM **primes, int count);

int main() {
    BIGNUM **primes = malloc(G * sizeof(BIGNUM *));
    if (!primes) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    int count = load_primes("primes2.txt", primes, G);
    if (count < G) {
        fprintf(stderr, "Could not load enough primes\n");
        return 1;
    }

    generate_insecure_numbers("insecure_numbers.txt", primes, count);

    // Cleanup
    for (int i = 0; i < count; i++) BN_free(primes[i]);
    free(primes);

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

void generate_insecure_numbers(char *output_file, BIGNUM **primes, int count) {
    FILE *file = fopen(output_file, "w");
    if (!file) {
        fprintf(stderr, "Failed to open %s\n", output_file);
        return;
    }

    BN_CTX *ctx = BN_CTX_new();
    srand(time(NULL)); // Seed for random number generation

    BIGNUM *current = BN_new();
    BN_copy(current, primes[0]);
    int index = 1;

    // Generate the first number until it reaches approximately K bits
    while (BN_num_bits(current) < K) {
        BN_mul(current, current, primes[index++], ctx);
    }

    // Output the first number
    char *num_str = BN_bn2dec(current);
    fprintf(file, "%s\n", num_str);
    OPENSSL_free(num_str);

    // Generate next numbers using previous prime factor
    for (int i = 1; i < index; i++) {
        BIGNUM *num = BN_new();
        BN_copy(num, primes[i - 1]);

        while (BN_num_bits(num) < K) {
            int rand_index = rand() % count;
            BN_mul(num, num, primes[rand_index], ctx);
        }

        num_str = BN_bn2dec(num);
        fprintf(file, "%s\n", num_str);
        OPENSSL_free(num_str);
        BN_free(num);
    }

    // Generate the rest of the numbers
    for (int i = index; i < N; i++) {
        BIGNUM *num = BN_new();
        BN_one(num);

        while (BN_num_bits(num) < K) {
            int rand_index = rand() % count;
            BN_mul(num, num, primes[rand_index], ctx);
        }

        num_str = BN_bn2dec(num);
        fprintf(file, "%s\n", num_str);
        OPENSSL_free(num_str);
        BN_free(num);
    }

    BN_free(current);
    BN_CTX_free(ctx);
    fclose(file);
}