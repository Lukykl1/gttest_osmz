// Based on https://c9x.me/articles/gthreads/code0.html
#include "gthr.h"
#include <signal.h>

// Dummy function to simulate some thread work
void f(void)
{
  static int x;
  int i, id;

  id = ++x;
  while (true)
  {

    printf("F Thread id = %d, val = %d BEGINNING\n", id, ++i);
    uninterruptibleNanoSleep(0, 50000000);
    printf("F Thread id = %d, val = %d END\n", id, ++i);
    uninterruptibleNanoSleep(0, 50000000);
  }
}

// Dummy function to simulate some thread work
void g(void)
{
  static int x;
  int i, id;

  id = ++x;
  while (true)
  {

    printf("G Thread id = %d, val = %d BEGINNING\n", id, ++i);
    uninterruptibleNanoSleep(0, 50000000);
    printf("G Thread id = %d, val = %d END\n", id, ++i);
    uninterruptibleNanoSleep(0, 50000000);
  }
}

void showStatsHandler(int signum)
{
  static int end = 0;
  static long last_called = 0;
  printf("\n\n%-3s%-10s%-20s%-20s%-20s%-10s%-10s%-10s%-10s \n",
         "ID", "STATE", "WAIT COUNT", "TOTAL RUNTIME", "TOTAL WAIT TIME", "MAX WAIT", "MIN WAIT", "MAX RUN", "MIN RUN");
  for (int i = 0; i < MaxGThreads; i++)
  {
    if (gttbl[i].st == Ready || gttbl[i].st == Running)
    {
      printf("%-3d%-10s%-20ld%-20ld%-20ld%-10ld%-10ld%-10ld%-10ld\n", i, gttbl[i].st == Running ? "running" : "ready",
             gttbl[i].stats.count_of_waits,
             gttbl[i].stats.total_run_time,
             gttbl[i].stats.total_waiting_time,
             gttbl[i].stats.max_wait_time,
             gttbl[i].stats.min_wait_time,
             gttbl[i].stats.max_run_time,
             gttbl[i].stats.min_run_time);
    }
  }
  printf("\n\n%-3s%-10s%-20s%-20s \n",
         "ID", "STATE", "RUNTIME %%", "WAIT TIME %%");
  for (int i = 0; i < MaxGThreads; i++)
  {
    if (gttbl[i].st == Ready || gttbl[i].st == Running)
    {
      long duration = get_time() - gt_started;
      printf("%-3d%-10s%-20f%-20f \n", i, gttbl[i].st == Running ? "running" : "ready",
             (double)gttbl[i].stats.total_run_time / (double)duration * 100.0,
             (double)gttbl[i].stats.total_waiting_time / (double)duration * 100.0);
    }
  }
  if(last_called + 5 * 1000000L < get_time()){
    end = 0;
    printf("last %ld, reset %ld, now %ld, end %d \n", last_called, last_called + 5 * 1000000L,  get_time(), end);
  }
  if (++end > 3)
  {
    exit(0);
  }
  last_called = get_time();
}
int main(void)
{
  signal(SIGINT, showStatsHandler);
  gtinit(); // initialize threads, see gthr.c
  gtgo(f);  // set f() as first thread
  gtgo(f);  // set f() as second thread
  gtgo(g);  // set g() as third thread
  gtgo(g);  // set g() as fourth thread
  gtret(1); // wait until all threads terminate
}
