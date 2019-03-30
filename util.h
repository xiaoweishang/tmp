#ifndef __DEBUG_HEADER_FILE__
#define __DEBUG_HEADER_FILE__

#include <sys/time.h>
#include <stdint.h>

#define MY_MIN(a, b) ({                         \
		__typeof__(a) __a = (a);                \
		__typeof__(b) __b = (b);                \
		(__a < __b) ? __a : __b;                \
		})
#define MY_MAX(a, b) ({                         \
		__typeof__(a) __a = (a);                \
		__typeof__(b) __b = (b);                \
		(__a > __b) ? __a : __b;                \
		})

#define MY_SWAP(a, b) ({						\
		__typeof__(a) __a = (a);				\
		(a) = (b);                              \
		(b) = __a;                              \
		})
#define handle_error_en(en, msg) \
	do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)
#define handle_error(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while (0)

//debug time
//fine-grained cycles
static unsigned long __inline__ rdtscp(void) {
  unsigned int tickl, tickh;
  __asm__ __volatile__("rdtscp":"=a"(tickl),"=d"(tickh)::"%ecx");
  return ((uint64_t)tickh << 32)|tickl;
}

static __inline uint64_t rdtsc(void) {
	u_int32_t low, high;

	__asm __volatile("rdtsc" : "=a" (low), "=d" (high));
	return (low | ((u_int64_t)high << 32));
}

uint64_t debug_time_usec(void);
double debug_time_sec(void);
uint64_t debug_diff_usec(const uint64_t last);
double debug_diff_sec(const double last);
uint64_t debug_tv_diff(const struct timeval * const t0, const struct timeval * const t1);
void debug_print_tv_diff(char *tag, const struct timeval t0, const struct timeval t1);
uint64_t debug_time_monotonic_usec(void);
uint64_t debug_time_monotonic_nsec(void);
void my_sleep(uint64_t microseconds);
//debug time end

int get_vcpu_count(void);
uint64_t get_pid_affinity(int pid);
uint64_t get_affinity(void);
uint64_t get_affinity_out(pthread_t tid);
void set_pid_affinity(uint64_t vcpu_num, int pid);
void set_affinity(uint64_t vcpu_num);
void set_affinity_out(uint64_t vcpu_num, pthread_t tid);
void set_priority(void);
void set_idle_priority(void);
void set_nice_priority(int priority, int pid);
uint64_t is_file_exist(char *file_path);
void init_sem(void);

void sgenrand(unsigned long);
long genrand(void);
long random_at_most(long);

#endif
