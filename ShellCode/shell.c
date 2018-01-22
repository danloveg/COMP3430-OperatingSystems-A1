/*
 * shell.c
 *
 * Author: Daniel Lovegrove
 *
 * Version: January 22/2018
 *
 * Purpose: Get familiar with fork and exec. This program takes user input and
 * executes what the user enters until they enter Ctrl-D, at which point the
 * program exits.
 */


#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "shell.h"
#include "shellvars.h"
#include "shellstring.h"
#include "shellfileio.h"


int main(int argc, char *argv[]) {
    char inBuf[MAX_INPUT_LEN] = "";
    char commandBuf[MAX_INPUT_LEN] = "";
    char *userInput = &inBuf[0];
    char *command = &commandBuf[0];
    char **args;
    int argCount;

    // Enable shell variables
    initShellVarProg();
    loadShellVariablesFromFile();

    printf("$ ");

    // Loop until we get CTRL-D
    while (fgets(userInput, MAX_INPUT_LEN, stdin) != NULL) {

        removeTrailingNewline(userInput);

        if (userInput[0] != '\0') {
            // Get command, arguments, and arguments length
            getCommandWithArgs(userInput, DELIMITER, command, &args, &argCount);

            // Execute the command
            executeUserCommand(command, &args, argCount);

            // Clean up for next iteration
            freeArray((void**) args, argCount);
        }

        printf("$ ");
    }

    printf("\n");

    quitShellVarProg();
    exit(0);
}


/**
 * Loads the shell variables from the file .shell_init at the user's home
 * directory.
 */
void loadShellVariablesFromFile() {
    if (openShellInitFile() == 0) {
        char *line = NULL;

        do {
            line = readFileString();
            tryAddVariable(line, SUPPRESS_OUTPUT);
            free(line);
        } while (line != NULL);

        closeShellInitFile();
    }
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
 * Execute the user's command with arguments.
 *
 * @param char *cmd: The string containing the user's command
 * @param char ***args: A pointer to an array of strings holding the user's
 *     command along with arguments, and a zero at the end.
 *
 * Example cmd and args for execvp (because it can be tricky)
 *     cmd: "/bin/ls"
 *     args: {"/bin/ls", "-l", "-a", 0}
 */
void executeUserCommand(char *cmd, char ***args, int arglen) {
    int pid, returnStatus;

    assert(cmd != NULL);
    assert(args != NULL);
    assert(*args != NULL);

    if (strcmp(cmd, SET_COMMAND) == 0) {
        // User used set command, try to set shell variable:
        setShellVariableFromArgs(cmd, args, arglen);
    } else {
        // Substitute any shell variables...
        bool substitutionPassed = substituteShellVariables(args, arglen);

        // Execute if all shell variables were valid
        if (substitutionPassed == true) {
            // Fork the current process
            if ((pid = fork()) == 0) {
                returnStatus = execvp(cmd, *args);
                exit(returnStatus);
            } else {
                waitpid(pid, &returnStatus, 0);
                assert(returnStatus != 11 && "execvp received NULL for one or more arguments");

                if (returnStatus == 65280 && cmd != NULL) {
                    printf("Unrecognized command.\n");
                }
            }
        }
    }
}


/**
 * Set a new shell variable from the user's command and args.
 *
 * @param char *cmd: The user's command
 * @param char ***args: The user's arguments in execv format
 * @param int arglen: The length of the args array
 */
void setShellVariableFromArgs(char *cmd, char ***args, int arglen) {
    assert(cmd != NULL);
    assert(args != NULL);
    assert(*args != NULL);

    if (arglen != 3) {
        printf("set requires one and only one argument.\n");
    } else {
        tryAddVariable((*args)[1], INTERACTIVE);
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
