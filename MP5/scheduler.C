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
  start = 0;
  running_thread = nullptr;

  Console::puts("Constructed Scheduler.\n");
}

void Scheduler::print_details() {
  Console::puts("Details:.\n");
  Console::puts("n_threads: ");
  Console::puti(n_threads);
  Console::puts("\n");
  Console::puts("start: ");
  Console::puti(start);
  Console::puts("\n");
}

void Scheduler::yield() {
  running_thread = q[start];
  start = (start + 1) % MAX_THREADS;
  n_threads--;
  Thread::dispatch_to(running_thread);
}

void Scheduler::resume(Thread *_thread) {
  while (z_threads) {
    Thread *del_thread = zombie_q[--z_threads];
    del_thread->cleanup();
    delete del_thread;
  }
  if (n_threads == MAX_THREADS) {
    Console::puts("Max threads already in queue.\n");
    assert(false);
  }
  q[(start + n_threads++) % MAX_THREADS] = _thread;
}

void Scheduler::add(Thread *_thread) { resume(_thread); }

void Scheduler::terminate(Thread *_thread) { assert(false); }
