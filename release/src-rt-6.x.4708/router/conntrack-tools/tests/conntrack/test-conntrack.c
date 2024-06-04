/* 
 * Very simple test-tool for the command line tool `conntrack'.
 * This code is released under GPLv2 or any later at your option.
 *
 * gcc test-conntrack.c -o test
 *
 * Do not forget that you need *root* or CAP_NET_ADMIN capabilities ;-)
 *
 * (c) 2008 Pablo Neira Ayuso <pablo@netfilter.org>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>

#define CT_PROG "../../src/conntrack"

int main()
{
	int ret, ok = 0, bad = 0, line;
	FILE *fp;
	char buf[1024];
	struct dirent **dents;
	struct dirent *dent;
	char file[1024];
	int i,n;
	char cmd_buf[1024 * 8];
	int i_cmd_buf = 0;
	char cmd, cur_cmd = 0;
	char *cmd_opt;

#define cmd_strappend(_s) do { \
	char * pos = stpncpy(cmd_buf + i_cmd_buf, _s, sizeof(cmd_buf) - i_cmd_buf); \
	i_cmd_buf = pos - cmd_buf; \
	if (i_cmd_buf == sizeof(cmd_buf)) { \
		printf("buffer full!\n"); \
		exit(EXIT_FAILURE); \
	} \
} while (0)

#define cmd_reset() do { \
		i_cmd_buf = 0; \
} while (0)

	n = scandir("testsuite", &dents, NULL, alphasort);

	for (i = 0; i < n; i++) {
		dent = dents[i];

		if (dent->d_name[0] == '.')
			continue;

		sprintf(file, "testsuite/%s", dent->d_name);

		line = 0;

		fp = fopen(file, "r");
		if (fp == NULL) {
			perror("cannot find testsuite file");
			exit(EXIT_FAILURE);
		}

		while (fgets(buf, sizeof(buf), fp)) {
			char *res;
			line++;

			if (buf[0] == '#' || buf[0] == ' ')
				continue;

			res = strchr(buf, ';');
			if (!res) {
				printf("malformed file %s at line %d\n", 
					dent->d_name, line);
				exit(EXIT_FAILURE);
			}
			*res = '\0';
			res++;
			for (; *res == ' ' || *res == '\t'; res++);
			cmd = res[0];
			cmd_opt = &res[1];
			for (; *cmd_opt == ' ' || *cmd_opt == '\t'; cmd_opt++);
			res = strchr(cmd_opt, '\n');
			if (res)
				*res = '\0';

			if (cur_cmd && cmd != cur_cmd) {
				/* complete current multi-line command */
				switch (cur_cmd) {
				case '\n':
					cmd_strappend("\" | ");
					break;
				default:
					printf("Internal Error: unexpected multiline command %c",
							cur_cmd);
					exit(EXIT_FAILURE);
					break;
				}

				cur_cmd = 0;
			}

			switch (cmd) {
			case '\n':
				if (!cur_cmd) {
					cmd_strappend("echo \"");
					cur_cmd = cmd;
				} else
					cmd_strappend("\n");
				cmd_strappend(buf);
				continue;
			default:
				cmd_strappend(CT_PROG);
				cmd_strappend(" ");
				cmd_strappend(buf);
				if (cmd == '|') {
					cmd_strappend(" | ");
					if (cmd_opt[0]) {
						cmd_strappend("sed \"");
						cmd_strappend(cmd_opt);
						cmd_strappend("\" | ");
					}
					continue;
				}
				cmd_reset();
				break;
			}

			printf("(%d) Executing: %s\n", line, cmd_buf);

			fflush(stdout);
			ret = system(cmd_buf);

			if (WIFEXITED(ret) &&
			    WEXITSTATUS(ret) == EXIT_SUCCESS) {
				if (cmd == 'O')
					ok++;
				else {
					bad++;
					printf("^----- BAD\n");
				}
			} else {
				if (cmd == 'B')
					ok++;
				else {
					bad++;
					printf("^----- BAD\n");
				}
			}
			printf("=====\n");
		}
		fclose(fp);
	}

	for (i = 0; i < n; i++)
		free(dents[i]);

	free(dents);

	fprintf(stdout, "OK: %d BAD: %d\n", ok, bad);
}
