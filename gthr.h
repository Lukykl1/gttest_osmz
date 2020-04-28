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

enum
{
  MaxGThreads = 10,     // Maximum number of threads, used as array size for gttbl
  StackSize = 0x400000, // Size of stack of each thread
};
struct one_stat_t
{
  long total;
  long max;
  long min;
};
struct gt
{
  // Saved context, switched by gtswtch.S (see for detail)
  struct gtctx
  {
    uint64_t rsp;
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t rbx;
    uint64_t rbp;
  } ctx;
  // Thread state
  enum
  {
    Unused,
    Running,
    Ready,
  } st;
  struct priority_t
  {
    int priority;
    int additional_priority;
    int tickets;
  } pr;
  struct stats_t
  {
    struct one_stat_t wait;
    struct one_stat_t run;
    long count_of_waits;
    long last_sleep;
    long last_start;
  } stats;
  //dobu běhu daného vlákna, dobu čekání na procesor, minimální, maximální a průměrnou hodnotu
};
typedef enum
{
  PRIORITY,
  LOTTERY,
  RR
} scheduler_t;

struct gt gttbl[MaxGThreads]; // statically allocated table for thread control
struct gt *gtcur;             // pointer to current thread
long gt_started;
int total_active_tickets;
scheduler_t scheduler;

void my_sleep(int ms);
void reset_stats(int thr_id);
void reset_all_stats();
void set_priority(int thr_id, int prio);
void set_tickets(int thr_id, int tickets);
void gtinit(scheduler_t type);                          // initialize gttbl
void gtret(int ret);                                    // terminate thread
void gtswtch(struct gtctx *old, struct gtctx *new);     // declaration from gtswtch.S
bool gtyield(void);                                     // yield and switch to another thread
void gtstop(void);                                      // terminate current thread
int gtgo(void (*f)(void), int priority, int tickets);   // create new thread and set f as new "run" function
void resetsig(int sig);                                 // reset signal
void gthandle(int sig);                                 // periodically triggered by alarm
int uninterruptibleNanoSleep(time_t sec, long nanosec); // uninterruptible sleep
long get_time_us(void);
void showStatsHandler(int signum);