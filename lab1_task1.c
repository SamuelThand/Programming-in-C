#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <ctype.h>

#define MAX_COMMAND_LENGTH 30
#define MAX_COMMANDS_IN_HISTORY 10
#define PROMPT() printf("SamSh>")
#define HELP() printf("-----------------------------------\
\nThis is a shell that executes commands in a child process.\n\n"\
"Usage:\n\t[command] [arguments] [&] \n\n" \
"\t&\n\t\tRun the command concurrently\n-----------------------------------\n")

void printCommandHistory(char history[MAX_COMMANDS_IN_HISTORY][MAX_COMMAND_LENGTH], int commandCounter, int historyIndex);
void doCommand(char command[]);
char **parseCommand(char input[]);
int executeCommandInChildProcess(char *command[]);
void freeCommand(char **executedCommand);
void incrementIndex(int *historyIndex, int *commandCounter);

/**
 * Shell for executing commands in a child process.
 *
 * @return exit code
 */
int main(void)
{
    char input[MAX_COMMAND_LENGTH];
    char history[MAX_COMMANDS_IN_HISTORY][MAX_COMMAND_LENGTH];
    int historyIndex = 0;
    int commandCounter = 0;

    while (true) {
        PROMPT();
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("invalid input, exiting..");
            exit(EXIT_FAILURE);
        } else if (*input == '\n' || input[0] == ' ') {
            continue;
        }

        input[strcspn(input, "\n")] = '\0';

        if (strcmp("help", input) == 0) {
            HELP();
        }
        else if (strcmp("exit", input) == 0) {
            printf("Exiting..");
            exit(EXIT_SUCCESS);
        }
        else if (strcmp("history", input) == 0) {
            printCommandHistory(history, commandCounter, historyIndex);
        }
        else if (input[0] == '!') {
            if (strcmp("!!", input) == 0) {
                if (commandCounter > 0) {
                    PROMPT();
                    printf("%s\n", history[historyIndex - 1]);
                    strcpy(history[historyIndex], history[historyIndex - 1]);
                    doCommand(history[historyIndex - 1]);
                    incrementIndex(&historyIndex, &commandCounter);
                }
            } else if (isdigit(input[1]) && input[1] != '0' && strlen(input) == 2) {
                int requestedHistoryIndex = (input[1] - '0') - 1;
                if (strcmp(history[requestedHistoryIndex], "") == 0) {
                    printf("No such command in history.\n");
                } else {
                    PROMPT();
                    printf("%s\n", history[requestedHistoryIndex]);
                    strcpy(history[historyIndex], history[requestedHistoryIndex]);
                    doCommand(history[requestedHistoryIndex]);
                    incrementIndex(&historyIndex, &commandCounter);
                }
            }
        }
        else {
            strcpy(history[historyIndex], input);
            doCommand(input);
            incrementIndex(&historyIndex, &commandCounter);
        }
    }
}

/**
 * Print the 10 most recent commands to the console.
 *
 * @param history 10 most recent executed commands
 * @param commandCounter The current count of issued commands
 * @param historyIndex The current position in the history index
 */
void printCommandHistory(char history[MAX_COMMANDS_IN_HISTORY][MAX_COMMAND_LENGTH], int commandCounter, int historyIndex)
{
    if (strcmp(history[0], "") == 0) {
        printf("No commands in history.\n");
        return;
    }

    int startIndex = (historyIndex - 1 + MAX_COMMANDS_IN_HISTORY) % MAX_COMMANDS_IN_HISTORY;
    for (int i = 0, j = 0; i < MAX_COMMANDS_IN_HISTORY; i++, j++) {
        if (strcmp("", history[i]) != 0) {
            int index = (startIndex - i + MAX_COMMANDS_IN_HISTORY) % MAX_COMMANDS_IN_HISTORY;
            printf("%d: %s\n", commandCounter - j, history[index]);
        }
    }
}

/**
 * Do the issued command.
 *
 * @param command The issued command
 */
void doCommand(char command[MAX_COMMAND_LENGTH])
{
    char **parsedCommand = parseCommand(command);
    executeCommandInChildProcess(parsedCommand);
    freeCommand(parsedCommand);
}

/**
 * Parse the issued command.
 *
 * @param input The issued command
 * @return Parsed command
 */
char **parseCommand(char input[])
{
    char **parsedCommand;
    char *pointerToEndOfToken;
    char *tempToken = strdup(input);

    int tokenCount = 0;
    char *tokens[MAX_COMMAND_LENGTH];
    char *token = strtok_r(tempToken, " ", &pointerToEndOfToken);
    while (token != NULL && tokenCount < MAX_COMMAND_LENGTH) {
        tokens[tokenCount] = strdup(token);
        tokenCount++;
        token = strtok_r(NULL, " ", &pointerToEndOfToken);
    }
    free(tempToken);

    parsedCommand = malloc(tokenCount * sizeof (char*) + sizeof(NULL));
    if (parsedCommand == NULL) {
        printf("Error: malloc failed in parseCommand\n");
        exit(EXIT_FAILURE);
    }

    memcpy(parsedCommand, tokens, tokenCount * sizeof (char*) + sizeof(NULL));
    parsedCommand[tokenCount] = NULL;

    return parsedCommand;
}

/**
 * Execute the command in a child process.
 *
 * @param input The command to execute
 * @return Status code
 */
int executeCommandInChildProcess(char *command[])
{
    pid_t pid;
    bool runConcurrently = false;

    int lastCommandArgumentIndex = 0;
    while (command[lastCommandArgumentIndex] != NULL) {
        lastCommandArgumentIndex++;
    } if (strcmp(command[lastCommandArgumentIndex - 1], "&") == 0 ) {
        if (lastCommandArgumentIndex != 1) {
            free(command[lastCommandArgumentIndex - 1]);
            command[lastCommandArgumentIndex - 1] = NULL;
            runConcurrently = true;
        }
    }

    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Fork Failed");
        return 1;
    } else if (pid == 0) { /* child process */
        execvp(command[0], command);
        exit(0);
    } else { /* parent process */
        if (!runConcurrently) {
            wait(NULL);
        }

        return 0;
    }
}

/**
 * Deallocate memory for an executed command.
 *
 * @param executedCommand The executed command
 */
void freeCommand(char **executedCommand) {
    int i = 0;
    while (executedCommand[i] != NULL) {
        free(executedCommand[i]);
        i++;
    }
    free(executedCommand);
}

/**
 * Increments the current position in the history index.
 *
 * @param historyIndex The history index
 * @param commandCounter The current count of issued commands
 */
void incrementIndex(int *historyIndex, int *commandCounter)
{
    *commandCounter = *commandCounter + 1;
    *historyIndex = *commandCounter % MAX_COMMANDS_IN_HISTORY;
}
