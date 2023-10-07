#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <ctype.h>

#define HELP() printf("-----------------------------------\
\nThis is a script that calculates the factorial of a number N using a child process.\
\nIt can handle up to 20!\n\n"\
"Usage:\n\t[main.c] [N]\n\n" \
"\tExample:\n\t\tmain.c 5\n-----------------------------------\n");

bool isNumericInput(char input[]);
int calculateFactorialInChildProcess(long long int N);

/**
 * Calculates the factorial of a number N using a child process.
 *
 * @param argc Argument count
 * @param argv Arguments
 * @return Status code
 */
int main(int argc, char **argv) {
    if (argc != 2) {
        HELP()
        exit(1);
    } else if (!isNumericInput(argv[1])) {
        printf("%s is not a number, exiting..\n", argv[1]);
        exit(1);
    }

    long long int N = atoll(argv[1]);
    calculateFactorialInChildProcess(N);
    return 0;
}

/**
 * Checks if each character in the input string is numeric.
 *
 * @param input Input string
 * @return The input is numeric
 */
bool isNumericInput(char input[])
{
    while (*input != '\0') {
        if (isdigit(*input++) == 0) {
            return false;
        }
    }

    return true;
}

/**
 * Calculates the factorial of N, prints each step, sums the steps and prints the result - using
 * a child process.
 *
 * @param N Factorial number
 * @return Status code
 */
int calculateFactorialInChildProcess(long long int N)
{
    pid_t pid;
    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Fork Failed");
        return 1;
    } else if (pid == 0) { /* child process */
        long long int factorial = 1;
        long long int steps[N];
        for (long long int i = 1; i <= N; ++i) {
            factorial = factorial * i;
            steps[i-1] = factorial;
            for (long long int j = 0; j < i; ++j) {
                printf("%lld ", steps[j]);
            }
            printf("\n");
        }
        printf("\nThe sum of the series is:\n");
        long long int lastSum = 0;
        for (long long int j = 0; j < N; ++j) {
            lastSum += steps[j];
            printf("%lld ",lastSum);
        }
        exit(0);
    } else { /* parent process */
        wait(NULL);
        return 0;
    }
}
