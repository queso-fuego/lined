#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char *lines[2000], line[BUFSIZ];
int a1, a2, a3, last_line, curr_line;
unsigned char cmd;
char *filename, *cmd_parms;

char *str_dup(char *s) {
	char *new = malloc(strlen(s) + 1);
	if (!new) return NULL;
	strcpy(new, s);
	return new;
}

// (.)a: Append new lines to buffer after given address
bool cmd_append() {
	while (fgets(line, sizeof line, stdin)) {
		if (!strcmp(line, ".\n")) break;	// Done inputting lines with single '.'
		a1++;	// Will append after given address 1
		// Move current buffer lines to make room for new line
		memmove(&lines[a1+1], &lines[a1], (last_line - a1 + 1) * sizeof *lines);
		lines[a1] = str_dup(line);
		last_line++;	// Added new line to buffer
	}
	curr_line = a1;	// '.' = last line added to buffer
	return true;
}

// (.)i: Same as cmd_append, but add _at_ the given address, _not_ after it
bool cmd_insert() {
	if (a1 > 0) a1--;
	return cmd_append();
}

// (.,.)d: Delete line range from buffer
bool cmd_delete() {
	// Free line memory 1st to not have leaks
	for (int i = a1; i <= a2; i++) free(lines[i]);
	// Overwrite deleted lines that have now been freed up
	memmove(&lines[a1], &lines[a2+1], (last_line - a2) * sizeof *lines);
	last_line -= a2 - a1 + 1;	// Removed lines from buffer
	curr_line = (a1 < last_line) ? a1 : last_line;	// '.' = line after deleted range, or new last line in buffer
	return true;
}

// (.,.)c: Delete then add new lines in their place, thereby "changing" those lines
bool cmd_change() {
	return cmd_delete() && cmd_insert();
}

// (.,.)n: Print lines with their line numbers
bool cmd_print_with_line_number() {
	for (int i = a1; i <= a2; i++) printf("%d\t%s", i, lines[i]);
	curr_line = a2;	// '.' = last valid line printed
	return true;
}

// (.,.)p: Print line numbers plainly, without their line numbers
bool cmd_print() {
	for (int i = a1; i <= a2; i++) printf("%s", lines[i]);
	curr_line = a2;	// '.' = last valid line printed
	return true;
}

// q: Quit/End the program
bool cmd_quit() {
	// Free remaining line buffer memory to be nice to the OS [|:^)
	for (int i = 1; i <= last_line; i++) free(lines[i]);
	exit(EXIT_SUCCESS);
	return false;
}

// (1,$)w: Write lines to current filename
bool cmd_write() {
	FILE *fp = fopen(filename, "w");
	if (!fp) return false;
	// TODO: Use actual line range a1 to a2, not all lines in buffer
	for (int i = 1; i <= last_line; i++) fputs(lines[i], fp);
	fclose(fp);
	return true;
}

// ($)=: Print line number of given address
bool cmd_print_address() {
	printf("%d\n", a1);
	return true;
}

bool (*cmd_functions[256])() = {
	['a'] = cmd_append,	// (.)a
	['c'] = cmd_change,	// (.,.)c
	['d'] = cmd_delete,	// (.,.)d
	['i'] = cmd_insert,	// (.)i
	['n'] = cmd_print_with_line_number,	// (.,.)n
	['p'] = cmd_print,	// (.,.)p
	['q'] = cmd_quit,	// q
	['w'] = cmd_write,	// (1,$)w
	['='] = cmd_print_address,	// ($)=
};

int main(int argc, char **argv) {
	if (argc < 2 || !argv[1]) exit(EXIT_FAILURE);
	// Read in input file to line buffer
	filename = argv[1];
	FILE *fp = fopen(filename, "r");
	if (!fp) exit(EXIT_FAILURE);
	while (fgets(line, sizeof line, fp)) lines[++last_line] = str_dup(line);
	fclose(fp);
	curr_line = last_line;	// Set '.' initially to last line read in from file

	// Input loop to read next command to run on lines
	while (true) {
		// Print prompt by default
		printf("*"), fflush(stdout);
		// Read in next line addresses and command to run
		if (!fgets(line, sizeof line, stdin)) break;	// Done with program
		if (sscanf(line, "%d,%d%c", &a1, &a2, &cmd) < 3) {
			puts("?");	// Ed school of error handling
			puts("Invalid input, need 'line1,line2cmd'");
			continue;
		}
		// Validate input a little bit
		if (a1 > a2 || a1 < 0 || a1 > last_line || a2 < 0 || a2 > last_line) {
			puts("?");
			puts("Addresses must be in ascending order and in range [0,$]");
			continue;
		}
		// Run command on lines
		if (!cmd_functions[cmd] || !cmd_functions[cmd]()) {
			puts("?");
			// TODO: Print error text if user has done 'H' command
		}
	}

	// Quit program
	cmd_quit();
}
