/*
 * shell.c
 *
 * Author: Daniel Lovegrove
 *
 * Version: January 19/2018
 *
 * Purpose: Get familiar with fork and exec. This program takes user input and
 * executes what the user enters until they enter Ctrl-D, at which point the
 * program exits.
 */


#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define MAX_INPUT_LEN 80
#define DELIMITER " "

void freeArray(void **ary, int len);
int countTokens(const char *str, const char *del);
void getCommandWithArgs(char *input, char *del, char *cmd, char ***args, int *arglen);
void executeUserCommand(char *cmd, char ***args);


int main(int argc, char *argv[]) {
    // Create character buffers
    char inBuf[MAX_INPUT_LEN] = "";
    char commandBuf[MAX_INPUT_LEN] = "";
    char *userInput = &inBuf[0];
    char *command = &commandBuf[0];
    char **args;

    int argCount;

    printf("$ ");

    // Loop until we get CTRL-D
    while (fgets(userInput, MAX_INPUT_LEN, stdin) != NULL) {

        // Remove newline character returned by fgets
        userInput[strcspn(userInput, "\n")] = 0;

        if (userInput[0] != '\0') {
            // Get command, arguments, and arguments length
            getCommandWithArgs(userInput, DELIMITER, command, &args, &argCount);

            // Execute the command
            executeUserCommand(command, &args);

            // Clean up for next iteration
            freeArray((void**) args, argCount);
        }

        printf("$ ");
    }

    printf("\n");
    exit(0);
}


/**
 * Count the number of tokens delimited by the delimeter.
 *
 * @param const char *str: The string to check for tokens
 * @param const char *del: The character that delimits the tokens
 */
int countTokens(const char *str, const char *del) {
    int i;
    int numTokens = -1;
    char delimiter = *del;

    assert(str != NULL && "string cannot be NULL");
    assert(del != NULL && "cannot use a NULL delimiter");

    if (str != NULL && del != NULL) {
        numTokens = 0;
        int length = strlen(str);
        assert(length > 0);

        for (i = 0; i < length; i++) {
            if (str[i] == delimiter) {
               numTokens++; 
            }
        }

        // Because we are counting delimiters, there is one token we missed
        numTokens++;
    }

    return numTokens;
}


/**
 * Extract the user's command and arguments from the input string, as well as
 * the length of the argument array.
 *
 * NOTE: ***args is assigned a pointer to a dynamically allocated string array
 *       and so must be deallocated when finished with.
 *
 * @param char *input: The user's input string
 * @param char *delim: The delimiter demarcating arguments (normally a space)
 * @param char *cmd: A string pointer that gets assigned to the command
 *      extracted from the input string
 * @param char ***args: A pointer to an array of strings that gets assigned to
 *      a dynamically allocated string array containing the command line args
 * @param int *arglen: The length of the dynamic args array returned
 */
void getCommandWithArgs(char *input, char *delim, char *cmd, char ***args, int *arglen) {
    char tokBuf[MAX_INPUT_LEN] = "";
    char *token = &tokBuf[0];
    int i;

    assert(input != NULL && "input string cannot be NULL");
    assert(delim != NULL && "delimiter cannot be NULL");

    if (input != NULL && delim != NULL) {
        // We need to add one for the null item at the end of the array
        *arglen = countTokens(input, delim) + 1;
        assert(*arglen > 0);
        *args = malloc(*arglen * sizeof(char *));
        assert(*args != NULL);

        // Fill dynamic array, with a zero as the last item
        for (i = 0; i < *arglen; i++) {

            // Get token
            if (i == 0) {
                token = strtok(input, delim);
                assert(token != NULL);
                strcpy(cmd, token);
                assert(cmd != NULL);
            } else if (i < *arglen - 1) {
                token = strtok(NULL, delim);
                assert(token != NULL);
            }

            // Put token (or zero) into array
            if (i < *arglen - 1) {
                (*args)[i] = malloc(strlen(token) + 1);
                assert((*args)[i] != NULL);
                strcpy((*args)[i], token);
            } else {
                (*args)[i] = malloc(sizeof(char));
                assert((*args)[i] != NULL);
                (*args)[i] = (char) 0;
            }
        }
    }
}


/**
 * Execute the user's command and arguments with execvp.
 *
 * @param char *cmd: The string containing the user's command
 * @param char ***args: A pointer to an array of strings holding the user's
 *     command along with arguments, and a zero at the end.
 *
 * Example cmd and args for execvp (because it can be tricky)
 *     cmd: "/bin/ls"
 *     args: {"/bin/ls", "-l", "-a", 0}
 */
void executeUserCommand(char *cmd, char ***args) {
    int pid, returnStatus;

    assert(cmd != NULL);
    assert(args != NULL);
    assert(*args != NULL);

    // Fork the current process
    if ((pid = fork()) == 0) {
        returnStatus = execvp(cmd, *args);
        exit(returnStatus);
    } else {
        waitpid(pid, &returnStatus, 0);
        assert(returnStatus != 11 && "execvp received NULL for one or more arguments");

        if (returnStatus == 65280 && cmd != NULL) {
            printf("Unrecognized command.\n", cmd);
        }
    }
}


/**
 * Free a dynamically created array
 *
 * @param void *ary: The array to free
 * @param int len: The length of the array
 */
void freeArray(void **ary, int len) {
    int i;

    if (ary != NULL) {
        for (i = 0; i < len; i++) {
            if (ary[i] != NULL) {
                free(ary[i]);
                ary[i] = NULL;
            }
        }

        free(ary);
        ary = NULL;
    }
}
