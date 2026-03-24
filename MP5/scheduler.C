/*
 File: scheduler.C

 Author:
 Date  :

 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "console.H"
#include "scheduler.H"
#include "thread.H"
#include "utils.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

void do_nothing() {
  for (;;)
    ;
}

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   S c h e d u l e r  */
/*--------------------------------------------------------------------------*/

Scheduler::Scheduler() {
  n_threads = 0;
  sentinel_thread = new Thread(do_nothing, nullptr, 0);
  running_thread = sentinel_thread;
  start = 0;
  Console::puts("Constructed Scheduler.\n");
}

void Scheduler::yield() {
  if (running_thread != sentinel_thread) {
    add(running_thread);
    n_threads++;
  }
  running_thread = q[start];
  start = (start + 1) % MAX_THREADS;
}

void Scheduler::resume(Thread *_thread) {
  if (n_threads == MAX_THREADS) {
    Console::puts("Max threads already in queue.\n");
    assert(false);
  }
  q[(start + n_threads) % MAX_THREADS] = _thread;
  n_threads++;
}

void Scheduler::add(Thread *_thread) { resume(_thread); }

void Scheduler::terminate(Thread *_thread) { assert(false); }
