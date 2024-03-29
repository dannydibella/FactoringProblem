#include <stdio.h>
#include <stdlib.h>
#include <openssl/bn.h>
#include <openssl/rand.h>
#include <string.h>
#include <stdbool.h>

#define K 256
#define MAX_NUMBERS 100

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

char* factorize_large(BIGNUM* bn) {
    BN_CTX* ctx = BN_CTX_new();
    BIGNUM* n = BN_dup(bn); // Duplicate bn to n because we will be modifying it
    BIGNUM* i = BN_new();
    BIGNUM* remainder = BN_new();
    BIGNUM* quotient = BN_new();
    char* result = (char*)malloc(1024 * sizeof(char)); // Adjust size as needed
    char* temp_str;
    result[0] = '\0'; // Initialize to empty string

    BN_set_word(i, 2); // Start with the smallest prime, 2
    // Check divisibility by 2 first to make the later loop simpler
    if (BN_mod(remainder, n, i, ctx) == 0) { // n % 2 == 0
        strcat(result, "2");
        BN_div(n, NULL, n, i, ctx); // n /= 2
        while (BN_mod(remainder, n, i, ctx) == 0) { // while n % 2 == 0
            strcat(result, ",2");
            BN_div(n, NULL, n, i, ctx); // n /= 2
        }
    }

    // Factorize for odd factors starting from 3
    BN_set_word(i, 3);
    while (BN_cmp(i, n) <= 0) {
        if (BN_mod(remainder, n, i, ctx) == 0) { // n % i == 0
            if (strlen(result) > 0) strcat(result, ",");
            temp_str = BN_bn2dec(i);
            strcat(result, temp_str);
            OPENSSL_free(temp_str);
            BN_div(n, NULL, n, i, ctx); // n /= i
            while (BN_mod(remainder, n, i, ctx) == 0) {
                strcat(result, ",");
                temp_str = BN_bn2dec(i);
                strcat(result, temp_str);
                OPENSSL_free(temp_str);
                BN_div(n, NULL, n, i, ctx); // n /= i
            }
        }
        BN_add(i, i, BN_value_one()); // i += 1
        BN_add(i, i, BN_value_one()); // Skip even numbers, i += 2
    }

    // Cleanup
    BN_free(i);
    BN_free(remainder);
    BN_free(quotient);
    BN_free(n);
    BN_CTX_free(ctx);

    return result;
}


int main() {
    BIGNUM **nums = (BIGNUM **)malloc(MAX_NUMBERS * sizeof(BIGNUM *));
    BIGNUM *large_product = BN_new();
    BN_one(large_product);
        
    if (!nums) {
        fprintf(stderr, "Initial memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen("numbers.txt", "r");
    if (!file) {
        fprintf(stderr, "Failed to open numbers.txt\n");
        exit(EXIT_FAILURE);
    }

    char line[1024];
    int N = 0; // This will now be determined by the number of lines read from the file

    while (fgets(line, sizeof(line), file) && N < MAX_NUMBERS) {
        nums[N] = BN_new();
        BN_dec2bn(&nums[N], line); // Convert line to BIGNUM and store in nums[N]
        N++;
    }
    printf("N = %d, K = %d\n", N, K);

    printf("BEGIN FACTORING\n");

    BN_CTX *bn_ctx = BN_CTX_new();
    BIGNUM **gcds = (BIGNUM **)malloc(N * sizeof(BIGNUM *));
    char **factorizations = (char **)malloc(N * sizeof(char *));

    for (int i = 0; i < N; i++) {
        BN_mul(large_product, large_product, nums[i], bn_ctx);
    }

    printf("Large Product Calculated\n");

    for (int i = 0; i < N; i++) {
        BIGNUM *adjusted_large_product = BN_new();
        BN_copy(adjusted_large_product, large_product);

        // Divide adjusted_large_product by nums[i] for the current iteration
        BIGNUM *div_result = BN_new();
        BN_CTX_start(bn_ctx); // Ensure context is started for temporary variables
        BIGNUM *remainder = BN_CTX_get(bn_ctx); // Temporary variable for remainder, ensuring it's cleaned up with the context

        // Perform the division
        if (!BN_div(div_result, remainder, adjusted_large_product, nums[i], bn_ctx)) {
            fprintf(stderr, "Division failed at iteration %d\n", i);
        }

        // Ensure there's no remainder in the division; otherwise, it indicates an issue
        if (!BN_is_zero(remainder)) {
            fprintf(stderr, "Non-zero remainder at iteration %d, unexpected division result\n", i);
        }

        // Use div_result (adjusted_large_product / nums[i]) for GCD calculation
        gcds[i] = BN_new();

        printf("Division done\n");

        BN_gcd(gcds[i], nums[i], div_result, bn_ctx); // Compute GCD

        printf("GCD done\n");

        factorizations[i] = factorize_large(gcds[i]); // Factorize the GCD

        // Cleanup temporary BIGNUMs
        BN_free(adjusted_large_product);
        BN_free(div_result);
        BN_CTX_end(bn_ctx); // Free up any temporary variables allocated within the context during this iteration

        printf("ITERATION: %d\n", i);
    }

    printf("GCDs computed and factorized for all numbers.\n");

    // Allocate memory for the verdicts array
    bool *verdicts = (bool *)malloc(N * sizeof(bool));
    if (!verdicts) {
        fprintf(stderr, "Allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // Check divisibility for each number by its combined factors
    for (int i = 0; i < N; i++) {
        verdicts[i] = can_be_factored_by_combined_factors(nums[i], factorizations[i], bn_ctx);
        if (i % 1000 == 0) {
            printf("Checked divisibility for %d/%d numbers.\n", i, N);
        }
    }

    printf("Divisibility checked for all numbers.\n");

    // Print the summary based on verdicts
    bool any_factored = false;
    for (int i = 0; i < N; i++) {
        if (verdicts[i]) {
            any_factored = true;
            char *num_str = BN_bn2dec(nums[i]); // Convert the BIGNUM to a decimal string representation
            if (num_str) {
                printf("Number %d (Value: %s) can be factored by its combined factors: %s\n", i, num_str, factorizations[i]);
                OPENSSL_free(num_str); // Free the string allocated by BN_bn2dec
            } else {
                printf("Failed to convert BIGNUM to string for number %d\n", i);
            }
        }
    }

    if (!any_factored) {
        printf("SUCCESS. Every number has a unique prime factor.\n");
    } else {
        printf("FAILURE. Not every number has a unique prime factor.\n");
    }

    // Freeing memory and other cleanup...
    for (int i = 0; i < N; i++) {
        BN_free(nums[i]);
    }
    free(nums);

    free(verdicts);
    for (int i = 0; i < N; i++) {
        free(factorizations[i]);
        free(gcds[i]);
    }
    free(factorizations);
    free(gcds);
    BN_CTX_free(bn_ctx);

    printf("Memory freed and program completed.\n");

    return 0;
}