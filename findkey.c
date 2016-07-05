#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <inttypes.h>
#include <string.h>
#include <openssl/des.h>
#include <openssl/rand.h>
#include <fcntl.h>
#include <stdbool.h>

bool iterateAndDecrypt(uint32_t seed, uint32_t max, char *encFile, long size, FILE *plaintextFile, char *decFile);
void decrypt(uint32_t seed, char* file, void* ciphertext, long cipherSize);
bool startsWith(const char *a, const char *b);

int main (int argc, char *argv[]) {
    // Get the encrypted file and the file for our plaintext in the command line
    char *encFile = argv[1];
    char *decFile = argv[2];
    FILE *ciphertextFile;
    FILE *plaintextFile;

    // Open encrypted file
    if (encFile != NULL) {
        ciphertextFile = fopen(encFile, "r" );
    }

    // If there was an error opening the file, terminate program
    if (ciphertextFile == 0 ) {
        printf( "Could not open file\n" );
        exit(1);
    }

    // Get the size of the encrypted file
    fseek(ciphertextFile, 0, SEEK_END);
    long size = ftell(ciphertextFile);

    // Stat the file to get it's timestamo and terminate on error
    struct stat statbuf;
    if (stat(encFile, &statbuf) == -1) {
        perror(encFile);
        exit(1);
    }
    fclose(ciphertextFile);

    // Initialize 32-bit integers to formulate candidate seeds
    uint32_t timestamp;
    uint32_t seed;

    // Initially set all the masks to brute force the bottom 16 bits
    uint32_t mask = 0xFFFF0000;
    uint32_t maxMask= 0x0000FFFF;

    // Maintain max and min for iterating through all of the candidates
    uint32_t max = 0x0000FFFF;
    uint32_t min = seed;
    uint32_t counter = 0;

    // Get the timestamp from the stat call
    timestamp = statbuf.st_mtime;
    bool found = false;

    // Keep going until we've found the seed or until we've brute forced all 32 bits
    while (!found && counter <= 0xFFFFFFFF) {
        // AND timestamp with mask to make lower bits go to 0
        seed = timestamp & mask;
        // Max is the most iterations it takes to increment the lower bits to 1
        max = seed + maxMask;
        min = seed;

        // Attempt to decrypt the text with a range of candidate seeds (min-max)
        found = iterateAndDecrypt(min, max, encFile, size, plaintextFile, decFile);

        // Set for next attempt; shift masking bits left to try a bigger range
        mask = mask << 1;
        maxMask = maxMask << 1;

        // Ensure that lower 16 bits max mask are 1 (after shift when they were 0)
        maxMask = maxMask | 0x0000FFFF;
        counter++;
    }

    // Let user know if we were unable to find seed
    if (!found) {
        printf("Unable to decrypt file :(\n");
    }
    return;
}

// Iterate through candidate seeds and corresponding candidate decrypted text, testing for plaintext
bool iterateAndDecrypt(uint32_t seed, uint32_t max, char *encFile, long size, FILE *plaintextFile, char *decFile) {
    // Create a string for text decrypted from file
    char candidateText[size];
    bool found = 0;

    // Test all possible seeds from min to max, incrementing one bit at a time
    for (; seed < max; seed ++ ) {
        // Reset candidateText string and try to decrypt
        memset(candidateText, 0, size*sizeof(char));
        decrypt(seed, encFile, candidateText, sizeof(candidateText));

        // If the decrypted text starts with 'Correct', we have found the seed
        // And have the plaintext
        if(startsWith(candidateText, "Correct")) {
            // Print the seed in hexidecimal
            printf("%x\n", seed);
            // Open results file from command line and write decrypted plaintext to it
            plaintextFile = fopen(decFile, "w");
            if (plaintextFile == NULL) {
                printf("Error opening file!\n");
                exit(1);
            }
            fprintf(plaintextFile, "%s", candidateText);
            fclose(plaintextFile);
            found = 1;
            break;
        }
    }
    return found;
}

// Decrypts ciphertext with a given seed and size
void decrypt(uint32_t seed, char* file, void* ciphertext, long cipherSize) {
    DES_cblock iv, key;
    DES_key_schedule schedule;
    srand(seed);
    int i;
    for (i=0; i<8; i++)
        key[i] = (unsigned char)rand();
    for (i=0; i<8; i++)
        iv[i] = (unsigned char)rand();
    DES_set_odd_parity(&key);
    DES_set_key_checked(&key, &schedule);
    // Open encrypted file to read ciphertext
    int fd = open (file, O_RDONLY);
    if(!fd) {
        printf("Could not open encrypted file");
        exit(1);
    }
    // Attempt to decrypt and put result in ciphertext
    DES_enc_read(fd, ciphertext, cipherSize, &schedule, &iv);
    close(fd);
}

// Tests to see if a given text begins with another string
bool startsWith(const char *a, const char *b) {
   if(strncmp(a, b, strlen(b)) == 0) {
       return 1;
   }
   else {
       return 0;
   }
}
