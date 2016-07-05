// Must link libraries -lpthread -lm in gcc for compiling...see Makefile

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
#include <math.h>
#include <pthread.h>

#define NUM_THREADS 10

void *threadWork(void *argument);
bool iterateAndDecrypt(uint32_t seed, uint32_t max, char *encFile, long size, FILE *plaintextFile, char *decFile);
void decrypt(uint32_t seed, char* file, void* ciphertext, long cipherSize);
bool startsWith(const char *a, const char *b);

struct threadParams {
    uint32_t seed;
    uint32_t max;
    char *encFile;
    long size;
    FILE *plaintextFile;
    char *decFile;
};


int main (int argc, char *argv[]) {

    // We assume argv[1] is a filename to open
    char *encFile = argv[1];
    char *decFile = argv[2];
    FILE *ciphertextFile;
    FILE *plaintextFile;
    if (encFile != NULL) {
        ciphertextFile = fopen(encFile, "r" );
    }

    /* fopen returns 0, the NULL pointer, on failure */
    if (ciphertextFile == 0 ) {
        printf( "Could not open file\n" );
    }

    fseek(ciphertextFile, 0, SEEK_END); // seek to end of file
    long size = ftell(ciphertextFile); // get current file pointer

    struct stat statbuf;
    if (stat(encFile, &statbuf) == -1) {
        perror(encFile);
        exit(1);
    }
    fclose(ciphertextFile);

    uint32_t seed = 0;
    uint32_t timestamp;
    uint32_t mask = 0xF9000000;
    timestamp = statbuf.st_mtime;

    long max = 0x07FFFFFF;
    bool found = 0;
    char buf[size];
    seed = timestamp & mask;
    max = seed + max;
    long min = seed;

    printf("original timestamp: %" PRIu32 "\n", timestamp);
    printf("min: %" PRIu32 "\n", seed);
    printf("max: %lu\n", max);

    pthread_t threads[NUM_THREADS];
    int thread_args[NUM_THREADS];

    uint32_t partition = (uint32_t) floor((max - min) / NUM_THREADS);
    int i;
    uint32_t tempMin = min;
    uint32_t tempMax = min;
    printf("partition: %" PRIu32 "\n", partition);

    // Create all threads one by one
    for (i = 0; i < NUM_THREADS; i++) {
        thread_args[i] = i;
        tempMin = tempMax;
        tempMax += partition;
        // Cover the last few elements that may not have fit in

            tempMax = max;
        }

        printf("T min: %" PRIu32 "\n", tempMin);
        printf("T max: %" PRIu32 "\n\n", tempMax);

        struct threadParams params;
        params.encFile = encFile;
        params.size = size;
        params.plaintextFile = plaintextFile;
        params.decFile = decFile;

        params.seed = tempMin;
        params.max = tempMax;

        pthread_create(&threads[i] , NULL, threadWork, (void *) &params);
    }

    // Wait for each thread to complete
    for (i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads[i], NULL);
    }

    printf("In main: All threads completed successfully\n");
    printf("Unable to decrypt file :(\n");
}

void *threadWork(void *argument) {
   struct threadParams *params = argument;

   iterateAndDecrypt(params->seed, params->max, params->encFile, params->size, params->plaintextFile, params->decFile);

   printf("This thread is done, no success\n");
}

bool iterateAndDecrypt(uint32_t seed, uint32_t max, char *encFile, long size, FILE *plaintextFile, char *decFile) {
    char buf[size];
    bool found = 0;
    for (; seed < max; seed ++ ) {
        if (seed % 100000 == 0) {

        }
        memset(buf, 0, size*sizeof(char));
        decrypt(seed, encFile, buf, sizeof(buf));
        if(startsWith(buf, "Correct")) {
            printf("FOUND IT! Seed: %" PRIu32 "\n", seed);
            printf("%x\n", seed);
            printf("Text: %s\n", buf);

            plaintextFile = fopen(decFile, "w");
            if (plaintextFile == NULL) {
                printf("Error opening file!\n");
                exit(1);
            }
            fprintf(plaintextFile, "%s", buf);
            fclose(plaintextFile);

            exit(0);
        }
    }
    return found;

}

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
    int fd = open (file, O_RDONLY);
    if(!fd) {
        printf("Could not open encrypted file");
        exit(1);
    }
    DES_enc_read(fd, ciphertext, cipherSize, &schedule, &iv);
    close(fd);
}

bool startsWith(const char *a, const char *b) {
   if(strncmp(a, b, strlen(b)) == 0) {
       return 1;
   }
   else {
       return 0;
   }
}
