#include "assert.H"
#include "console.H"
#include "exceptions.H"
#include "page_table.H"
#include "paging_low.H"

#define MB *(0x1 << 20)
#define KB *(0x1 << 10)

PageTable *PageTable::current_page_table = nullptr;
unsigned int PageTable::paging_enabled = 0;
ContFramePool *PageTable::kernel_mem_pool = nullptr;
ContFramePool *PageTable::process_mem_pool = nullptr;
unsigned long PageTable::shared_size = 0;

/**
 * @brief Initializes the paging subsystem.
 *
 * This function sets up the paging system by assigning the memory frame pools
 * used by the kernel and user processes, and configuring the size of the
 * shared memory region.
 *
 * The shared memory region must be exactly 4 MB. An assertion is used to
 * enforce this constraint during initialization.
 *
 * @param _kernel_mem_pool   Pointer to the contiguous frame pool used by the
 * kernel. This pool manages physical frames reserved for kernel memory
 * allocations.
 *
 * @param _process_mem_pool  Pointer to the contiguous frame pool used by user
 *                           processes. This pool manages physical frames
 *                           allocated to processes.
 *
 * @param _shared_size       Size (in bytes) of the shared memory region.
 *                           Must be exactly 4 MB.
 *
 * @note This function must be called before paging is enabled.
 * @warning The function will trigger an assertion failure if _shared_size
 *          is not equal to 4 MB.
 */
void PageTable::init_paging(ContFramePool *_kernel_mem_pool,
                            ContFramePool *_process_mem_pool,
                            const unsigned long _shared_size) {
  kernel_mem_pool = _kernel_mem_pool;
  process_mem_pool = _process_mem_pool;
  shared_size = _shared_size;
  assert(shared_size == 4 MB);
  Console::puts("Initialized Paging System\n");
}

/**
 * @brief Constructs and initializes a new page table.
 *
 * Allocates and sets up a page directory and an initial page table using
 * frames from the kernel memory pool.
 *
 * The first page table is created to establish an identity (direct) mapping
 * for the shared memory region. Each entry maps a virtual address to the
 * same physical address, with the Read/Write and Present (Valid) bits enabled.
 *
 * The first entry of the page directory points to this initialized page table
 * with appropriate permission bits set. All remaining page directory entries
 * are marked as Read/Write but not present (invalid), indicating that no
 * additional page tables are currently allocated.
 */
PageTable::PageTable() {
  n_registered_pools = 0;

  // Initialize page directory
  unsigned long frame = kernel_mem_pool->get_frames(1);
  page_directory = (unsigned long *)(frame * 4 KB);

  // Initialize first page table for direct mapping of shared size
  frame = process_mem_pool->get_frames(1);
  unsigned long address = 0;

  unsigned long *page_table = (unsigned long *)(frame * 4 KB);

  for (int i = 0; i < shared_size / (4 KB); i++) {
    // Enabling R/W bit and Valid bit
    page_table[i] = address | 3;
    address += 4 KB;
  }

  page_directory[0] = (unsigned long)page_table | 3;
  for (int i = 1; i < 1024; i++) {
    // Marking them as read write and invalid
    page_directory[i] = 0 | 2;
  }
  page_directory[1023] = (unsigned long)page_directory | 3;

  Console::puts("Constructed Page Table object\n");
}

/**
 * @brief Loads this page table into the processor.
 *
 * Sets this PageTable instance as the currently active page table and
 * updates the CR3 control register with the physical address of the
 * page directory.
 *
 * Writing to CR3 causes the CPU to use this page directory for address
 * translation and typically flushes the TLB.
 */
void PageTable::load() {
  current_page_table = this;
  write_cr3((unsigned long)page_directory);
  Console::puts("Loaded page table\n");
}

/**
 * @brief Enables paging in the processor.
 *
 * Sets the Paging bit in the CR0 control register to activate
 * hardware-supported virtual memory address translation.
 *
 * Once enabled, all memory accesses are translated through the currently
 * loaded page directory (CR3). The paging_enabled flag is updated to
 * reflect the new system state.
 */
