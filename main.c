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

int main(int argc, char **argv)
{
  if (argc > 1 && strcmp(argv[1], "priority") == 0)
  {
    gtinit(PRIORITY); // initialize threads, see gthr.c
  }
  else
  {
    gtinit(LOTTERY); // initialize threads, see gthr.c
  }
  gtgo(f, 10, 10); // set f() as first thread
  gtgo(f, 10, 10); // set f() as first thread
  gtgo(f, 9, 9);   // set f() as second thread
  gtgo(g, 6, 6);   // set g() as third thread
  gtgo(g, 2, 2);   // set g() as third thread
  gtgo(g, 2, 2);   // set g() as fourth thread
  gtgo(g, 0, 1);   // set g() as fourth thread
  gtret(1);        // wait until all threads terminate
}
