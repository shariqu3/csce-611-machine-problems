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
#include "simple_timer.H"
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

void Scheduler::sentinel_entry() {
  for (;;) {
    if (Thread::SYSTEM_SCHEDULER != nullptr) {
      Thread::SYSTEM_SCHEDULER->yield();
    }
  }
}

Scheduler::Scheduler() {
  n_threads = 0;
  start = 0;
  running_thread = nullptr;
  sentinel_thread = nullptr;
  sentinel_stack = nullptr;
  z_threads = 0;

  for (int i = 0; i < MAX_THREADS; i++) {
    q[i] = nullptr;
    zombie_q[i] = nullptr;
  }

  sentinel_stack = new char[SENTINEL_STACK_SIZE];
  sentinel_thread = new Thread(&Scheduler::sentinel_entry, sentinel_stack,
                               SENTINEL_STACK_SIZE);

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
  bool is_interrupts_enabled = Machine::interrupts_enabled();
  if (is_interrupts_enabled) {
    Machine::disable_interrupts();
  }

  if (n_threads == 0) {
    Thread *current_thread = Thread::CurrentThread();

    if (current_thread == sentinel_thread) {
      if (is_interrupts_enabled) {
        Machine::enable_interrupts();
      }
      return;
    }

    running_thread = sentinel_thread;

    if (is_interrupts_enabled) {
      Machine::enable_interrupts();
    }

    Thread::dispatch_to(sentinel_thread);
    return;
  }

  running_thread = q[start];
  q[start] = nullptr;
  start = (start + 1) % MAX_THREADS;
  n_threads--;

  if (is_interrupts_enabled) {
    Machine::enable_interrupts();
  }

  Thread::dispatch_to(running_thread);
}

void Scheduler::resume(Thread *_thread) {
  bool is_interrupts_enabled = Machine::interrupts_enabled();
  if (is_interrupts_enabled) {
    Machine::disable_interrupts();
  }

  while (z_threads) {
    Thread *del_thread = zombie_q[--z_threads];
    zombie_q[z_threads] = nullptr;
    del_thread->cleanup();
    delete del_thread;
    del_thread = nullptr;
  }
  if (_thread == nullptr) {
    if (is_interrupts_enabled) {
      Machine::enable_interrupts();
    }
    return;
  }

  if (_thread == sentinel_thread) {
    if (is_interrupts_enabled) {
      Machine::enable_interrupts();
    }
    return;
  }

  if (n_threads == MAX_THREADS) {
    if (is_interrupts_enabled) {
      Machine::enable_interrupts();
    }
    Console::puts("Max threads already in queue.\n");
    assert(false);
  }
  q[(start + n_threads++) % MAX_THREADS] = _thread;

  if (is_interrupts_enabled) {
    Machine::enable_interrupts();
  }
}

void Scheduler::add(Thread *_thread) { resume(_thread); }

void Scheduler::terminate(Thread *_thread) {
  bool is_interrupts_enabled = Machine::interrupts_enabled();
  if (is_interrupts_enabled) {
    Machine::disable_interrupts();
  }

  if (_thread == nullptr) {
    if (is_interrupts_enabled) {
      Machine::enable_interrupts();
    }
    return;
  }

  if (_thread == sentinel_thread) {
    if (is_interrupts_enabled) {
      Machine::enable_interrupts();
    }
    return;
  }

  if (z_threads == MAX_THREADS) {
    if (is_interrupts_enabled) {
      Machine::enable_interrupts();
    }
    Console::puts("Too many zombie threads.\n");
    assert(false);
  }

  Thread *current_thread = Thread::CurrentThread();

  if (_thread == current_thread) {
    zombie_q[z_threads++] = _thread;

    if (is_interrupts_enabled) {
      Machine::enable_interrupts();
    }

    yield();
    assert(false);
  }

  bool found = false;
  for (int i = 0; i < n_threads; i++) {
    int idx = (start + i) % MAX_THREADS;
    if (q[idx] == _thread) {
      found = true;
      for (int j = i; j < n_threads - 1; j++) {
        int from = (start + j + 1) % MAX_THREADS;
        int to = (start + j) % MAX_THREADS;
        q[to] = q[from];
      }
      int tail = (start + n_threads - 1) % MAX_THREADS;
      q[tail] = nullptr;
      n_threads--;
      break;
    }
  }

  if (!found && _thread != running_thread) {
    if (is_interrupts_enabled) {
      Machine::enable_interrupts();
    }
    return;
  }

  zombie_q[z_threads++] = _thread;

  if (is_interrupts_enabled) {
    Machine::enable_interrupts();
  }
}

void Scheduler::request_preemption() {
  Thread *current_thread = Thread::CurrentThread();
  if (current_thread == nullptr) {
    return;
  }

  if (n_threads == 0) {
    return;
  }

  resume(current_thread);
  yield();
}