#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// MindSync

int
main(int argc, char *argv[]) {
	if (argc != 2) {
		printf("usage: sleep seconds\n");
		exit(1);
	}
	int time = atoi(argv[1]);

	sleep(time);
	exit(0);
}


