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
#include "thread.H"
#include "system.H"
#include "scheduler.H"

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

NonBlockingDisk::NonBlockingDisk(unsigned int _size) 
  : SimpleDisk(_size) {
    waiting_thread = nullptr;
    InterruptHandler::register_handler(14, this);
}

void NonBlockingDisk::wait_while_busy() {
    while (is_busy()) {
        waiting_thread = Thread::CurrentThread();
        System::SCHEDULER->yield();
    }
}

void NonBlockingDisk::handle_interrupt(REGS * _r) {
    if (waiting_thread != nullptr) {
        Thread* t = waiting_thread;
        waiting_thread = nullptr;
        System::SCHEDULER->resume(t);
    }
}
