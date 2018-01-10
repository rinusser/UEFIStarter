/** \file
 * Tests for the memory allocation tracker
 *
 * \author Richard Nusser
 * \copyright 2017-2018 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_lib_memory
 */

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include "../../../include/memory.h"
#include "../../../include/logger.h"
#include "../../framework/tests.h"


/**
 * Makes sure memory page tracking works.
 *
 * \test stopping the memory tracker with unfreed tracked pages results in a logged error
 * \test freeing tracked pages twice shouldn't work and result in a logged error
 * \test stopping the memory tracker with unfreed untracked pages should not log an error
 */
void test_page_tracking()
{
  void *ptr;
  UINTN prev_error_count;
  LOGLEVEL previous_log_level;
  BOOLEAN result;

  reset_memory_tracking();

  //allocating pages then stopping should result in error
  ptr=allocate_pages(1);
  prev_error_count=get_logger_entry_count(ERROR);
  previous_log_level=get_log_level();
  set_log_level(OFF);
  stop_tracking_memory();
  set_log_level(previous_log_level);
  assert_intn_equals(1,get_logger_entry_count(ERROR)-prev_error_count,L"1 unfreed page entry should result in 1 error on stopping");
  free_pages_ex(ptr,1,FALSE);

  //freeing unallocated tracked should result in error and rv FALSE
  prev_error_count=get_logger_entry_count(ERROR);
  previous_log_level=get_log_level();
  set_log_level(OFF);
  result=free_pages(ptr,1);
  set_log_level(previous_log_level);
  assert_intn_equals(FALSE,result,L"freeing pages twice shouldn't work");
  assert_intn_equals(1,get_logger_entry_count(ERROR)-prev_error_count,L"freeing pages twice should throw an error");

  //allocating untracked pages then stopping shouldn't throw an error
  ptr=allocate_pages_ex(1,FALSE);
  prev_error_count=get_logger_entry_count(ERROR);
  stop_tracking_memory();
  assert_intn_equals(0,get_logger_entry_count(ERROR)-prev_error_count,L"unfreed untracked page entries should be ignored on stopping");
  free_pages_ex(ptr,1,FALSE);
}

/**
 * Makes sure pool memory tracking works.
 *
 * \test free_pool_memory_entries() should return the number of freed pool memory entries
 */
void test_pool_tracking()
{
  UINTN result;

  free_pool_memory_entries();

  //tracking n pool entries then freeing pool entries should result in rv n
  track_pool_memory(AllocatePool(1));
  track_pool_memory(AllocatePool(20));
  track_pool_memory(AllocatePool(12));
  result=free_pool_memory_entries();
  assert_intn_equals(3,result,L"function should return number of freed entries");
}


/**
 * Test runner for this group.
 * Gets called via the generated test runner.
 *
 * \return whether the test group was executed
 */
BOOLEAN run_memory_tests()
{
  INIT_TESTGROUP(L"memory");
  RUN_TEST(test_page_tracking,L"page tracking");
  RUN_TEST(test_pool_tracking,L"pool tracking");
  FINISH_TESTGROUP();
}
