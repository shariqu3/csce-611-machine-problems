/*
 File: vm_pool.C

 Author:
 Date  : 2024/09/20

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
#include "utils.H"
#include "vm_pool.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

#define MAX_REGIONS 256

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   V M P o o l */
/*--------------------------------------------------------------------------*/

VMPool::VMPool(unsigned long _base_address, unsigned long _size,
               ContFramePool *_frame_pool, PageTable *_page_table) {
  base_address = _base_address;
  size = _size;
  frame_pool = _frame_pool;
  page_table = _page_table;

  allocated = (Region *)base_address;
  free_regions = (Region *)(base_address + MAX_REGIONS * sizeof(Region));

  int n_alloc = 0;
  int n_free = 1;

  free_regions[0].base = base_address;
  // Chose to round down here if requested memory is not a multiple of page size
  free_regions[0].n_pages = _size / page_table->PAGE_SIZE;

  page_table->register_pool(this);
  Console::puts("Constructed VMPool object.\n");
}

unsigned long VMPool::allocate(unsigned long _size) {
  // Rounding up here
  // unsigned long req_pages =
  //     (_size + page_table->PAGE_SIZE - 1) / page_table->PAGE_SIZE;
  // if (n_alloc >= MAX_REGIONS) {
  //   Console::puts("Out of memory regions.\n");
  //   return 0;
  // }
  //
  // for (int i = 0; i < n_free; i++) {
  //   if (free_regions[i].n_pages >= req_pages) {
  //     // allocated[n_alloc].base = free_regions
  //   }
  // }
  // return 0;
  Console::puts("Allocated region of memory.\n");
}

void VMPool::release(unsigned long _start_address) {
  assert(false);
  Console::puts("Released region of memory.\n");
}

bool VMPool::is_legitimate(unsigned long _address) {
  assert(false);
  Console::puts("Checked whether address is part of an allocated region.\n");
}
