#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <memory.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

enum {
    MaxGThreads = 5,		// Maximum number of threads, used as array size for gttbl
    StackSize = 0x400000,	// Size of stack of each thread
};

struct gt {
  // Saved context, switched by gtswtch.S (see for detail)
  struct gtctx {
    uint64_t rsp;
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t rbx;
    uint64_t rbp;
  }
  ctx;
  // Thread state
  enum {
    Unused,
    Running,
    Ready,
  }
  st;
  struct stats_t{
    long total_waiting_time;
    long max_wait_time;
    long min_wait_time;
    long count_of_waits;
    long total_run_time;
    long max_run_time;
    long min_run_time;
    long last_sleep;
    long last_start;
  } stats;
  //dobu běhu daného vlákna, dobu čekání na procesor, minimální, maximální a průměrnou hodnotu
};

struct gt gttbl[MaxGThreads];	// statically allocated table for thread control
struct gt * gtcur;				// pointer to current thread
long gt_started;

void gtinit(void);				// initialize gttbl
void gtret(int ret);			// terminate thread
void gtswtch(struct gtctx * old, struct gtctx * new);	// declaration from gtswtch.S
bool gtyield(void);				// yield and switch to another thread
void gtstop(void);				// terminate current thread
int gtgo(void( * f)(void));		// create new thread and set f as new "run" function
void resetsig(int sig);			// reset signal
void gthandle(int sig);			// periodically triggered by alarm
int uninterruptibleNanoSleep(time_t sec, long nanosec);	// uninterruptible sleep
long get_time_us(void);