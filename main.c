#include <stdio.h>
#include <stdlib.h>
#include <openssl/bn.h>
#include <openssl/rand.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>

#define K 512
#define MAX_NUMBERS 10000
#define NUM_THREADS 8
#define TESTFILE "unknown_numbers.txt"

typedef struct {
    int start_idx;
    int end_idx;
    BIGNUM **nums;
    BIGNUM ***gcds;
} ThreadData;

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

void *compute_gcds_alternating(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    BN_CTX *ctx = BN_CTX_new(); // Each thread creates its own BN_CTX

    for (int i = data->start_idx; i < data->end_idx; i += NUM_THREADS) {
        for (int j = i + 1; j < data->end_idx; j++) {
            data->gcds[i][j] = BN_new();
            
            // Print the numbers before computing GCD
            char *num_i_str = BN_bn2dec(data->nums[i]);
            char *num_j_str = BN_bn2dec(data->nums[j]);
            OPENSSL_free(num_i_str);
            OPENSSL_free(num_j_str);
            
            if (!BN_gcd(data->gcds[i][j], data->nums[i], data->nums[j], ctx)) {
                fprintf(stderr, "BN_gcd failed for numbers %d and %d\n", i, j);
                exit(EXIT_FAILURE);
            }

            // Print the GCD for debugging
            char *gcd_str = BN_bn2dec(data->gcds[i][j]);
            OPENSSL_free(gcd_str);
        }
        printf("Thread %d completed row %d\n", data->start_idx, i);
    }

    BN_CTX_free(ctx); // Free the context after use
    return NULL;
}


