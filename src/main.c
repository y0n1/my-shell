#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define COMMAND_MAX_LENGTH 256
#define MAX_COMMANDS_COUNT 16
#define NEWLINE_CHAR '\n'
#define NULL_CHAR '\0'
#define PIPE_CHAR '|'
#define AMP_CHAR '&'
#define AMP_STR "&"
#define SPACE_STR " "
#define CD_CMD_STR "cd"
#define EXIT_CMD_STR "exit"
#define COMMAND_PROMPT_STR "> "
#define GOOD_BYE_MSG "\nThank you for using my shell!\n"
#define BAD_SYNTAX_NEAR_TOKEN "%s: syntax error near unexpected token `%c'\n"

typedef enum {
	false,
	true
} bool;

/**
 * Prints the current folder location and command line prompt.
 */
void show_cmd_prompt() {
	char *currDir = getcwd(0, 0);
	printf("%s%s", currDir, COMMAND_PROMPT_STR);
}

/**
 * Trims white spaces from the given string and
 * Returns the length of the resulting string.
 */
size_t trim_ws(char *i_String) {
	char *end;

	/* Trim leading spaces */
	while (isspace(*i_String)) {
		i_String++;
	}

	/* Trim trailing space */
	end = i_String + strlen(i_String) - 1;
	while (end > i_String && isspace(*end)) {
		end--;
	}

	/* Write string terminator */
	*(end + 1) = NULL_CHAR;

	return strlen(i_String);
}

/**
 * Checks if the given string is not the empty string.
 */
bool is_not_empty_string(const char *i_String) {
	return strcmp(i_String, SPACE_STR) != 0;
}

/**
 * Parses a string (i_FullCmd) into and array of tokens (o_CmdsArray).
 */
size_t get_tokens(char *i_FullCmd, const char *i_Delimiter, char **o_CmdsArray) {
	char *token = NULL;
	size_t size = 0;
	size_t tokenLength = 0;

	token = strtok(i_FullCmd, i_Delimiter);
	while (token != NULL) {
		tokenLength = trim_ws(token);
		if (is_not_empty_string(token)) {
			/* store each command in its own cell */
			o_CmdsArray[size] = malloc((tokenLength + 1) * sizeof (char));
			strcpy(o_CmdsArray[size++], token);
		}
		/* retrieve the next token */
		token = strtok(NULL, AMP_STR);
	}

	/* append string terminator */
	o_CmdsArray[size] = NULL_CHAR;

	return size;
}

/**
 * Gets a full-command and returns only the command without its arguments.
 * This function allocates memory for the returned string!
 */
char *get_command(const char *i_Cmd) {
	char *buffer = malloc((strlen(i_Cmd) + 1) * sizeof (char));

	strcpy(buffer, i_Cmd);
	strtok(buffer, SPACE_STR);

	return buffer;
}

/**
 * Gets a full command and returns only the arguments without the command.
 * This function allocates memory for the returned string!
 */
char *get_arguments(const char *c_Cmd) {
	char *buffer = malloc((strlen(c_Cmd) + 1) * sizeof (char));

	strcpy(buffer, c_Cmd);
	while (isspace(*buffer)) {
		buffer++;
	}

	return buffer;
}

/**
 * Checks whether the given PID belongs to the parent or the child process.
 */
bool is_child(pid_t i_Pid) {
	return i_Pid == 0;
}

/**
 * Executes a process, possibly in background.
 */
int execute(char *command, bool i_runsInBackground) {
	pid_t pid = 0;
	int status = 0;
	char *commandName = get_command(command);
	char **argv;

	get_tokens(command, SPACE_STR, argv);
	if (i_runsInBackground) {
		pid = fork();
		if (is_child(pid)) {
			execvp(commandName, argv);
			fprintf(stderr, "ERROR: cannot start program %s\n", commandName);
			return EXIT_FAILURE;
		} else {
			printf("[1] %d\n", pid);
			waitpid(pid, &status, 0);
		}
	} else {
		execvp(commandName, argv);
	}

	return WEXITSTATUS(status);
}

/**
 * Detects the exit command.
 */
bool is_not_exit(const char *i_Cmd) {
	return strcmp(EXIT_CMD_STR, i_Cmd) != 0;
}

/**
 * Detects the 'cd' command
 */
bool is_chdir(const char *i_Cmd) {
	char *cmdName = get_command(i_Cmd);
	bool result = strcmp(cmdName, CD_CMD_STR) == 0;
	free(cmdName);

	return result;
}

/**
 * Checks if the last character in i_CmdBuffer is '&'. This function assumes
 * that the string i_CmdBuffer has been trimmed properly already.
 */
bool has_bg_flag(const char *i_CmdBuffer) {
	size_t length = strlen(i_CmdBuffer);
	char lastChar = *(i_CmdBuffer + length - 1);

	return lastChar == AMP_CHAR;
}

/**
 * Reads the next command from the command line.
 */
size_t read_cmd(char *io_Buffer, bool *o_BgFlagStatus) {
	fgets(io_Buffer, COMMAND_MAX_LENGTH, stdin);
	trim_ws(io_Buffer);
	*o_BgFlagStatus = has_bg_flag(io_Buffer);

	return strlen(io_Buffer);
}

void free_all(char **arr, size_t lenArr) {
	int i;

	for (i = 0; i < lenArr; i++) {
		free(arr[i]);
	}
}

/**
 * main function.
 */
int main(int argc, char **argv) {
	bool bgFlag = false;
	size_t cmdsArraySize = 0;
	size_t argsArraySize = 0;
	char *currentCommand = NULL;
	char fullCmd[COMMAND_MAX_LENGTH];
	char *cmdsArray[MAX_COMMANDS_COUNT];
	int index;

	// main shell loop.
	while (is_not_exit(fullCmd)) {

		// scans the users input
		show_cmd_prompt();

		// reads the command and sets the background flag
		read_cmd(fullCmd, &bgFlag);

		/* Check for invalid tokens */
		if (*fullCmd == AMP_CHAR || *fullCmd == PIPE_CHAR) {
			printf(BAD_SYNTAX_NEAR_TOKEN, argv[0], *fullCmd);
			continue;
		}

		/* If user just pressed enter then skip */
		if (*fullCmd == NEWLINE_CHAR) {
			continue;
		}

		// read the entire command into an array
		cmdsArraySize = get_tokens(fullCmd, AMP_STR, cmdsArray);

		// process the parsed input of commands
		for (index = 0; index < cmdsArraySize; ++index) {
			currentCommand = cmdsArray[index];

			// checks if exit received.
			if (!is_not_exit(currentCommand)) {
				strcpy(fullCmd, currentCommand);
				break;
			}

			// in case of change directory is called
			if (is_chdir(currentCommand)) {
				chdir(get_arguments(currentCommand));
			}

			// runs the command.
			execute(currentCommand, bgFlag);
		}
		free_all(cmdsArray, cmdsArraySize);
	}
	printf(GOOD_BYE_MSG);

	return EXIT_SUCCESS;
}
