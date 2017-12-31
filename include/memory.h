/** \file
 * Memory allocation tracker: use this to prevent memory leaks
 *
 * \author Richard Nusser
 * \copyright 2017 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_lib_memory
 */

#ifndef __MEMORY_H
#define __MEMORY_H

/**
 * maximum number of entries per memory page list node
 *
 * Keep this as high as possible without breaking page size: sizeof(memory_page_list_t) should be as close to n*4096
 * as possible without going over it.
 *
 * As of right now 510 will result in 2 pages for 64 bit systems and 1 page for 32 bit systems.
 */
#define MEMORY_PAGE_LIST_MAX_ENTRY_COUNT 510

/** like MEMORY_PAGE_LIST_MAX_ENTRY_COUNT, but for pool_memory_list_t */
#define POOL_MEMORY_LIST_MAX_ENTRY_COUNT 1022


/** type for a single tracked allocation of memory pages */
typedef struct
{
  UINTN pages;    /**< the number of pages allocated */
  void *address;  /**< the memory location of the first page */
} memory_page_list_entry_t;

/** type for a list node of tracked memory page allocations */
typedef struct
{
  UINTN entry_count; /**< the number of entries in this node */
  memory_page_list_entry_t entries[MEMORY_PAGE_LIST_MAX_ENTRY_COUNT]; /**< the allocation entries in this node */
  struct memory_page_list_t *next; /**< the next list node, may be null */
} memory_page_list_t;

/** type for a list node of tracked pool memory */
typedef struct
{
  UINTN entry_count;                               /**< the number of entries in this node */
  void *entries[POOL_MEMORY_LIST_MAX_ENTRY_COUNT]; /**< the allocation entries in this node */
  struct pool_memory_list_t *next;                 /**< the next list node, may be null */
} pool_memory_list_t;


void reset_memory_tracking();


void *allocate_pages(UINTN pages);
BOOLEAN free_pages(void *address, UINTN pages);

void *allocate_pages_ex(UINTN pages, BOOLEAN track);
BOOLEAN free_pages_ex(void *address, UINTN pages, BOOLEAN track);


void track_pool_memory(void *address);
UINTN free_pool_memory_entries();


void print_memory_page_list();
void print_pool_memory_list();

void init_tracking_memory();
UINTN stop_tracking_memory();


#endif
