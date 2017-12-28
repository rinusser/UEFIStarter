/**
 * Memory allocation tracker: use this to prevent memory leaks
 *
 * \author Richard Nusser
 * \copyright 2017 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \link https://github.com/rinusser/UEFIStarter
 */
#ifndef __MEMORY_H
#define __MEMORY_H

//keep this as high as possible without breaking page size
//sizeof(memory_page_list_t) should be as close to n*4096 as possible without going over it
//as of right now, 510 will result in 2 pages for 64 bit systems and 1 page for 32 bit systems
#define MEMORY_PAGE_LIST_MAX_ENTRY_COUNT 510

//like above, but for pool_memory_list_t
#define POOL_MEMORY_LIST_MAX_ENTRY_COUNT 1022


typedef struct
{
  UINTN pages;
  void *address;
} memory_page_list_entry_t;

typedef struct
{
  UINTN entry_count;
  memory_page_list_entry_t entries[MEMORY_PAGE_LIST_MAX_ENTRY_COUNT];
  struct memory_page_list_t *next;
} memory_page_list_t;

typedef struct
{
  UINTN entry_count;
  void *entries[POOL_MEMORY_LIST_MAX_ENTRY_COUNT];
  struct pool_memory_list_t *next;
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
