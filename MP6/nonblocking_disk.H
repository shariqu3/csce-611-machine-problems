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
    Thread* waiting_thread;

protected:
    virtual void wait_while_busy();

public:
   NonBlockingDisk(unsigned int _size); 
   /* Creates a NonBlockingDisk device with the given size connected to the 
      MASTER slot of the primary ATA controller.
      NOTE: We are passing the _size argument out of laziness. 
      In a real system, we would infer this information from the 
      disk controller. */
   bool virtual possible_disk_operation();
   void wait_while_busy();
   /* Returns true if a disk operation is possible at the moment, and false  
   otherwise. */


   virtual void handle_interrupt(REGS * _r);
};

#endif
