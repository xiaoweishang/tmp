#define _GNU_SOURCE
#define _LARGEFILE64_SOURCE

#include <sys/time.h>
#include <execinfo.h>
#include <inttypes.h>
#include <time.h>
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
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/ipc.h>

#include "util.h"
#include "glib-2.0/glib.h"

sem_t sem_main;
sem_t sem_worker;

pthread_cond_t worker_cond  = PTHREAD_COND_INITIALIZER;
pthread_cond_t main_cond  = PTHREAD_COND_INITIALIZER;
pthread_mutex_t worker_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t main_mutex = PTHREAD_MUTEX_INITIALIZER;

uint64_t debug_time_usec(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (uint64_t) tv.tv_sec * 1000000lu + tv.tv_usec;
}

uint64_t debug_time_monotonic_nsec(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t) ((uint64_t) ts.tv_sec * 1000000000lu + (uint64_t)ts.tv_nsec);
}

uint64_t debug_time_monotonic_usec(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t) ((uint64_t) ts.tv_sec * 1000000lu + (uint64_t) ((double) ts.tv_nsec / 1000.0));
}

double debug_time_sec(void) {
  const uint64_t usec = debug_time_usec();
  return ((double)usec) / 1000000.0;
}

uint64_t debug_diff_usec(const uint64_t last) {
  return debug_time_usec() - last;
}

double debug_diff_sec(const double last) {
  return debug_time_sec() - last;
}

uint64_t debug_tv_diff(const struct timeval * const t0, const struct timeval * const t1) {
  return UINT64_C(1000000) * (t1->tv_sec - t0->tv_sec) + (t1->tv_usec - t0->tv_usec);
}

void debug_print_tv_diff(char * tag, const struct timeval t0, const struct timeval t1) {
  printf("%s: %" PRIu64 " us\n", tag, debug_tv_diff(&t0, &t1));
}

void my_sleep(uint64_t microseconds) {
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	ts.tv_nsec += microseconds * 1000;

	if (ts.tv_nsec >= 1000000000) {
		ts.tv_nsec -= 1000000000;
		ts.tv_sec += 1;
	}
	clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL);
}

int get_vcpu_count(void) {
	return get_nprocs();
}

uint64_t get_pid_affinity(int pid) {
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);

	int s = sched_getaffinity(pid, sizeof(cpu_set_t), &cpuset);
	if (s != 0) return -1;
	uint64_t j = 0;
	uint64_t _j = 0;
	for (j = 0; j < CPU_SETSIZE; j++)
		if (CPU_ISSET(j, &cpuset)) {
			_j = j;
		}
	return _j;
}


uint64_t get_affinity(void) {
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);

	int s = pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
	if (s != 0) return -1;
	uint64_t j = 0;
	uint64_t _j = 0;
	for (j = 0; j < CPU_SETSIZE; j++)
		if (CPU_ISSET(j, &cpuset)) {
			_j = j;
		}
	return _j;
}

uint64_t get_affinity_out(pthread_t tid) {
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);

	int s = pthread_getaffinity_np(tid, sizeof(cpu_set_t), &cpuset);
	if (s != 0) return -1;
	uint64_t j = 0;
	uint64_t _j = 0;
	for (j = 0; j < CPU_SETSIZE; j++)
		if (CPU_ISSET(j, &cpuset)) {
			_j = j;
		}
	return _j;
}

void set_pid_affinity(uint64_t vcpu_num, int pid) {
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(vcpu_num, &cpuset);

	if (sched_setaffinity(pid, sizeof(cpu_set_t), &cpuset) < 0) {
		//fprintf(stderr, "Set thread to VCPU error!\n");
	}
}


void set_affinity(uint64_t vcpu_num) {
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(vcpu_num, &cpuset);

	if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) < 0) {
		fprintf(stderr, "Set thread to VCPU error!\n");
	}
}

void set_affinity_out(uint64_t vcpu_num, pthread_t tid) {
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(vcpu_num, &cpuset);

	if (pthread_setaffinity_np(tid, sizeof(cpu_set_t), &cpuset) < 0) {
		fprintf(stderr, "Set thread to VCPU error!\n");
	}
}

void set_priority(void) {
	struct sched_param param;
	int priority = sched_get_priority_max(SCHED_RR);
	param.sched_priority = priority;
	int s = pthread_setschedparam(pthread_self(), SCHED_RR, &param);
	if (s != 0) handle_error("Pthread_setschedparam error!\n");
}

void set_idle_priority(void) {
	struct sched_param param;
	param.sched_priority = 0;
	int s = pthread_setschedparam(pthread_self(), SCHED_IDLE, &param);
	if (s != 0) handle_error("Pthread_setschedparam error!\n");
}

void set_nice_priority(int priority, int pid) {
	int which = PRIO_PROCESS;

	int ret = setpriority(which, pid, priority);
}

uint64_t is_file_exist(char *file_path) {
	if (TRUE == g_file_test(file_path, G_FILE_TEST_EXISTS))
		return 1;
	else
		return 0;
}

void init_sem(void) {
	if (sem_init(&sem_main, 0, 0) == -1)
		handle_error("sem1_init_error");
	if (sem_init(&sem_worker, 0, 0) == -1)
		handle_error("sem1_init_error");
}

