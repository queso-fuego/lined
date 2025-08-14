#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char *lines[2000], line[BUFSIZ];
int a1, a2, a3, last_line, curr_line;
char cmd, *filename, *cmd_parms;

char *str_dup(char *s) {
	char *new = malloc(strlen(s) + 1);
	if (!new) return NULL;
	strcpy(new, s);
	return new;
}

int main(int argc, char **argv) {
	if (argc < 2 || !argv[1]) exit(EXIT_FAILURE);
	// Read in input file to line buffer
	filename = argv[1];
	FILE *fp = fopen(filename, "r");
	if (!fp) exit(EXIT_FAILURE);
	while (fgets(line, sizeof line, fp)) lines[++last_line] = str_dup(line);
	fclose(fp);

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
		switch (cmd) {
			case 'a':	// Append lines to the buffer
			while (fgets(line, sizeof line, stdin)) {
				if (!strcmp(line, ".\n")) break;	// Done inputting lines with single '.'
				a1++;	// Will append after given address 1
				// Move current buffer lines to make room for new line
				memmove(&lines[a1+1], &lines[a1], (last_line - a1 + 1) * sizeof *lines);
				lines[a1] = str_dup(line);
				last_line++;	// Added new line to buffer
			}
			break;

			case 'd': 	// Delete lines from the buffer
			// Free line memory 1st to not have leaks
			for (int i = a1; i <= a2; i++) free(lines[i]);
			// Overwrite deleted lines that have now been freed up
			memmove(&lines[a1], &lines[a2+1], (last_line - a2) * sizeof *lines);
			last_line -= a2 - a1 + 1;	// Removed lines from buffer
			break;

			case 'n':	// Print lines with line number
			for (int i = a1; i <= a2; i++) printf("%d\t%s", i, lines[i]);
			break;

			case 'p':	// Print lines without line number
			for (int i = a1; i <= a2; i++) printf("%s", lines[i]);
			break;

			case 'q':	// Quit program
			goto done;
			break;

			case 'w':	// Write lines to file
			fp = fopen(filename, "w");
			// TODO: Use actual line range a1 to a2, not all lines in buffer
			for (int i = 1; i <= last_line; i++) fputs(lines[i], fp);
			fclose(fp);
			break;
		}
	}
	done:
	// Free remaining line memory to be nice to the OS [|:^)
	for (int i = 1; i <= last_line; i++) free(lines[i]);
}
