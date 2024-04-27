#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void primes(int p[2]) {
	int prime;
	int flag;
	int n;
	close(p[1]); // we won't write to parent
	if (read(p[0], &prime, 4) != 4) {
		fprintf(2, "child process failed to read\n");
		exit(1);
	}
	printf("prime %d", prime);
	flag = read(p[0], &n, 4);
	// wait for the first n from parent
	if (flag) {
		int np[2];
		pipe(np);
		if (fork() == 0) {
			primes(np);
		} else {
			close(np[0]);
			if (n % prime != 0) {
				write(np[1], &n, 4);
			}
			while(read(p[0], &n, 4)) {
				if (n % prime != 0) {
					write(np[1], &n, 4);
				}
			}
			close(p[0]);
			close(np[1]);
			wait(0);
			// wait for child process
		}
	}
	exit(0);
}

int main(int argc, char const *argv[]) {
	int [2];
	pipe(p);
	if (fork() == 0) {
		primes(p);
	} else {
		close(p[0]); // we won't read from child
		for (int i = 2; i < 36; i++) {
			if (write(p[1], &i, 4) != 4) {
				fprintf(2, "first process failed to write %d into the pipe\n", i);
				exit(1)
			}
		}
		close(p[1]);
		wait(0); // wait for child process
	}
	
	exit(0);
}
