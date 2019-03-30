#define _GNU_SOURCE
#include <sys/signalfd.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sched.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/ipc.h>

#include "glib-2.0/glib.h"
#include "util.h"

#define LEN						(64UL)
#define WORKER_NUM				(2UL)
#define THREAD1_SIZE			(1)

struct fill {
	char str[LEN];	//64 bytes
};//cache line size of Jupiter is 64 bytes.

static pthread_t worker[WORKER_NUM];
struct fill *thread1_data;
static int thread1_cpuid;

void usage(void) {
	printf("Usage:\n\t./seq thread_cpuid(0-79)\n");
}

void init_thread1_data(void) {
	uint64_t i = 0;
	uint64_t j = 0;
	char _j;

	thread1_data = (struct fill *) malloc(sizeof(struct fill) * THREAD1_SIZE * 1024 * 16);
	if (thread1_data == NULL)
		handle_error("malloc error!");

	for (i = 0; i < THREAD1_SIZE*1024*16; i++) {
		for (j = 0; j < LEN; j++) {
			thread1_data[i].str[j] = '%';
		}
	}
	
	//warm up
	for (j = 0; j < 600; j++) {
		for (i = 0; i < THREAD1_SIZE*1024*16; i++) {
			thread1_data[i].str[1] = '%';
			thread1_data[i].str[30] = '%';
			_j = thread1_data[i].str[63] = '%';
		}
	}
}

void *worker_thread1(void *arg) {
	uint64_t i = 0;
	uint64_t j = 0;
	uint64_t _i = 0;
	char _j;
	uint64_t __i;
	uint64_t len = 0;
	long index = 0;
	long _index = 0;
	set_affinity(thread1_cpuid);
	uint64_t coreid = get_affinity();
	printf("thread is on core %lu\n", coreid);
	
	init_thread1_data();

	uint64_t start = debug_time_usec(); 
	for (j = 0; j < 60000; j++) {
		for (i = 0; i < THREAD1_SIZE*1024*16; i++) {
			_j = thread1_data[i].str[0];
		}
	}
	uint64_t time_diff = debug_diff_usec(start);
	printf("Time diff is %lf seconds\n", (double) (time_diff/1000000.0));
	printf("Cache bandwidth is %lf GB/s.\n",
			(double)((j-1)*THREAD1_SIZE/1024)/(double)time_diff*1000000.0);

}

void free_resource(void) {
	if (thread1_data != NULL) free(thread1_data);
}

void sig_handler(int signo) {
	if (signo == SIGINT) {
		printf("Free resource ...\n");
		free_resource();
	} else
		handle_error("Signal Error!\n");

	exit(EXIT_SUCCESS);
}

void init_worker_thread(void) {
	int ret = 0;

	ret = pthread_create(&(worker[0]), NULL, worker_thread1, NULL);
	if (ret != 0) {
		printf("Pthread create error!\n");
		exit(EXIT_SUCCESS);
	}

}

int main(int argc, char **argv) {
	if (argc != 2) {
		usage();
		return -1;
	}

	thread1_cpuid = atoi(argv[1]);
	printf("thread_cpuid: %d\n", thread1_cpuid);

	init_worker_thread();
	
	if (signal(SIGINT, sig_handler) == SIG_ERR) {
			handle_error("SIGINT error!\n");
	}
	pthread_join(worker[0], NULL);
	free_resource();
	return 0;
}
