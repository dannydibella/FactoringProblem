#include <stdio.h>
#include <stdlib.h>
#include <openssl/bn.h>
#include <openssl/rand.h>
#include <string.h>

#define K 256
#define N 1000

void generate_odd_256bit_integer(BIGNUM *num) {
    if (!BN_rand(num, K, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ODD)) {
        fprintf(stderr, "Random number generation failed\n");
        exit(EXIT_FAILURE);
    }
}

int main() {
    printf("\nBEGIN FACTORING\n\n");

    BIGNUM **nums = (BIGNUM **)malloc(N * sizeof(BIGNUM *));
    BIGNUM ***gcds = (BIGNUM ***)malloc(N * sizeof(BIGNUM **));
    BN_CTX *bn_ctx = BN_CTX_new();

    if (!nums || !gcds || !bn_ctx) {
        fprintf(stderr, "Allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // Generate N 256-bit numbers
    for (int i = 0; i < N; i++) {
        nums[i] = BN_new();
        generate_odd_256bit_integer(nums[i]);
    }

    // Allocate 2D array for GCDs
    for (int i = 0; i < N; i++) {
        gcds[i] = (BIGNUM **)malloc(N * sizeof(BIGNUM *));
        for (int j = 0; j < N; j++) {
            gcds[i][j] = NULL; // Initialize with NULL
        }
    }

    // Compute GCDs and store them
    for (int i = 0; i < N; i++) {
        for (int j = i + 1; j < N; j++) { // Start from i+1 to avoid duplicate calculations and self GCD
            gcds[i][j] = BN_new();
            if (!BN_gcd(gcds[i][j], nums[i], nums[j], bn_ctx)) {
                fprintf(stderr, "BN_gcd failed\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    // Example of how to use GCDs here...
    // Remember to free allocated BIGNUMs and arrays

    for (int i = 0; i < N; i++) {
        for (int j = i + 1; j < N; j++) {
            char *gcd_str = BN_bn2dec(gcds[i][j]);
            if (gcd_str) {
                printf("GCD of %d and %d is: %s\n", i, j, gcd_str);
                OPENSSL_free(gcd_str);
            }
        }
    }

    // Free memory
    for (int i = 0; i < N; i++) {
        BN_free(nums[i]);
        for (int j = i + 1; j < N; j++) {
            BN_free(gcds[i][j]);
        }
        free(gcds[i]);
    }
    free(nums);
    free(gcds);
    BN_CTX_free(bn_ctx);

    printf("COMPLETE\n\n");

    return 0;
}