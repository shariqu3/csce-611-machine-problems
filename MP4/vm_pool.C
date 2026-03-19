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

/**
 * @brief Constructs a Virtual Memory Pool.
 * * Initializes the pool with a base virtual address and size. It sets up
 * the metadata tracking (allocated and free lists) within the very first
 * page of the virtual address space it manages.
 * * @param _base_address The starting virtual address for this pool.
 * @param _size Total size of the virtual address space in bytes.
 * @param _frame_pool The physical frame pool associated with this VM pool.
 * @param _page_table The page table used to register this pool and manage
 * translations.
 */
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

  page_table->register_pool(this);
  allocated[0].base = base_address;
  allocated[0].n_pages = 1;

  unsigned long total_pages = _size / PageTable::PAGE_SIZE;

  free_regions[0].base = base_address + PageTable::PAGE_SIZE;
  // Chose to round down here if requested memory is not a multiple of page size
  free_regions[0].n_pages = total_pages - 1;

  Console::puts("Constructed VMPool object.\n");
}

/**
 * @brief Allocates a contiguous region of virtual memory.
 * * Searches the free list for a region large enough to satisfy the request.
 * If found, it updates the free list and adds the new region to the
 * allocated list.
 * * @param _size The number of bytes to allocate.
 * @return The starting virtual address of the allocated region, or 0 if
 * allocation fails.
 * @note The size is rounded up to the nearest page boundary.
 */
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

/**
 * @brief Releases an allocated region of virtual memory.
 * * Identifies the region starting at the given address, tells the page
 * table to free the associated physical frames, and returns the
 * virtual range to the free list.
 * * @param _start_address The virtual address where the region to be freed
 * starts.
 * @warning Attempting to free the base address (metadata page) will fail
 * to protect the VMPool's management structures.
 */
void VMPool::release(unsigned long _start_address) {
  if (n_free >= MAX_REGIONS) {
    Console::puts("Free memory list is already at max capacity.\n");
    return;
  }

  if (_start_address == base_address) {
    Console::puts("Error: Trying to free metadata page containing free list "
                  "and allocated list.\n");
    return;
  }

  for (unsigned long i = 0; i < n_alloc; i++) {
    if (allocated[i].base == _start_address) {
      unsigned long pages = allocated[i].n_pages;
      unsigned long base = allocated[i].base;

      for (unsigned long j = 0; j < pages; j++) {
        unsigned long page_no = (base / PageTable::PAGE_SIZE) + j;
        page_table->free_page(page_no);
      }

      free_regions[n_free] = allocated[i];
      allocated[i] = allocated[n_alloc - 1];
      n_alloc--;
      n_free++;
      Console::puts("Released region of memory.\n");
      return;
    }
  }

  Console::puts("No region found with the given start_address.\n");
}

/**
 * @brief Checks if a given virtual address is part of a valid allocation.
 * * Used by the Page Table's fault handler to determine if a page fault
 * was caused by a legitimate access or a segmentation fault.
 * * @param _address The virtual address to validate.
 * @return true if the address is within an allocated region (including
 * metadata), false otherwise.
 */
bool VMPool::is_legitimate(unsigned long _address) {
  if (_address >= base_address &&
      _address < base_address + PageTable::PAGE_SIZE) {
    Console::puts("The address is a part of the metadat region.\n");
    return true;
  }
  for (unsigned long i = 0; i < n_alloc; i++) {

    unsigned long start = allocated[i].base;
    unsigned long end = start + allocated[i].n_pages * PageTable::PAGE_SIZE;

    if (_address >= start && _address < end)
      return true;
  }
  Console::puts("The address is not a part of an allocated region.\n");
  return false;
}
