#include <stdio.h>
#include <string.h>

// This global string will be targeted by the String Encryption Pass
const char* SECRET_KEY = "Enterprise-Grade-Security-2024";

// This function contains loops and nested logic to test 
// Control Flow Flattening and Basic Block Splitting
int process_data(int input) {
    int result = 0;
    
    // Complex control flow for the Flattening Pass
    if (input % 2 == 0) {
        for (int i = 0; i < 10; i++) {
            result += (input * i);
            if (result > 100) {
                result -= 50;
            } else {
                result += 10;
            }
        }
    } else {
        result = input * input;
        while (result > 0) {
            result--;
        }
    }
    
    return result;
}

int main(int argc, char** argv) {
    printf("--- Obfuscator Test Binary ---\n");

    // Accessing the global string to ensure decryption stub is triggered
    if (argc > 1 && strcmp(argv[1], SECRET_KEY) == 0) {
        printf("Access Granted!\n");
    } else {
        printf("Access Denied.\n");
    }

    int val = 42;
    int processed = process_data(val);
    
    printf("Input: %d, Processed Result: %d\n", val, processed);
    
    return 0;
}
