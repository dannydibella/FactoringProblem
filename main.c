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

// Helper method for factorization
char* factorize(unsigned int n) {
    char* result = (char*)malloc(256 * sizeof(char)); // Allocate enough space
    char factor[64];
    result[0] = '\0'; // Initialize to empty string

    // Factorize for odd factors starting from 3
    for (unsigned int i = 3; i <= n; i += 2) {
        if (n % i == 0) {
            if (strlen(result) > 0) {
                strcat(result, ","); // Add comma between factors
            }
            sprintf(factor, "%u", i); // Format factor
            strcat(result, factor);
            while (n % i == 0) {
                n /= i; // Reduce n by dividing it by the current factor
            }
        }
    }

    return result;
}

int main() {
    printf("\nBEGIN FACTORING\n\n");

    BIGNUM **nums = (BIGNUM **)malloc(N * sizeof(BIGNUM *));
    BIGNUM ***gcds = (BIGNUM ***)malloc(N * sizeof(BIGNUM **));
    char ***factorizations = (char ***)malloc(N * sizeof(char **));
    BN_CTX *bn_ctx = BN_CTX_new();

    if (!nums || !gcds || !bn_ctx || !factorizations) {
        fprintf(stderr, "Allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // Generate N 256-bit numbers
    for (int i = 0; i < N; i++) {
        nums[i] = BN_new();
        generate_odd_256bit_integer(nums[i]);
    }

    // Allocate 2D arrays for GCDs and factorizations
    for (int i = 0; i < N; i++) {
        gcds[i] = (BIGNUM **)malloc(N * sizeof(BIGNUM *));
        factorizations[i] = (char **)malloc(N * sizeof(char *));
        for (int j = 0; j < N; j++) {
            gcds[i][j] = NULL; // Initialize with NULL
            factorizations[i][j] = NULL; // Initialize with NULL
        }
    }

    // Compute GCDs, convert to unsigned int, and factorize
    for (int i = 0; i < N; i++) {
        for (int j = i + 1; j < N; j++) { // Avoid duplicate calculations and self GCD
            gcds[i][j] = BN_new();
            if (!BN_gcd(gcds[i][j], nums[i], nums[j], bn_ctx)) {
                fprintf(stderr, "BN_gcd failed\n");
                exit(EXIT_FAILURE);
            }
            // Convert BIGNUM to unsigned int and factorize
            unsigned int gcd_uint = BN_get_word(gcds[i][j]);
            if (gcd_uint != (unsigned int)-1) { // Check conversion success
                factorizations[i][j] = factorize(gcd_uint);
            }
        }
    }

    // Print factorizations
    for (int i = 0; i < N; i++) {
        for (int j = i + 1; j < N; j++) {
            if (factorizations[i][j] != NULL) {
                printf("Factorization of GCD(%d, %d) is: %s\n", i, j, factorizations[i][j]);
            }
        }
    }

    // Free memory
    for (int i = 0; i < N; i++) {
        BN_free(nums[i]);
        for (int j = i + 1; j < N; j++) {
            BN_free(gcds[i][j]);
            free(factorizations[i][j]);
        }
        free(gcds[i]);
        free(factorizations[i]);
    }
    free(nums);
    free(gcds);
    free(factorizations);
    BN_CTX_free(bn_ctx);

    printf("COMPLETE\n\n");

    return 0;
}