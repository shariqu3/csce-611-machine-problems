/*
     File        : nonblocking_disk.H

     Author      : 
     Date        : 
     Description : 
*/

#ifndef _NONBLOCKING_DISK_H_
#define _NONBLOCKING_DISK_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "simple_disk.H"
#include "interrupts.H"

class Thread;

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* N o n B l o c k i n g D i s k  */
/*--------------------------------------------------------------------------*/

class NonBlockingDisk : public SimpleDisk, public InterruptHandler {
private:
    Thread* disk_queue[8];
    int start;
    int n_threads;
    bool is_disk_in_use;
    Thread* current_disk_thread;

protected:
    virtual void wait_while_busy();

public:
   NonBlockingDisk(unsigned int _size); 
   /* Creates a NonBlockingDisk device with the given size connected to the 
      MASTER slot of the primary ATA controller.
      NOTE: We are passing the _size argument out of laziness. 
      In a real system, we would infer this information from the 
      disk controller. */

   virtual bool possible_disk_operation();
   /* Returns true if a disk operation is possible at the moment, and false  
   otherwise. */

   virtual void read(unsigned long _block_no, unsigned char* _buf);
   virtual void write(unsigned long _block_no, unsigned char* _buf);

   virtual void handle_interrupt(REGS * _r);
};

#endif
