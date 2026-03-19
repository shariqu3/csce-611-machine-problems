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
  assert(_size >= PageTable::PAGE_SIZE);
  base_address = _base_address;
  size = _size;
  frame_pool = _frame_pool;
  page_table = _page_table;

  allocated = (Region *)base_address;
  free_regions = (Region *)(base_address + MAX_REGIONS * sizeof(Region));

  n_alloc = 1;
  n_free = 1;

  allocated[0].base = base_address;
  allocated[0].n_pages = 1;

  unsigned long total_pages = _size / PageTable::PAGE_SIZE;

  free_regions[0].base = base_address + PageTable::PAGE_SIZE;
  // Chose to round down here if requested memory is not a multiple of page size
  free_regions[0].n_pages = total_pages - 1;

  page_table->register_pool(this);
  Console::puts("Constructed VMPool object.\n");
}

unsigned long VMPool::allocate(unsigned long _size) {
  if (n_alloc >= MAX_REGIONS) {
    Console::puts("Out of memory regions.\n");
    return 0;
  }
  // Rounding up here
  unsigned long req_pages =
      (_size + PageTable::PAGE_SIZE - 1) / PageTable::PAGE_SIZE;

  if (req_pages == 0)
    return 0;

  for (unsigned long i = 0; i < n_free; i++) {
    if (free_regions[i].n_pages >= req_pages) {
      allocated[n_alloc].base = free_regions[i].base;
      allocated[n_alloc].n_pages = req_pages;
      n_alloc++;

      if (free_regions[i].n_pages == req_pages) {
        free_regions[i] = free_regions[n_free - 1];
        n_free--;
        Console::puts("Allocated region of memory.\n");
        return allocated[n_alloc - 1].base;
      }

      else {
        free_regions[i].base =
            free_regions[i].base + (req_pages * PageTable::PAGE_SIZE);
        free_regions[i].n_pages -= req_pages;

        Console::puts("Allocated region of memory.\n");
        return allocated[n_alloc - 1].base;
      }
    }
  }
  Console::puts("Allocated failed.\n");
  return 0;
}

void VMPool::release(unsigned long _start_address) {
  for (int i = 0; i < n_alloc; i++) {
    if (allocated[i].base == _start_address) {
      free_regions[n_free] = allocated[i];
      allocated[i] = allocated[n_alloc - 1];
      n_alloc--;
      n_free++;
    }
  }

  Console::puts("Released region of memory.\n");
}

bool VMPool::is_legitimate(unsigned long _address) {
  assert(false);
  Console::puts("Checked whether address is part of an allocated region.\n");
}
