/** \file
 * Memory tracker, will find memory leaks
 *
 * \author Richard Nusser
 * \copyright 2017-2018 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_lib_memory
 */

#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include "../include/memory.h"
#include "../include/logger.h"

/** the number of pages required for each allocation list node */
#define MEMORY_PAGE_LIST_PAGE_COUNT ((sizeof(memory_page_list_t)-1)/4096+1)

void *allocate_pages_ex(UINTN,BOOLEAN);

/** internal pointer to first memory page allocation list node */
static memory_page_list_t *_memory_page_list;

/** internal pointer to first pool memory allocation list node */
static pool_memory_list_t _pool_memory_list;


/**
 * Debugging function: prints a human-readable list of tracked memory pages.
 */
void print_memory_page_list()
{
  unsigned int tc;
  if(_memory_page_list==NULL)
  {
    Print(L"  memory page list is empty.\n");
    return;
  }
  Print(L"entries: %d\n",_memory_page_list->entry_count);
  if(_memory_page_list->entry_count>MEMORY_PAGE_LIST_MAX_ENTRY_COUNT)
  {
    LOG.error(L"memory page list entry count invalid");
    return;
  }
  for(tc=0;tc<_memory_page_list->entry_count;tc++)
  {
    Print(L"  entry %03d: %lX, %d page(s)\n",tc,_memory_page_list->entries[tc].address,_memory_page_list->entries[tc].pages);
  }
}

/**
 * Resets the internal memory tracking.
 *
 * This will forget all currently tracked memory, resulting in untracked memory leaks if you're not freeing those pages
 * later.
 */
void reset_memory_tracking()
{
  _memory_page_list=NULL;
}

/**
 * internal: finds next memory page allocation list entry
 *
 * \param page_list the page list node to look in
 * \param index     the index variable to write any matches to
 * \return whether the lookup succeeded
 */
static BOOLEAN _get_next_free_entry(memory_page_list_t **page_list, UINTN *index)
{
  unsigned int tc;

  if(!_memory_page_list)
  {
    _memory_page_list=allocate_pages_ex(MEMORY_PAGE_LIST_PAGE_COUNT,FALSE);
    LOG.trace(L"memory page list is at %lX",_memory_page_list);
    _memory_page_list->entry_count=0;
    _memory_page_list->next=NULL;
  }
  if(!_memory_page_list)
    return FALSE;

  for(tc=0;tc<_memory_page_list->entry_count;tc++)
  {
    if(_memory_page_list->entries[tc].address==NULL)
    {
      *index=tc;
      *page_list=_memory_page_list;
      return TRUE;
    }
  }
  
  /** \TODO if there's a next page_list, check that. if not, allocate new page_list and link to it */
  if(_memory_page_list->entry_count+1>=MEMORY_PAGE_LIST_MAX_ENTRY_COUNT)
    return FALSE;

  *index=_memory_page_list->entry_count;
  *page_list=_memory_page_list;
  return TRUE;
}

/**
 * internal: finds a page list entry with the given start address
 *
 * \param address the first page's address to look for
 * \return a pointer to the list entry, or NULL if address isn't tracked
 */
static memory_page_list_entry_t *_find_page_list_entry(void *address)
{
  unsigned int tc;
  if(!_memory_page_list)
    return NULL;
  for(tc=0;tc<_memory_page_list->entry_count;tc++)
    if(_memory_page_list->entries[tc].address==address)
      return &_memory_page_list->entries[tc];
  return NULL;
}

/**
 * internal: allocates memory pages.
 * This function isn't static - if you declare it with "extern" in a header you can access it directly.
 *
 * \param pages the amount of pages to allocate
 * \param track whether to track the pages
 * \return the first page's address, or NULL on error
 */
void *allocate_pages_ex(UINTN pages, BOOLEAN track)
{
  EFI_STATUS result;
  EFI_PHYSICAL_ADDRESS address;
  memory_page_list_t *page_list;
  UINTN index;

  if(track)
  {
    if(!_get_next_free_entry(&page_list,&index))
      return NULL;
    LOG.trace(L"got next free page list entry index %ld",index);
  }

  result=gST->BootServices->AllocatePages(AllocateAnyPages,EfiLoaderData,pages,&address);
  if(result!=EFI_SUCCESS)
  {
    LOG.error(L"could not allocate %d page(s): %r",pages,result);
    return NULL;
  }
  LOG.debug(L"allocated %d page(s) at %016lX",pages,address);

  if(track)
  {
    page_list->entries[index].address=(void *)address;
    page_list->entries[index].pages=pages;
    if(page_list->entry_count<=index)
      page_list->entry_count=index+1;
  }

  return (void *)address;
}

/**
 * Allocate tracked memory pages
 *
 * \param pages the number of pages to allocate
 * \return the first page's address, or NULL on error
 */
void *allocate_pages(UINTN pages)
{
  return allocate_pages_ex(pages,TRUE);
}

/**
 * internal: frees memory pages.
 * This function isn't static - if you declare it with "extern" in a header you can access it directly.
 *
 * \param address the first page's address
 * \param pages   the number of pages allocated
 * \param track   whether the pages were tracked
 * \return whether the pages were freed successfully
 */
