// Based on https://c9x.me/articles/gthreads/code0.html
#include "gthr.h"
#include <signal.h>

bool text_enabled = false;
// Dummy function to simulate some thread work
void f(void)
{
  static int x;
  int i, id;

  id = ++x;
  while (true)
  {
    if (text_enabled)
    {
      printf("F Thread id = %d, val = %d BEGINNING\n", id, ++i);
    }
    uninterruptibleNanoSleep(0, 50000000);
    if (text_enabled)
    {
      printf("F Thread id = %d, val = %d END\n", id, ++i);
    }
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
    if (text_enabled)
    {
      printf("G Thread id = %d, val = %d BEGINNING\n", id, ++i);
    }
    uninterruptibleNanoSleep(0, 50000000);
    if (text_enabled)
    {
      printf("G Thread id = %d, val = %d END\n", id, ++i);
    }
    uninterruptibleNanoSleep(0, 50000000);
  }
}
void test(void)
{
  static int x;
  int i, id;
  id = ++x;
  bool flip = true;
  while (true)
  {
    printf("TEST Thread id = %d, val = %d \n", id, ++i);
    my_sleep(15000);
    flip = !flip;
    showStatsHandler(0);
    if (scheduler == LOTTERY)
    {
      set_tickets(7, flip ? 1 : 10);
    }
    else if (scheduler == PRIORITY)
    {
      set_priority(7, flip ? 1 : 10);
    }
    reset_all_stats();
  }
}
int main(int argc, char **argv)
{
  if (argc > 1 && (strcmp(argv[1], "priority") == 0 || strcmp(argv[1], "p") == 0))
  {
    gtinit(PRIORITY); // initialize threads, see gthr.c
  }
  else if (argc > 1 && (strcmp(argv[1], "rr") == 0 || strcmp(argv[1], "roundrobin") == 0))
  {
    gtinit(RR); // initialize threads, see gthr.c
  }
  else
  {
    gtinit(LOTTERY); // initialize threads, see gthr.c
  }
  if (argc > 2 && strcmp(argv[2], "text") == 0)
  {
    text_enabled = true;
  }
  gtgo(test, 10, 10); // set f() as first thread
  gtgo(f, 10, 10);    // set f() as first thread
  gtgo(f, 10, 10);    // set f() as first thread
  gtgo(f, 9, 9);      // set f() as second thread
  gtgo(g, 6, 6);      // set g() as third thread
  gtgo(g, 2, 2);      // set g() as third thread
  gtgo(g, 2, 2);      // set g() as fourth thread
  gtgo(g, 0, 1);      // set g() as fourth thread
  gtret(1);           // wait until all threads terminate
}
