#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// MindSync

int
main(int argc, char *argv[]) {
	if (argc != 2) {
		printf("usage: sleep seconds\n");
		exit(1); // 1 mean error occur
	}
	int time = atoi(argv[1]); // turn string to int

	sleep(time); // system call
	exit(0); // don't forget to exit
}


