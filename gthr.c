#include "gthr.h"

#define DEBUG_SIGNAL_SCALE 1

long get_time_us(void)
{
  struct timespec ts;
  timespec_get(&ts, TIME_UTC);
  return (long)ts.tv_sec * 1000000L + ts.tv_nsec / 1000; //us
}
int get_priority(struct priority_t pr)
{
  return pr.additional_priority + pr.priority;
}
// function triggered periodically by timer (SIGALRM)
void gthandle(int sig)
{
  gtyield();
}

// initialize first thread as current context
void gtinit(scheduler_t type)
{
  scheduler = type;
  gtcur = &gttbl[0];   // initialize current thread with thread #0
  gtcur->st = Running; // set current to running
  gtcur->stats.last_start = get_time_us();
  gtcur->pr.tickets = 1;
  total_active_tickets = 1;
  gt_started = get_time_us();
  srand(time(0));
  signal(SIGINT, showStatsHandler);
  signal(SIGALRM, gthandle); // register SIGALRM, signal from timer generated by alarm
}

// exit thread
void __attribute__((noreturn)) gtret(int ret)
{
  if (gtcur != &gttbl[0])
  {                     // if not an initial thread,
    gtcur->st = Unused; // set current thread as unused
    total_active_tickets -= gtcur->pr.tickets;
    gtyield();            // yield and make possible to switch to another thread
    assert(!"reachable"); // this code should never be reachable ... (if yes, returning function on stack was corrupted)
  }
  while (gtyield())
    ; // if initial thread, wait for other to terminate
  exit(ret);
}
static void set_wait_stats()
{
  long wait_time = get_time_us() - gtcur->stats.last_sleep;
  gtcur->stats.count_of_waits++;
  if (wait_time > gtcur->stats.wait.max)
  {
    gtcur->stats.wait.max = wait_time;
  }
  if (wait_time < gtcur->stats.wait.min || gtcur->stats.wait.min == 0)
  {
    gtcur->stats.wait.min = wait_time;
  }
  gtcur->stats.last_start = get_time_us();
  gtcur->stats.wait.total += wait_time;
}

static void set_run_stats()
{
  if (gtcur->stats.last_start != 0) //never run before
  {
    long run_time = get_time_us() - gtcur->stats.last_start;
    if (run_time > gtcur->stats.run.max)
    {
      gtcur->stats.run.max = run_time;
    }
    if (run_time < gtcur->stats.run.min || gtcur->stats.run.min == 0)
    {
      gtcur->stats.run.min = run_time;
    }
    gtcur->stats.last_sleep = get_time_us();
    gtcur->stats.run.total += run_time;
  }
}

int find_max_priority()
{
  int max_priority = -1;
  for (int i = 0; i < MaxGThreads; i++)
  {
    if (gttbl[i].st != Unused && get_priority(gttbl[i].pr) > max_priority)
    {
      max_priority = get_priority(gttbl[i].pr);
    }
  }
  return max_priority;
}

