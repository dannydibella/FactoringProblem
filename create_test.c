#include <stdio.h>
#include <openssl/bn.h>
#include <openssl/rand.h>

#define N 1000
#define K 256

int main() {
    FILE *file;
    BIGNUM *num;
    BN_CTX *ctx;

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

    for (int i = 0; i < N; i++) {
        // Generate a random K-bit, odd BIGNUM
        num = BN_new();
        if (num == NULL || !BN_rand(num, K, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ODD)) {
            fprintf(stderr, "Failed to generate random number\n");
            BN_free(num);
            fclose(file);
            BN_CTX_free(ctx);
            return 1;
        }

        // Convert BIGNUM to decimal string and write to file
        char *num_str = BN_bn2dec(num);
        if (num_str == NULL) {
            fprintf(stderr, "Failed to convert BIGNUM to string\n");
            BN_free(num);
            fclose(file);
            BN_CTX_free(ctx);
            return 1;
        }
        fprintf(file, "%s\n", num_str);

        // Free allocated memory
        OPENSSL_free(num_str);
        BN_free(num);
    }

    // Cleanup and close file
    fclose(file);
    BN_CTX_free(ctx);

    printf("Generated %d numbers of %d bits each in numbers.txt\n", N, K);
    return 0;
}