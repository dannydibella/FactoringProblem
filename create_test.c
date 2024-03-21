#include <stdio.h>
#include <openssl/bn.h>
#include <openssl/rand.h>

#define K 100
#define N 100

// Prototype for the new number generation function
BIGNUM **generate_random_odd_BIGNUMs(int count, int bits);

int main() {
    FILE *file;
    BN_CTX *ctx;
    BIGNUM **nums;

    // Initialize OpenSSL BIGNUM context
    ctx = BN_CTX_new();
    if (ctx == NULL) {
        fprintf(stderr, "Failed to create BN_CTX\n");
        return 1;
    }

    // Open file to write numbers
    file = fopen("numbers.txt", "w");
    if (file == NULL) {
        fprintf(stderr, "Failed to open file for writing\n");
        BN_CTX_free(ctx);
        return 1;
    }

    // Generate the numbers
    nums = generate_random_odd_BIGNUMs(N, K);

    // Write numbers to file and free them
    for (int i = 0; i < N; i++) {
        char *num_str = BN_bn2dec(nums[i]);
        if (num_str != NULL) {
            fprintf(file, "%s\n", num_str);
            OPENSSL_free(num_str);
        }
        BN_free(nums[i]);
    }

    // Free the array of BIGNUM pointers
    free(nums);

    // Cleanup and close file
    fclose(file);
    BN_CTX_free(ctx);

    printf("Generated %d numbers of %d bits each in numbers.txt\n", N, K);
    return 0;
}

BIGNUM **generate_random_odd_BIGNUMs(int count, int bits) {
    BIGNUM **nums = malloc(count * sizeof(BIGNUM*));
    if (nums == NULL) {
        fprintf(stderr, "Failed to allocate memory for BIGNUM pointers\n");
        return NULL;
    }

    for (int i = 0; i < count; i++) {
        nums[i] = BN_new();
        if (nums[i] == NULL || !BN_rand(nums[i], bits, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ODD)) {
            fprintf(stderr, "Failed to generate random number\n");
            // Cleanup partially allocated BIGNUMs
            for (int j = 0; j <= i; j++) {
                BN_free(nums[j]);
            }
            free(nums);
            return NULL;
        }
    }

    return nums;
}