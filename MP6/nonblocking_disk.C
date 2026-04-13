/*
     File        : nonblocking_disk.c

     Author      : 
     Modified    : 

     Description : 

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "utils.H"
#include "console.H"
#include "nonblocking_disk.H"

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

NonBlockingDisk::NonBlockingDisk(unsigned int _size) 
  : SimpleDisk(_size) {
  n_threads = 0;
  start = 0;
  for (int i = 0; i < MAX_THREADS; i++) {
    disk_q[i] = nullptr;
  }
}

bool NonBlockingDisk::possible_disk_operation() {
  if(SimpleDisk::is_busy() && n_threads) {
    return false;
  }
  return true;
}

void NonBlockingDisk::wait_while_busy(){
  //1. Add current thread to disk_q
  if(n_threads==MAX_THREADS)
  {
    Console::puts("DISK QUEUE max limit reached.");
    assert(false);
  }
  disk_q[(start+n_threads++)%MAX_THREADS] = Thread::CurrentThread();
  //2. Then yield  without adding current thread to scheduler queue
  Thread::SYSTEM_SCHEDULER->yield();
}
