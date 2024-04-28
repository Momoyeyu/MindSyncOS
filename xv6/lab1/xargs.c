#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int readline(char *new_argv[32], int cur_argv) {
	char buf[1024];
	int n = 0;
	while (read(0, buf + n, 1)) {
		if (n == 1023) {
			fprintf(2, "too long argv\n");
			exit(1);
		}
		if (buf[n] == '\n')
			break;
		n++;
	}
	if (n == 0) return 0;
	buf[n] = 0;
	int offset = 0;
	while(offset < n) {
		new_argv[cur_argv++] = buf + offset;
		while(buf[offset] != ' ' && offset < n) {
			offset++;
		}
		while(buf[offset] == ' ' && offset < n) {
			buf[offset++] = 0;
		}
	}
	return cur_argv;
}

int main(int argc, char const *argv[]) {
	if (argc < 2) {
		fprintf(2, "usage: xargs command (arg ...)\n");
		exit(1);
	}
	char *command = malloc(strlen(argv[1]) + 1);
	char *new_argv[MAXARG];
	strcpy(command, argv[1]);
	for(int i = 1; i < argc; i++) {
		new_argv[i - 1] = malloc(strlen(argv[i]) + 1);
		strcpy(new_argv[i - 1], argv[i]);
	}
	int cur_argv;
	while((cur_argv = readline(new_argv, argc - 1)) != 0) {
		new_argv[cur_argv] = 0;
		if (fork() == 0) {
			exec(command, new_argv);
			// if we fail:
			fprintf(2, "exec fail");
			exit(1);
		}
		wait(0); // wait sub process
	}

	exit(0);
}