void increase_other_threads(struct gt *p)
{
  struct gt *start = p;
  do
  { // iterate through gttbl[] until we find new thread in state Ready
    if (++p == &gttbl[MaxGThreads])
    { // at the end rotate to the beginning
      p = &gttbl[0];
    }
    if (p->st != Running)
    {
      p->pr.additional_priority++;
    }
  } while (p != start);
}
// switch from one thread to other
bool gtyield(void)
{
  struct gt *p;
  struct gtctx *old, *new;

  resetsig(SIGALRM); // reset signal

  p = gtcur;

  if (scheduler == LOTTERY)
  {
    int ticket = rand() % total_active_tickets + 1;
    int ticket_seen = 0;
    for (int i = 0; i < MaxGThreads; i++)
    {
      //printf("i %d gttbl[i].pr.tickets %d ticket_seen %d; sum %d > %d\n", i, gttbl[i].pr.tickets, ticket_seen, gttbl[i].pr.tickets + ticket_seen, ticket);
      if (gttbl[i].pr.tickets + ticket_seen > ticket)
      {
        //printf("i %d gttbl[i].pr.tickets %d ticket_seen %d; sum %d > %d\n", i, gttbl[i].pr.tickets, ticket_seen, gttbl[i].pr.tickets + ticket_seen, ticket);
        p = &gttbl[i];
        break;
      }
      ticket_seen += gttbl[i].pr.tickets;
    }
  }
  else if (scheduler == RR)
  {
    while (p->st != Ready)
    { // iterate through gttbl[] until we find new thread in state Ready
      if (++p == &gttbl[MaxGThreads])
      { // at the end rotate to the beginning
        p = &gttbl[0];
      }
      if (p == gtcur) // did not find any other Ready threads
      {
        return false;
      }
    }
  }
  else
  {
    int max_priority = find_max_priority();
    while (p->st != Ready || get_priority(p->pr) < max_priority)
    { // iterate through gttbl[] until we find new thread in state Ready
      if (++p == &gttbl[MaxGThreads])
      { // at the end rotate to the beginning
        p = &gttbl[0];
        increase_other_threads(p);
      }
      if (p == gtcur) // did not find any other Ready threads
      {
        return false;
      }
    }
  }

  //printf("SWITCHING from %p to %p, priority %d to %d\n", &gtcur, &p, get_priority(gtcur->pr), get_priority(p->pr));
  set_run_stats();

  if (gtcur->st != Unused)
  { // switch current to Ready and new thread found in previous loop to Running
    gtcur->st = Ready;
  }
  p->st = Running;
  p->pr.additional_priority = 0;
  old = &gtcur->ctx; // prepare pointers to context of current (will become old)
  new = &p->ctx;     // and new to new thread found in previous loop
  gtcur = p;         // switch current indicator to new thread
  gtswtch(old, new); // perform context switch (assembly in gtswtch.S)

  set_wait_stats();

  return true;
}

// return function for terminating thread
void gtstop(void)
{
  gtret(0);
}

