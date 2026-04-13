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
    start = 0;
    n_threads = 0;
    is_disk_in_use = false;
    current_disk_thread = nullptr;
    for (int i = 0; i < 8; i++) {
        disk_queue[i] = nullptr;
    }
    InterruptHandler::register_handler(14, this);
}

bool NonBlockingDisk::possible_disk_operation() {
    return !is_disk_in_use && (n_threads < 8);
}

void NonBlockingDisk::wait_while_busy() {
    while (is_busy()) {
        current_disk_thread = Thread::CurrentThread();
        System::SCHEDULER->yield();
    }
}

void NonBlockingDisk::read(unsigned long _block_no, unsigned char* _buf) {
    bool is_ints_enabled = Machine::interrupts_enabled();
    if (is_ints_enabled) Machine::disable_interrupts();
    
    while (is_disk_in_use) {
        if (n_threads < 8) {
            disk_queue[(start + n_threads++) % 8] = Thread::CurrentThread();
        } else {
            Console::puts("Disk queue full!\n");
            assert(false);
        }
        
        if (is_ints_enabled) Machine::enable_interrupts();
        System::SCHEDULER->yield(); // Wait to be dequeued
        if (is_ints_enabled) Machine::disable_interrupts();
    }
    
    is_disk_in_use = true;
    if (is_ints_enabled) Machine::enable_interrupts();

    // Actual operation
    SimpleDisk::read(_block_no, _buf);

    if (is_ints_enabled) Machine::disable_interrupts();
    is_disk_in_use = false;
    
    if (n_threads > 0) {
        Thread* next = disk_queue[start];
        disk_queue[start] = nullptr;
        start = (start + 1) % 8;
        n_threads--;
        System::SCHEDULER->resume(next);
    }
    if (is_ints_enabled) Machine::enable_interrupts();
}

void NonBlockingDisk::write(unsigned long _block_no, unsigned char* _buf) {
    bool is_ints_enabled = Machine::interrupts_enabled();
    if (is_ints_enabled) Machine::disable_interrupts();
    
    while (is_disk_in_use) {
        if (n_threads < 8) {
            disk_queue[(start + n_threads++) % 8] = Thread::CurrentThread();
        } else {
            Console::puts("Disk queue full!\n");
            assert(false);
        }
        
        if (is_ints_enabled) Machine::enable_interrupts();
        System::SCHEDULER->yield(); // Wait to be dequeued
        if (is_ints_enabled) Machine::disable_interrupts();
    }
    
    is_disk_in_use = true;
    if (is_ints_enabled) Machine::enable_interrupts();

    // Actual operation
    SimpleDisk::write(_block_no, _buf);

    if (is_ints_enabled) Machine::disable_interrupts();
    is_disk_in_use = false;
    
    if (n_threads > 0) {
        Thread* next = disk_queue[start];
        disk_queue[start] = nullptr;
        start = (start + 1) % 8;
        n_threads--;
        System::SCHEDULER->resume(next);
    }
    if (is_ints_enabled) Machine::enable_interrupts();
}

void NonBlockingDisk::handle_interrupt(REGS * _r) {
    if (current_disk_thread != nullptr) {
        Thread* t = current_disk_thread;
        current_disk_thread = nullptr;
        System::SCHEDULER->resume(t);
    }
}
