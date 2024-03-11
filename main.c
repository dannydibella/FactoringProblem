#include <stdio.h>
#include <stdlib.h>
#include <openssl/bn.h>
#include <openssl/rand.h>
#include <string.h>
#include <stdbool.h>

#define K 256
#define N 10000

void generate_odd_256bit_integer(BIGNUM *num) {
    if (!BN_rand(num, K, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ODD)) {
        fprintf(stderr, "Random number generation failed\n");
        exit(EXIT_FAILURE);
    }
}

bool can_be_factored_by_combined_factors(BIGNUM *num, char* combined_factors, BN_CTX *ctx) {
    if (strlen(combined_factors) == 0) return false; // No factors provided

    BIGNUM *remainder = BN_new();
    BIGNUM *divisor = BN_new();
    BIGNUM *quotient = BN_new();
    BIGNUM *one = BN_new();
    BIGNUM *temp_num = BN_dup(num); // Duplicate num to keep original number intact
    BN_one(one); // Initialize 'one' as a BIGNUM representation of 1

    char *token = strtok(combined_factors, ",");
    while (token != NULL) {
        unsigned long factor_val = strtoul(token, NULL, 10);
        BN_set_word(divisor, factor_val);

        // Divide temp_num by divisor as long as there's no remainder
        do {
            BN_div(quotient, remainder, temp_num, divisor, ctx); // quotient = temp_num / divisor, remainder = temp_num % divisor

            if (BN_cmp(remainder, one) < 0) { // If remainder is 0, division is exact
                BN_copy(temp_num, quotient); // Update temp_num with quotient for next iteration
            }
        } while (BN_is_zero(remainder)); // Continue while division is exact (remainder is 0)

        token = strtok(NULL, ",");
    }

    // After trying to divide by all factors, check if temp_num has been reduced to 1
    bool is_factorable = BN_cmp(temp_num, one) == 0;

    // Cleanup
    BN_free(remainder);
    BN_free(divisor);
    BN_free(quotient);
    BN_free(one);
    BN_free(temp_num);

    return is_factorable;
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

char** create_combined_factors(char ***factorizations, int num_numbers) {
    char** combined_factors = (char**)malloc(num_numbers * sizeof(char*));
    for (int i = 0; i < num_numbers; i++) {
        combined_factors[i] = (char*)malloc(1024 * sizeof(char)); // Allocate space
        combined_factors[i][0] = '\0'; // Initialize to empty string

        for (int j = 0; j < num_numbers; j++) {
            if (factorizations[i][j] != NULL) {
                char temp_factors[1024]; // Temporary buffer for tokenizing
                strcpy(temp_factors, factorizations[i][j]); // Copy to temp buffer
                char* token = strtok(temp_factors, ",");
                while (token != NULL) {
                    if (!strstr(combined_factors[i], token)) {
                        if (strlen(combined_factors[i]) > 0) strcat(combined_factors[i], ",");
                        strcat(combined_factors[i], token);
                    }
                    token = strtok(NULL, ",");
                }
            }
        }
    }
    return combined_factors;
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

    char** combined_factors = create_combined_factors(factorizations, N);

    // Print combined factors for each number
    for (int i = 0; i < N; i++) {
        printf("Combined factors for number %d: %s\n", i, combined_factors[i]);
    }

    // Allocate memory for the verdicts array
    bool *verdicts = (bool *)malloc(N * sizeof(bool));
    if (!verdicts) {
        fprintf(stderr, "Allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // Check divisibility for each number by its combined factors
    for (int i = 0; i < N; i++) {
        verdicts[i] = can_be_factored_by_combined_factors(nums[i], combined_factors[i], bn_ctx);
    }

    // Print the summary based on verdicts
    bool any_factored = false;
    for (int i = 0; i < N; i++) {
        if (verdicts[i]) {
            any_factored = true;
            char *num_str = BN_bn2dec(nums[i]); // Convert the BIGNUM to a decimal string representation
            if (num_str) {
                printf("Number %d (Value: %s) can be factored by its combined factors: %s\n", i, num_str, combined_factors[i]);
                OPENSSL_free(num_str); // Free the string allocated by BN_bn2dec
            } else {
                printf("Failed to convert BIGNUM to string for number %d\n", i);
            }
        }
    }

    if (any_factored) {
        printf("\nFAILURE. Not every number has a unique prime factor.\n");
    } else {
        printf("\nSUCCESS. Every number has a unique prime factor.\n");
    }

    // Freeing memory and other cleanup...
    free(verdicts);

    // Free combined factors
    for (int i = 0; i < N; i++) {
        free(combined_factors[i]);
    }
    free(combined_factors);

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

    printf("\nCOMPLETE\n\n");

    return 0;
}