void showStatsHandler(int signum)
{
  static int end = 0;
  static long last_called = 0;
  printf("\n\nSCHEDULING TYPE: %s\n", scheduler == LOTTERY ? "lottery" : (scheduler == RR ? "round robin" : "priority"));
  printf("%-3s| %-9s| %-18s| %-18s| %-18s| %-9s| %-9s| %-9s| %-9s| \n",
         "ID", "STATE", "WAIT COUNT", "TOTAL RUNTIME", "TOTAL WAIT TIME", "MAX WAIT", "MIN WAIT", "MAX RUN", "MIN RUN");

  for (int i = 0; i < MaxGThreads; i++)
  {
    if (gttbl[i].st == Ready || gttbl[i].st == Running)
    {
      printf("%-3d| %-9s| %-18ld| %-18ld| %-18ld| %-9ld| %-9ld| %-9ld| %-9ld|\n", i, gttbl[i].st == Running ? "running" : "ready",
             gttbl[i].stats.count_of_waits,
             gttbl[i].stats.run.total,
             gttbl[i].stats.wait.total,
             gttbl[i].stats.wait.max,
             gttbl[i].stats.wait.min,
             gttbl[i].stats.run.max,
             gttbl[i].stats.run.min);
    }
  }
  printf("\n\n%-3s| %-9s| %-20s| %-20s|",
         "ID", "STATE", "RUNTIME %%", "WAIT TIME %%");
  if (scheduler == LOTTERY)
  {
    printf(" %-10s|", "TICKETS");
  }
  if (scheduler == PRIORITY)
  {
    printf(" %-10s| %-10s|", "PRIORITY", "ADD PRIOR");
  }
  printf("\n");
  for (int i = 0; i < MaxGThreads; i++)
  {
    if (gttbl[i].st == Ready || gttbl[i].st == Running)
    {
      long duration = get_time_us() - gt_started;
      printf("%-3d| %-9s| %-20f| %-20f|", i, gttbl[i].st == Running ? "running" : "ready",
             (double)gttbl[i].stats.run.total / (double)duration * 100.0,
             (double)gttbl[i].stats.wait.total / (double)duration * 100.0);
      if (scheduler == LOTTERY)
      {
        printf(" %-10d|", gttbl[i].pr.tickets);
      }
      if (scheduler == PRIORITY)
      {
        printf(" %-10d| %-10d|", gttbl[i].pr.priority, gttbl[i].pr.additional_priority);
      }
      printf("\n");
    }
  }
  if (scheduler == LOTTERY)
  {
    printf("total tickets %d \n", total_active_tickets);
  }
  if (last_called + 5 * 1000000L < get_time_us())
  {
    end = 0;
    printf("End reseted. To end press ctrl + c three times in quick succesion\n");
  }
  else
  {
    printf("To end remains Ctrl + C: %d \n", 3 - (++end));
  }
  if (end > 2)
  {
    exit(0);
  }
  last_called = get_time_us();
}
void my_sleep(int ms)
{
  long started = get_time_us();
  long duration = ms * 1000;
  while (started + duration > get_time_us())
  {
    gtyield();
  }
}
void reset_stats(int thr_id)
{
  memset(&gttbl[thr_id].stats, 0, sizeof(struct stats_t));
  gttbl[thr_id].stats.last_sleep = get_time_us();
}
void reset_all_stats()
{
  for (int i = 0; i < MaxGThreads; i++)
  {
    memset(&gttbl[i].stats, 0, sizeof(struct stats_t));
    if (gttbl[i].st == Running)
    {
      gttbl[i].stats.last_start = get_time_us();
    }
    else
    {
      gttbl[i].stats.last_sleep = get_time_us();
    }
  }
  gt_started = get_time_us();
}
void set_priority(int thr_id, int prio)
{
  gttbl[thr_id].pr.priority = prio;
  gttbl[thr_id].pr.additional_priority = 0;
}
void set_tickets(int thr_id, int tickets)
{
  total_active_tickets -= gttbl[thr_id].pr.tickets;
  gttbl[thr_id].pr.tickets = tickets;
  total_active_tickets += tickets;
}
// create new thread by providing pointer to function that will act like "run" method
int gtgo(void (*f)(void), int priority, int tickets)
{
  char *stack;
  struct gt *p;

  for (p = &gttbl[0];; p++)       // find an empty slot
    if (p == &gttbl[MaxGThreads]) // if we have reached the end, gttbl is full and we cannot create a new thread
      return -1;
    else if (p->st == Unused)
      break; // new slot was found

  stack = malloc(StackSize); // allocate memory for stack of newly created thread
  if (!stack)
    return -1;

  *(uint64_t *)&stack[StackSize - 8] = (uint64_t)gtstop; //	put into the stack returning function gtstop in case function calls return
  *(uint64_t *)&stack[StackSize - 16] = (uint64_t)f;     //  put provided function as a main "run" function
  p->ctx.rsp = (uint64_t)&stack[StackSize - 16];         //  set stack pointer
  p->st = Ready;                                         //  set state
  p->stats.last_sleep = get_time_us();
  p->pr.priority = priority;
  p->pr.additional_priority = 0;
  p->pr.tickets = tickets;
  total_active_tickets += tickets;
  return 0;
}

void resetsig(int sig)
{
  if (sig == SIGALRM)
  {
    alarm(0); // Clear pending alarms if any
  }

  sigset_t set;         // Create signal set
  sigemptyset(&set);    // Clear it
  sigaddset(&set, sig); // Set signal (we use SIGALRM)

  sigprocmask(SIG_UNBLOCK, &set, NULL); // Fetch and change the signal mask

  if (sig == SIGALRM)
  {
    // Generate alarms
    ualarm(500 * DEBUG_SIGNAL_SCALE, 500 * DEBUG_SIGNAL_SCALE); // Schedule signal after given number of microseconds
  }
}

int uninterruptibleNanoSleep(time_t sec, long nanosec)
{
  struct timespec req;
  req.tv_sec = sec;
  req.tv_nsec = nanosec;

  do
  {
    if (0 != nanosleep(&req, &req))
    {
      if (errno != EINTR)
        return -1;
    }
    else
    {
      break;
    }
  } while (req.tv_sec > 0 || req.tv_nsec > 0);
  return 0; /* Return success */
}
