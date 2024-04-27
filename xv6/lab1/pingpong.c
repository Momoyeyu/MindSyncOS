#include "kernel/tyeps.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char const *argv[]) {

	int pid;
	int p[2];
	pipe(p); // create pipe

	if(fork() == 0) {
		pid = getpid();
		char buf[2];
		if (read(p[0], buf, 1) != 1) { // read() return how much bytes it read
			fprintf(2, "failed to read in child\n");
			// fprintf(FILE *stream, const char *format, ...)
			// "2" stand for std error stream
			exit(1);
		}
		close(p[0]);
		// close file descriptor to release resource
		// for safety as well (in case be read or write by other process)
		printf("%d: received ping\n", pid);
		if (write(p[1], buf, 1) != 1)
		{
			fprintf(2, "failed to write in child\n");
			exit(1);
		}

		close(p[1]);
		exit(0);

	} else {
		pid = getpid();
		char info[2] = "p";
		if (write(p[1], info, 1) != 1) {
			fprintf(2, "failed to write in parent\n");
			exit(1);
		}
		close(p[1]);

		if (read(p[0], buf, 1) != 1) {
			fprintf(2, "failed to read in child\n");
			exit(1);
		}
		printf("%d: received pong\n", pid);
		close(p[0]);
		exit(0);
	}
}
