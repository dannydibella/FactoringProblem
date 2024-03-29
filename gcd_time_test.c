#include <stdio.h>
#include <openssl/bn.h>
#include <time.h>

void generate_random_number(BIGNUM *num, int bits) {
    if (!BN_rand(num, bits, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ANY)) {
        fprintf(stderr, "Error generating random number\n");
        exit(1);
    }
}

int main() {
    int k = 256000;
    int l = 256;

    // Initialize OpenSSL BIGNUM variables
    BIGNUM *a = BN_new();
    BIGNUM *b = BN_new();
    BIGNUM *gcd = BN_new();

    // Seed the random number generator
    srand(time(NULL));

    // Generate two random numbers of k and l bits
    generate_random_number(a, k);
    generate_random_number(b, l);

    // Start timing
    clock_t start = clock();

    // Calculate the GCD of a and b
    BN_CTX *ctx = BN_CTX_new();
    if (!BN_gcd(gcd, a, b, ctx)) {
        fprintf(stderr, "Error calculating GCD\n");
        return 1;
    }
    BN_CTX_free(ctx);

    // Stop timing
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;

    // Print the GCD
    char *gcd_str = BN_bn2dec(gcd);
    printf("GCD: %s\n", gcd_str);
    OPENSSL_free(gcd_str);

    // Print the time spent
    printf("Time taken to calculate GCD: %.6f seconds\n", time_spent);

    // Free the allocated memory
    BN_free(a);
    BN_free(b);
    BN_free(gcd);

    return 0;
}