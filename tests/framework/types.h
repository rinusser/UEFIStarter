/**
 * Common types for tests
 *
 * \author Richard Nusser
 * \copyright 2017 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \link https://github.com/rinusser/UEFIStarter
 */
#ifndef __TEST_TYPES_H
#define __TEST_TYPES_H

#include <Uefi.h>


typedef struct
{
  UINTN individual_groups:1;
  UINTN individual_tests:1;
  UINTN one_char_per_test:1;
  UINTN multiple_lines_per_test:1;
  UINTN assertion_counts:1;
  UINTN individual_assertions:1;
  UINTN summary_statistics:1;
} test_verbosity_t;

typedef enum
{
  FAILURE=0,
  INCOMPLETE,
  SUCCESS
} test_outcome;

typedef struct
{
  INT64 assert_count;
  INT64 assert_fails;
  INT64 successful_test_count;
  INT64 failed_test_count;
  INT64 incomplete_count;
  INT64 skipped_count;
  test_outcome outcome;
} test_results_t;


#endif
