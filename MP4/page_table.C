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
  // Initialize page directory
  unsigned long frame = kernel_mem_pool->get_frames(1);
  page_directory = (unsigned long *)(frame * 4 KB);

  // Initialize first page table for direct mapping of shared size
  frame = kernel_mem_pool->get_frames(1);
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
  unsigned long fault_address = read_cr2();

  unsigned long dir_idx = fault_address >> 22;
  unsigned long pt_idx = (fault_address << 10) >> 22;
  unsigned long offset = fault_address & ((1 << 12) - 1);

  unsigned long pde_entry = current_page_table->page_directory[dir_idx];

  unsigned long *page_table;
  unsigned long pte_entry;
  if ((pde_entry & 1) == 0) {
    unsigned long new_frame = kernel_mem_pool->get_frames(1);
    page_table = (unsigned long *)(new_frame * 4 KB);

    for (int i = 0; i < 1024; i++) {
      // Enabling R/W bit and invalid bit
      page_table[i] = 0 | 2;
    }
    current_page_table->page_directory[dir_idx] = (new_frame << 12) | 3;
    pte_entry = 0 | 2;
  } else {
    unsigned long pt_frame = pde_entry >> 12;
    page_table = (unsigned long *)(pt_frame * 4 KB);
    pte_entry = page_table[pt_idx];
  }

  if (pte_entry & 1) {
    Console::puts("A valid page table from a valid page directroy throws "
                  "handle fault error\n");
    assert(false);
  } else {
    unsigned long new_frame = process_mem_pool->get_frames(1);
    page_table[pt_idx] = (new_frame << 12) | 3;
  }
}
unsigned long *PageTable::PDE_address(unsigned long addr) {
  assert(false);
  return nullptr;
}
unsigned long *PageTable::PTE_address(unsigned long addr) {
  assert(false);
  return nullptr;
}

void PageTable::register_pool(VMPool *_vm_pool) {
  assert(false);
  Console::puts("registered VM pool\n");
}

void PageTable::free_page(unsigned long _page_no) {
  assert(false);
  Console::puts("freed page\n");
}
