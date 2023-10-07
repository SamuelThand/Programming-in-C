#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <stdlib.h>

#define SEARCH_PROCESSES 4
#define TEXT_BUFFER_SIZE 512
#define HELP() printf("-----------------------------------\
\nThis is a script that searches a textfile using child processes.\n\n"\
"Usage:\n\t[main.c] [filename] [\"searchstring\"] \n\n" \
"\tExample:\n\t\tmain.c textfile.txt \"findthistext\" \n-----------------------------------\n")


int countRowsInFile(FILE *filePointer);
bool unevenRowCount(int rowsCount);
void calculateRowCounters(int *rowsCount, int *rowsToAdd, int *rowsForEachProcess, bool shouldAddRows);
char **allocateTextRowsMemory(int rowsCount);
void loadTextRows(FILE *filePointer, char **textRows, bool shouldAddRows, int rowsToAdd);
void delegateSearchToChildProcesses(int rowsForEachProcess, char **textRows, char *searchString);
int searchTextWithChildProcess(char **rowsToSearch, int startingRow, int rowCount, char searchString[]);

/**
 * Searches a texfile using child processes.
 *
 * @param argc Arguments count
 * @param argv Arguments
 * @return Status code
 */
int main(int argc, char **argv)
{

    if (argc != 3 ) {
        HELP();
    }

    FILE *filePointer = fopen(argv[1], "r");
    if (!filePointer) {
        printf("Error opening file. Exiting..");
        return 1;
    }

    int rowsCount = countRowsInFile(filePointer);
    int rowsToAdd = 0;
    int rowsForEachProcess = 0;
    bool shouldAddRows = unevenRowCount(rowsCount);
    calculateRowCounters(&rowsCount, &rowsToAdd, &rowsForEachProcess, shouldAddRows);

    fseek(filePointer, 0, SEEK_SET);
    char **textRows = allocateTextRowsMemory(rowsCount);
    loadTextRows(filePointer, textRows, shouldAddRows, rowsToAdd);
    fclose(filePointer);

    delegateSearchToChildProcesses(rowsForEachProcess, textRows, argv[2]);

    for (int i = 0; i < rowsCount; i++) {
        free(textRows[i]);
    }
    free(textRows);

    return 0;
}

/**
 * Counts the rows in a file.
 *
 * @param filePointer Pointer to the file
 * @return The rowcount
 */
int countRowsInFile(FILE *filePointer)
{
    int rowsCount = 1;
    int character;
    do {
        character = fgetc(filePointer);
        if (character == '\n') {
            rowsCount++;
        }
    } while (character != EOF);

    return rowsCount;
}

/**
 * Determines if the rowcount is uneven.
 *
 * @param rowsCount The rowcount
 * @return The rowcount is uneven
 */
bool unevenRowCount(int rowsCount)
{
    if (rowsCount % SEARCH_PROCESSES != 0) {
        return true;
    } else {
        return false;
    }
}

/**
 * Calculates the rowcount, how many rows to add, and how many rows should be processed by each process.
 *
 * @param rowsCount Pointer to the rowcount
 * @param rowsToAdd Pointer to the amount of rows to add
 * @param rowsForEachProcess Pointers to the amount of rows for each process
 * @param shouldAddRows If rows should be added
 */
void calculateRowCounters(int *rowsCount, int *rowsToAdd, int *rowsForEachProcess, bool shouldAddRows)
{
    if (shouldAddRows) {
        while (*rowsCount % SEARCH_PROCESSES != 0) {
            *rowsCount += 1;
            *rowsToAdd += 1;
        }
    }
    *rowsForEachProcess = *rowsCount / SEARCH_PROCESSES;
}

/**
 * Allocate heap memory for the rows of text in the file.
 *
 * @param rowsCount The rowcount
 * @return An allocated array of for text rows
 */
char **allocateTextRowsMemory(int rowsCount)
{
    char **textRows = (char **) malloc(rowsCount * sizeof(char *));
    for (int i = 0; i < rowsCount; i++) {
        textRows[i] = (char *) malloc(TEXT_BUFFER_SIZE * sizeof(char));
    }

    return textRows;
}

/**
 * Loads all text rows from the file to the allocated array.
 *
 * @param filePointer Pointer to the file
 * @param textRows The allocated array for the text rows
 * @param shouldAddRows If rows should be added
 * @param rowsToAdd Amount of rows to be added
 */
void loadTextRows(FILE *filePointer, char **textRows, bool shouldAddRows, int rowsToAdd)
{
    int row = 0;
    char textSlice[TEXT_BUFFER_SIZE];
    while (fgets(textSlice, TEXT_BUFFER_SIZE, filePointer) != NULL) {
        strcpy(textRows[row], textSlice);
        row++;
    }

    if (shouldAddRows) {
        for (int i = 0; i < rowsToAdd; ++i) {
            strcpy(textRows[row + i], "");
        }
    }
}

/**
 * Splits the rows of texts evenly between the search processes.
 *
 * @param rowsForEachProcess The amount of rows for each process
 * @param textRows The rows of text
 * @param searchString The search string
 */
void delegateSearchToChildProcesses(int rowsForEachProcess, char **textRows, char *searchString)
{
    int startingRow = 0;
    for (int processNumber = 0; processNumber < SEARCH_PROCESSES; ++processNumber) {
        char **rowsToSearch = (char **) malloc(rowsForEachProcess * sizeof(char *));
        for (int i = 0; i < rowsForEachProcess; i++) {
            rowsToSearch[i] = (char *) malloc(TEXT_BUFFER_SIZE * sizeof(char));
        }

        int rowNumber = 0;
        while (rowNumber < rowsForEachProcess) {
            strcpy(rowsToSearch[rowNumber], textRows[rowNumber + startingRow]);
            rowNumber++;
        }

        searchTextWithChildProcess(rowsToSearch, startingRow, rowsForEachProcess, searchString);

        for (int i = 0; i < rowsForEachProcess; i++) {
            free(rowsToSearch[i]);
        }
        free(rowsToSearch);

        startingRow += rowsForEachProcess;
    }
}

/**
 * Search rows of text for the search string using a child process.
 *
 * @param rowsToSearch The rows to be searched
 * @param startingRow  The starting row number
 * @param rowCount The amount of rows to be searched
 * @param searchString The string to search for
 * @return Status code
 */
int searchTextWithChildProcess(char **rowsToSearch, int startingRow, int rowCount, char searchString[])
{
    pid_t pid;
    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Fork Failed");
        return 1;
    } else if (pid == 0) { /* child process */
        for (int row = 0; row < rowCount; row++) {
            char *occurrence = strstr(rowsToSearch[row], searchString);
            if (occurrence != NULL) {
                long column = occurrence - rowsToSearch[row];
                printf("Found in row %d at column %ld: '%s'\n", startingRow + row + 1, column + 1, occurrence);
            }
        }
        exit(0);
    } else { /* parent process */
        wait(NULL);
        return 0;
    }
}