BOOLEAN free_pages_ex(void *address, UINTN pages, BOOLEAN track)
{
  EFI_STATUS result;
  memory_page_list_entry_t *entry;

  if(track)
  {  
    entry=_find_page_list_entry(address);
    if(!entry)
    {
      LOG.error(L"trying to free memory with no page list entry: %016lX",address);
      return FALSE;
    }

    if(entry->pages!=pages)
      LOG.warn(L"trying to free %ld page(s) at %016lX, but it had %ld page(s)",pages,address,entry->pages);
  }

  result=gST->BootServices->FreePages((EFI_PHYSICAL_ADDRESS)address,pages);
  if(result!=EFI_SUCCESS)
  {
    LOG.error(L"could not free %d page(s) at %016lX: %r",pages,address,result);
    return FALSE;
  }

  LOG.debug(L"freed %d page(s) at %016lX",pages,address);

  if(track)
  {
    entry->address=NULL;
    entry->pages=0;
  }

  return TRUE;
}

/**
 * Frees tracked memory pages.
 *
 * \param address the first page's address
 * \param pages   the number of pages allocated
 * \return whether the pages were freed successfully
 */
BOOLEAN free_pages(void *address, UINTN pages)
{
  return free_pages_ex(address,pages,TRUE);
}


/**
 * Starts tracking a pool memory address.
 *
 * Once pool memory is tracked the next call to free_pool_memory_entries() will free all currently tracked pool memory
 * entries. Consider this function call to mean "I don't need this piece of memory anymore, free it whenever".
 *
 * \param address the start of the pool memory
 *
 * \TODO implement linked list for pool memory tracking
 */
void track_pool_memory(void *address)
{
  UINTN tc;

  LOG.trace(L"adding %016lX to pool memory list",address);

  for(tc=0;tc<_pool_memory_list.entry_count;tc++)
  {
    if(_pool_memory_list.entries[tc]==NULL)
    {
      _pool_memory_list.entries[tc]=address;
      return;
    }
  }
  if(tc>=POOL_MEMORY_LIST_MAX_ENTRY_COUNT)
  {
    LOG.error(L"pool memory list full, either free more often or implement linked list");
    return;
  }
  _pool_memory_list.entries[tc]=address;
  _pool_memory_list.entry_count++;
}

/**
 * Debugging function: prints a human-readable list of tracked pool memory entries.
 */
void print_pool_memory_list()
{
  UINTN tc;
  Print(L"pool memory entry count: %d\n",_pool_memory_list.entry_count);
  for(tc=0;tc<_pool_memory_list.entry_count+3;tc++)
    Print(L"  entry %d: %016lX\n",tc,_pool_memory_list.entries[tc]);
}

/**
 * Frees all currently tracked pool memory entries.
 *
 * \return the number of entries freed.
 */
UINTN free_pool_memory_entries()
{
  UINTN rv=0;
  UINTN tc;

  for(tc=0;tc<_pool_memory_list.entry_count;tc++)
  {
    if(_pool_memory_list.entries[tc]!=NULL)
    {
      FreePool(_pool_memory_list.entries[tc]);
      _pool_memory_list.entries[tc]=NULL;
      rv++;
    }
  }
  _pool_memory_list.entry_count=0;

  LOG.debug(L"freed %d pool memory entries",rv);
  return rv;
}


/**
 * Initialize the memory tracker.
 */
void init_tracking_memory()
{
  _memory_page_list=NULL;
  _pool_memory_list.entry_count=0;
}

/**
 * Stops tracking all memory.
 * This will log errors if there are unfreed memory pages.
 *
 * \return the number of errors, 0 if there were no tracked memory pages remaining
 */
UINTN stop_tracking_memory()
{
  unsigned int tc;
  UINTN errors=0;

  free_pool_memory_entries(); /** \XXX the free_pool_memory_entries() call was at the end of this function - make sure there aren't any side-effects from calling this before the NULL check below */

  if(_memory_page_list==NULL)
    return 0;

  LOG.trace(L"memory page list is at %016lX, entry_count=%d",_memory_page_list,_memory_page_list->entry_count);

  if(_memory_page_list->entry_count>MEMORY_PAGE_LIST_MAX_ENTRY_COUNT)
  {
    LOG.error(L"memory page list corrupt: number of entries (%d) above maximum (%d)",_memory_page_list->entry_count,MEMORY_PAGE_LIST_MAX_ENTRY_COUNT);
    return 1;
  }

  for(tc=0;tc<_memory_page_list->entry_count;tc++)
  {
    if(_memory_page_list->entries[tc].address!=NULL)
    {
      errors++;
      LOG.error(L"unfreed memory at %016lX (%d page(s))",_memory_page_list->entries[tc].address,_memory_page_list->entries[tc].pages);
    }
  }

  if(!free_pages_ex(_memory_page_list,MEMORY_PAGE_LIST_PAGE_COUNT,FALSE))
    errors++;

  _memory_page_list=NULL;

  return errors;
}