int main() {
    BIGNUM **nums = (BIGNUM **)malloc(MAX_NUMBERS * sizeof(BIGNUM *));
    BIGNUM ***gcds = (BIGNUM ***)malloc(MAX_NUMBERS * sizeof(BIGNUM **));
    char ***factorizations = (char ***)malloc(MAX_NUMBERS * sizeof(char **));
    BN_CTX *bn_ctx = BN_CTX_new();
    pthread_t threads[NUM_THREADS];
    ThreadData thread_data[NUM_THREADS];
    
    if (!nums) {
        fprintf(stderr, "Initial memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen(TESTFILE, "r");
    if (!file) {
        fprintf(stderr, "Failed to open textfile\n");
        exit(EXIT_FAILURE);
    }

    char line[1024];
    int N = 0; // This will now be determined by the number of lines read from the file

    while (fgets(line, sizeof(line), file) && N < MAX_NUMBERS) {
        nums[N] = BN_new();
        BN_dec2bn(&nums[N], line); // Convert line to BIGNUM and store in nums[N]
        N++;
    }
    printf("N = %d, K = %d, MAX_NUMBERS = %d\n", N, K, MAX_NUMBERS);

    printf("BEGIN FACTORING\n");

    printf("Memory allocation for numbers, GCDs, factorizations, and BN_CTX complete.\n");

    // Allocate 2D arrays for GCDs and factorizations
    for (int i = 0; i < N; i++) {
        gcds[i] = (BIGNUM **)malloc(N * sizeof(BIGNUM *));
        factorizations[i] = (char **)malloc(N * sizeof(char *));
        for (int j = 0; j < N; j++) {
            gcds[i][j] = NULL; // Initialize with NULL
            factorizations[i][j] = NULL; // Initialize with NULL
        }
    }

    printf("Memory allocation for 2D arrays of GCDs and factorizations complete.\n");

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].start_idx = i;
        thread_data[i].end_idx = MAX_NUMBERS;
        thread_data[i].nums = nums;
        thread_data[i].gcds = gcds;

        printf("Creating thread %d with range %d to %d\n", i, i, N);

        if (pthread_create(&threads[i], NULL, compute_gcds_alternating, &thread_data[i])) {
            fprintf(stderr, "Error creating thread\n");
            return 1;
        }
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
        printf("Thread %d has finished execution\n", i);
    }

    // Print the GCDs
    for (int i = 0; i < N; i++) {
        for (int j = i + 1; j < N; j++) {
            if (gcds[i][j] != NULL) {
                char *gcd_str = BN_bn2dec(gcds[i][j]); // Convert the BIGNUM to a decimal string representation
                if (gcd_str) {
                    OPENSSL_free(gcd_str); // Free the string allocated by BN_bn2dec
                } else {
                    printf("Failed to convert GCD to string for numbers %d and %d\n", i, j);
                }
            }
        }
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // Compute Factorizations of the GCDs
    for (int i = 0; i < N; i++) {
        for (int j = i + 1; j < N; j++) {
            if (gcds[i][j] != NULL) {  // Ensure GCD was successfully computed
                unsigned int gcd_uint = BN_get_word(gcds[i][j]);
                if (gcd_uint != (unsigned int)-1) { // Check conversion success
                    factorizations[i][j] = factorize(gcd_uint);
                } else {
                    fprintf(stderr, "Failed to convert GCD to unsigned int\n");
                }
            }
        }
        if (i % 100 == 0) {
            printf("Computed GCDs and factorizations for %d/%d numbers.\n", i, N);
        }
    }

    // Old method for computing GCDs and Factorizations
    // for (int i = 0; i < N; i++) {
    //     for (int j = i + 1; j < N; j++) { // Avoid duplicate calculations and self GCD
    //         // gcds[i][j] = BN_new();
    //         // if (!BN_gcd(gcds[i][j], nums[i], nums[j], bn_ctx)) {
    //         //     fprintf(stderr, "BN_gcd failed\n");
    //         //     exit(EXIT_FAILURE);
    //         // }
    //         // Convert BIGNUM to unsigned int and factorize
    //         unsigned int gcd_uint = BN_get_word(gcds[i][j]);
    //         if (gcd_uint != (unsigned int)-1) { // Check conversion success
    //             factorizations[i][j] = factorize(gcd_uint);
    //         }
    //     }
    //     if (i % 100 == 0) {
    //         printf("Computed GCDs and factorizations for %d/%d numbers.\n", i, N);
    //     }
    // }

    printf("GCDs computed and factorized for all numbers.\n");

    char** combined_factors = create_combined_factors(factorizations, N);

    printf("Combined factors created for all numbers.\n");

    // Allocate memory for the verdicts array
    bool *verdicts = (bool *)malloc(N * sizeof(bool));
    if (!verdicts) {
        fprintf(stderr, "Allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // Check divisibility for each number by its combined factors
    for (int i = 0; i < N; i++) {
        verdicts[i] = can_be_factored_by_combined_factors(nums[i], combined_factors[i], bn_ctx);
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
                printf("Number %d (Value: %s) can be factored by its combined factors: %s\n", i, num_str, combined_factors[i]);
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
        BN_free(nums[i]); // Free each BIGNUM in nums array

        for (int j = i + 1; j < N; j++) {
            if (gcds[i][j] != NULL) { // Ensure that GCDs that were allocated are freed
                BN_free(gcds[i][j]);
            }
            if (factorizations[i][j] != NULL) { // Free factorization results
                free(factorizations[i][j]);
            }
        }
        free(gcds[i]);  // Free the inner array of gcds for each i
        free(factorizations[i]); // Free the inner array of factorizations for each i
    }

    free(nums); // Free the array of pointers to BIGNUMs
    free(gcds); // Free the outer array of pointers to pointers to BIGNUMs
    free(factorizations); // Free the outer array of pointers to char pointers

    if (verdicts != NULL) { // Check if verdicts was allocated and needs to be freed
        free(verdicts);
    }

    for (int i = 0; i < N; i++) {
        if (combined_factors[i] != NULL) { // Ensure each combined factor string is freed
            free(combined_factors[i]);
        }
    }
    free(combined_factors); // Free the array of combined factors

    BN_CTX_free(bn_ctx); // Free the BN_CTX

    printf("Memory freed and program completed.\n");

    return 0;
}