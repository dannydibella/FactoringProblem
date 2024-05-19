#include <stdio.h>
#include <openssl/bn.h>

int main() {
    // Initialize BIGNUM variables
    BIGNUM *a = BN_new();
    BIGNUM *b = BN_new();
    BIGNUM *gcd = BN_new();
    BN_CTX *ctx = BN_CTX_new();

    // Check if BIGNUM and BN_CTX are initialized properly
    if (a == NULL || b == NULL || gcd == NULL || ctx == NULL) {
        fprintf(stderr, "Error initializing BIGNUM or BN_CTX\n");
        return 1;
    }

    // Assign large numbers to a and b (example values)
    if (!BN_dec2bn(&a, "239812851035039") ||
        !BN_dec2bn(&b, "496008393037631")) {
        fprintf(stderr, "Error assigning values to BIGNUM\n");
        return 1;
    }

    // Compute the GCD of a and b
    if (!BN_gcd(gcd, a, b, ctx)) {
        fprintf(stderr, "Error computing GCD\n");
        return 1;
    }

    // Print the GCD
    char *gcd_str = BN_bn2dec(gcd);
    if (gcd_str != NULL) {
        printf("GCD: %s\n", gcd_str);
        OPENSSL_free(gcd_str);
    } else {
        fprintf(stderr, "Error converting GCD to string\n");
    }

    // Free the allocated memory
    BN_free(a);
    BN_free(b);
    BN_free(gcd);
    BN_CTX_free(ctx);

    return 0;
}