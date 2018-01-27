/** \file
 * Common types for tests
 *
 * \author Richard Nusser
 * \copyright 2017-2018 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_tests_runner
 */

#ifndef __TEST_TYPES_H
#define __TEST_TYPES_H

#include <Uefi.h>


/** data type for test verbosity configuration */
typedef struct
{
  UINTN individual_groups:1;       /**< whether individual test groups should be shown */
  UINTN individual_tests:1;        /**< whether individual tests should be shown */
  UINTN one_char_per_test:1;       /**< whether there should be a one character summary per test */
  UINTN multiple_lines_per_test:1; /**< whether test output should span multiple lines */
  UINTN assertion_counts:1;        /**< whether the number of assertions should be shown */
  UINTN individual_assertions:1;   /**< whether individual assertions should be shown */
  UINTN summary_statistics:1;      /**< whether test result statistics should be shown */
} test_verbosity_t;

/** data type for test outcome status */
typedef enum
{
  FAILURE=0,  /**< the test failed */
  INCOMPLETE, /**< the test was incomplete */
  SUCCESS     /**< the test succeeded */
} test_outcome;

/** data type for test results */
typedef struct
{
  INT64 assert_count;          /**< the total number of assertions */
  INT64 assert_fails;          /**< the number of failed assertions */
  INT64 successful_test_count; /**< the number of succeeded tests */
  INT64 failed_test_count;     /**< the number of failed tests */
  INT64 incomplete_count;      /**< the number of incomplete tests */
  INT64 skipped_count;         /**< the number of skipped tests */
  test_outcome outcome;        /**< the test outcome */
} test_results_t;


#endif
