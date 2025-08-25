#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>	// isdigit(), isblank()

char *lines[2000], line[BUFSIZ];
int a1, a2, a3, last_line, curr_line;
bool have_a1, have_a2, have_a3;	// Did we parse out addresses from the last command line read in?
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
	if (!have_a1) { a2 = a1 = curr_line; have_a1 = true; }
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
	if (!have_a1) { a2 = a1 = curr_line; have_a1 = true; }
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
	if (!have_a1) { a2 = a1 = curr_line; have_a1 = true; }
	for (int i = a1; i <= a2; i++) printf("%d\t%s", i, lines[i]);
	curr_line = a2;	// '.' = last valid line printed
	return true;
}

// (.,.)p: Print line numbers plainly, without their line numbers
bool cmd_print() {
	if (!have_a1) { a2 = a1 = curr_line; have_a1 = true; }
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
	if (!have_a1) { a1 = 1; a2 = last_line; have_a1 = true; }
	FILE *fp = fopen(filename, "w");
	if (!fp) return false;
	size_t bytes_written = 0;
	for (int i = a1; i <= a2; i++) {
		fputs(lines[i], fp);
		bytes_written += strlen(lines[i]);
	}
	fclose(fp);
	// TODO: Check future "-s" command line flag
	printf("%d\n", (int)bytes_written);
	return true;
}

// ($)=: Print line number of given address
bool cmd_print_address() {
	if (!have_a1) { a2 = a1 = last_line; have_a1 = true; }
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

// Parse next address out from input line
bool next_address(char **ln, int *addr) {
	*addr = -1;
	char *s = *ln;
	for (bool first = true; ; first = false) {
		while (isblank(*s)) s++;
		char c = *s;
		switch (c) {
			case '$': case '.':
				if (!first) return false;	// Needs to be first component in address
				*addr = (c == '$') ? last_line : curr_line;	// '$' or '.'
				s++;
			break;

			case '/': case '?':
				if (!first) return false;
				// TODO:
				s++;
			break;

			case '+': case '-':
				if (first) *addr = curr_line;	// Else will offset from current line (addr)
				// Set addr + or - 1 if single +/-, else +/- number after the sign
				s++;
				while (isblank(*s)) s++;
				if (!isdigit(*s)) *addr += (c == '+') ? 1 : -1;
				else {
					*addr += (c == '+') ? atoi(s) : -atoi(s);
					while (isdigit(*s)) s++;
				}
			break;

			case '\'':
			// TODO:
			break;

			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
				// Number, either set addr to this number, or offset from current address
				if (first) *addr = atoi(s);
				else *addr += atoi(s);
				while (isdigit(*s)) s++;
			break;

			default:
			goto done;	// No more address components, leave loop
			break;
		}
	}
	done:
	if (*addr < -1 || *addr > last_line) return false;
	*ln = s;	// Update line to skip consumed characters
	return true;
}

// Get next line range and command to run
bool next_command() {
	// Print prompt by default
	printf("*"), fflush(stdout);
	// Read in next line addresses and command to run
	if (!fgets(line, sizeof line, stdin)) return cmd_quit();	// Done with program

	a1 = a2 = a3 = 0;	// Reset addresses
	have_a1 = have_a2 = have_a3 = false;
	cmd = '\0';	// Reset command
	char *s = line;	// Command line to parse into addresses, etc.
	if (!next_address(&s, &a1)) return false;
	have_a1 = (a1 > -1);	// Did we read in the 1st address?
	while (*s == ',' || *s == ';') {
		// Have multiple addresses, also parse them out or set defaults as needed for ','/';'
		if (have_a2) a1 = a2;
		if (!have_a1) a1 = (*s == ',') ? 1 : curr_line;	// Default ',' = 1,$ : else ';' = .,$
		if (*s == ';') curr_line = a1;
		s++;	// Skip ,;

		// Get next address 2
		if (!next_address(&s, &a2)) return false;
		have_a2 = (a2 > -1);	// Did we read in the next address 2?
		if (!have_a2) a2 = (have_a1) ? a1 : last_line;
		have_a2 = have_a1 = true;	// Got both addresses now
	}
	if (!have_a2) a2 = a1;	// Default to 1st address if needed, e.g. for (.,.) command default lines

	// Get command to run
	cmd = *s++;
	if (!cmd_functions[cmd]) return false;	// Invalid/unsupported command
	while (isblank(*s)) s++;
	cmd_parms = s;
	return true;
}

int main(int argc, char **argv) {
	if (argc < 2 || !argv[1]) exit(EXIT_FAILURE);
	// Read in input file to line buffer
	filename = argv[1];
	FILE *fp = fopen(filename, "r");
	if (!fp) exit(EXIT_FAILURE);
	size_t bytes_read = 0;
	while (fgets(line, sizeof line, fp)) {
		lines[++last_line] = str_dup(line);
		bytes_read += strlen(line);
	}
	fclose(fp);
	printf("%d\n", (int)bytes_read);
	curr_line = last_line;	// Set '.' initially to last line read in from file

	// Input loop to read next command to run on lines
	while (true) {
		// Run command on lines
		if (!next_command() || !cmd_functions[cmd]()) {
			puts("?");
			// TODO: Print error text if user has done 'H' command
		}
	}

	// Quit program
	cmd_quit();
}
