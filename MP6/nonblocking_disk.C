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

/*--------------------------------------------------------------------------*/
/* HARDWARE POLLING (OVERRIDDEN) */
/*--------------------------------------------------------------------------*/
/* 
    Instead of actively spinning inside a busy loop and wasting CPU cycles 
    when the disk hardware is fetching data, we capture the actively bound 
    disk thread and proactively yield execution back to the Scheduler. 
    Crucially, since we use disable_interrupts(), we guarantee an interrupt 
    cannot fire precisely before the thread is safely yielded!
*/
void NonBlockingDisk::wait_while_busy() {
    bool is_ints_enabled = Machine::interrupts_enabled();
    Machine::disable_interrupts();
    while (is_busy()) {
        current_disk_thread = Thread::CurrentThread();
        if (is_ints_enabled) Machine::enable_interrupts();
        System::SCHEDULER->yield();
        Machine::disable_interrupts();
    }
    if (is_ints_enabled) Machine::enable_interrupts();
}

/*--------------------------------------------------------------------------*/
/* READ OPERATIONS */
/*--------------------------------------------------------------------------*/
/*
    Provides thread-safe access to the SimpleDisk read routines. 
    It evaluates the is_disk_in_use mutex natively within an interrupt 
    disabled region. If occupied, the calling thread is added strictly to the 
    internal disk_queue and subsequently yields the CPU. If free, the mutex 
    is claimed, the hardware is queried, and upon returning, the mutex releases 
    with the next queued thread cleanly handed back over to the Scheduler for wakeup.
*/
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

/*--------------------------------------------------------------------------*/
/* WRITE OPERATIONS */
/*--------------------------------------------------------------------------*/
/*
    Symmetrical to read operations, handles block writing requests safely 
    by enforcing the is_disk_in_use boolean lock. Concurrent threads will be 
    added precisely onto the bounded wait array. Once the current operation 
    safely terminates and the hardware controller resets, it resumes the next 
    sleeping thread preserving strict execution serialization without 
    corrupting the underlying hardware device registers.
*/
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

/*--------------------------------------------------------------------------*/
/* ASYNCHRONOUS INTERRUPT HANDLER */
/*--------------------------------------------------------------------------*/
/*
    Registered natively on IRQ 14, this routine is driven exclusively by the 
    physical primary IDE controller upon successfully completing data fetches 
    or write commits. Rather than constantly checking status flags over the loop, 
    the native signal safely revives the previously sleeping thread allowing 
    subsequent completion evaluation to immediately re-lock logic sequentially.
*/
void NonBlockingDisk::handle_interrupt(REGS * _r) {
    if (current_disk_thread != nullptr) {
        Thread* t = current_disk_thread;
        current_disk_thread = nullptr;
        System::SCHEDULER->resume(t);
    }
}
