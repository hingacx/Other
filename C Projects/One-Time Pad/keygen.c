#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>




int main(int argc, char *argv[]) {

    // Checks to see if the appropriate number of files were passed.
    if (argc < 2) {
        printf("Not enough arguments, try ./keygen keylength\n");
        exit(0); 
    }

    // Generates a seed to get a random number each time.
    srand(time(0));

    // Converts the argument into a int data type.
    int len = atoi(argv[1]);
    // Create a string to hold the random chars with the size being the argument + 1 to hold '\n'.
    char key[len+1];

    for (int i = 0; i < len; i++) {
        // Gets a random number 0-27.
        int tempNum = rand() % 27;

        // Checks to see if the number is a ASCII number A - Z.
        if (tempNum < 26) {
            // 65 Represents 'A', so if tempNum is 0, then we have 65 + 0 = 65 = 'A' as an example.
            key[i] = tempNum + 65;

        // If the tempNum is 27, then we set the char to be the ASCII number 32 which is a SPACE.
        } else {
            key[i] = 32;

        }
    }

    // Sets the last spot to be a newline char.
    key[len] = '\n';
    write(STDOUT_FILENO, key, len+1);

    return 0;
}