void PageTable::enable_paging() {
  write_cr0(read_cr0() | 0x80000000);
  paging_enabled = 1;
  Console::puts("Enabled paging.\n");
}

/**
 * @brief Handles a page fault exception.
 *
 * Resolves page faults by dynamically allocating page tables and/or
 * physical frames as needed.
 *
 * The faulting virtual address is obtained from CR2 and decomposed into:
 *  - Page Directory index
 *  - Page Table index
 *  - Offset within the page
 *
 * If the corresponding page directory entry is not present, a new page
 * table is allocated from the kernel memory pool, initialized as
 * read/write but not present, and inserted into the page directory.
 *
 * If the page table entry is not present, a new physical frame is
 * allocated from the process memory pool and mapped with Present and
 * Read/Write permissions enabled.
 *
 * If a fault occurs on an already-present page table entry, the situation
 * is considered erroneous and triggers an assertion failure.
 *
 * @param _r Pointer to the saved processor register state at the time
 *           of the fault (not used directly in this implementation).
 */
void PageTable::handle_fault(REGS *_r) {
  (void)_r;
  unsigned long fault_address = read_cr2();

  bool legitimate = (fault_address < shared_size);
  if (!legitimate) {
    for (unsigned int i = 0; i < current_page_table->n_registered_pools; i++) {
      if (current_page_table->registered_pools[i]->is_legitimate(
              fault_address)) {
        legitimate = true;
        break;
      }
    }
  }

  if (!legitimate) {
    Console::puts("SEGMENTATION FAULT: illegitimate memory access\n");
    abort();
  }

  unsigned long dir_idx = fault_address >> 22;
  unsigned long *pde = current_page_table->PDE_address(fault_address);

  if ((*pde & 1) == 0) {
    unsigned long new_frame = process_mem_pool->get_frames(1);
    assert(new_frame != 0);
    *pde = (new_frame * PAGE_SIZE) | 3;

    unsigned long *new_pt = (unsigned long *)(0xFFC00000 + (dir_idx << 12));
    for (int i = 0; i < 1024; i++) {
      // Enabling R/W bit and invalid bit
      new_pt[i] = 2;
    }
  }

  unsigned long *pte = current_page_table->PTE_address(fault_address);

  if (*pte & 1) {
    Console::puts("A valid page table from a valid page directroy throws "
                  "handle fault error\n");
    assert(false);
  } else {
    unsigned long new_frame = process_mem_pool->get_frames(1);
    assert(new_frame != 0);
    *pte = (new_frame * PAGE_SIZE) | 3;
  }
  write_cr3(read_cr3());
}

unsigned long *PageTable::PDE_address(unsigned long addr) {
  unsigned long X = addr >> 22;
  return (unsigned long *)(0xFFFFF000 + (X << 2));
}

unsigned long *PageTable::PTE_address(unsigned long addr) {
  unsigned long X = addr >> 22;
  unsigned long Y = (addr << 10) >> 22;
  return (unsigned long *)(0xFFC00000 + (X << 12) + (Y << 2));
}

void PageTable::register_pool(VMPool *_vm_pool) {
  assert(_vm_pool != nullptr);
  assert(n_registered_pools < MAX_VM_POOLS);
  registered_pools[n_registered_pools] = _vm_pool;
  n_registered_pools++;
  Console::puts("registered VM pool\n");
}

void PageTable::free_page(unsigned long _page_no) {
  unsigned long page_addr = _page_no * PAGE_SIZE;
  unsigned long *pde = current_page_table->PDE_address(page_addr);
  if ((*pde & 1) == 0) {
    Console::puts("Freeing incorrect page\n");
    return;
  }

  unsigned long *pte = current_page_table->PTE_address(page_addr);
  if ((*pte & 1) == 0) {
    Console::puts("Freeing incorrect page\n");
    return;
  }

  unsigned long frame_no = (*pte & 0xFFFFF000) / PAGE_SIZE;
  ContFramePool::release_frames(frame_no);
  *pte = 0 | 2;

  write_cr3(read_cr3());
  Console::puts("freed page\n");
}
