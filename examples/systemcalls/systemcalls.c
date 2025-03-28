#include "systemcalls.h"

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{

/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/
    int ret;

    // Execute the command using system()
    ret = system(cmd);

    // Check if the system() call succeeded
    if (ret == -1) {
        perror("Error: system() call failed");
        return false;
    }

    // Handle the return value from the command
    if (WIFEXITED(ret)) {
        int exit_status = WEXITSTATUS(ret);
        printf("Command executed successfully with exit status: %d\n", exit_status);
        if (exit_status != 0) {
            printf("Command returned a non-zero exit status, indicating an error.\n");
        }
    } else if (WIFSIGNALED(ret)) {
        printf("Command terminated by signal: %d\n", WTERMSIG(ret));
    } else {
        printf("Command did not terminate normally.\n");
    }

    return true;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/
    va_end(args);

    pid_t pid = fork();
    if (pid < 0) {
        // Fork failed
        perror("Error: fork() failed");
        return false;
    } else if (pid == 0) {
        // Child process: Execute the command
        if (execv(command[0], command) == -1) {
            perror("Error: execv() failed");
            exit(EXIT_FAILURE); // Exit child process with failure
        }
    } else {
        // Parent process: Wait for the child process to finish
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("Error: waitpid() failed");
            return false;
        }

        // Check the exit status of the child process
        if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            return exit_status == 0; // Return true if the command succeeded
        } else {
            return false; // Command did not terminate normally
        }
    }
    return true;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];


/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/

    va_end(args);

        pid_t pid = fork();
    if (pid < 0) {
        // Fork failed
        perror("Error: fork() failed");
        return false;
    } else if (pid == 0) {
        // Child process: Redirect standard output to the file
        int fd = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror("Error: open() failed");
            exit(EXIT_FAILURE); // Exit child process with failure
        }

        // Redirect stdout to the file
        if (dup2(fd, STDOUT_FILENO) < 0) {
            perror("Error: dup2() failed");
            close(fd);
            exit(EXIT_FAILURE); // Exit child process with failure
        }

        close(fd); // Close the file descriptor as it's no longer needed

        // Execute the command
        if (execv(command[0], command) == -1) {
            perror("Error: execv() failed");
            exit(EXIT_FAILURE); // Exit child process with failure
        }
    } else {
        // Parent process: Wait for the child process to finish
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("Error: waitpid() failed");
            return false;
        }

        // Check the exit status of the child process
        if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            return exit_status == 0; // Return true if the command succeeded
        } else {
            return false; // Command did not terminate normally
        }
    }
    return true;
}
