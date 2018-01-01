/** \file
 * Basis for test suites.
 *
 * \author Richard Nusser
 * \copyright 2017 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_tests_runner
 */

#ifndef __TESTS_H
#define __TESTS_H

#include <Uefi.h>
#include "types.h"
#include "asserts.h"
#include "output.h"
#include "../../include/console.h"


extern test_results_t individual_test_results;
extern test_results_t global_test_results;
extern test_verbosity_t test_verbosity;


/**
 * Runs a test group.
 * This gets created automatically (in generated/runner.c in the test suite's directory).
 */
void run_tests();

void run_test(void (*func)(), CHAR16 *description);
void run_group(BOOLEAN (*func)());
BOOLEAN is_skipped_test(CHAR16 *name);

/**
 * Helper macro to start a testgroup.
 *
 * \param NAME the test group's name
 */
#define INIT_TESTGROUP(NAME) \
  if(is_skipped_test(NAME)) \
  { \
    global_test_results.skipped_count++; \
    return FALSE; \
  } \
  print_test_group_start(NAME);

/**
 * Helper macro to end a testgroup.
 */
#define FINISH_TESTGROUP() return TRUE;

/**
 * Helper macro to run a test.
 *
 * \param FUNC the test's function to execute
 * \param DESC the test's description
 */
#define RUN_TEST(FUNC,DESC) run_test(FUNC,DESC);

/** Helper macro to mark a test as incomplete */
#define mark_test_incomplete() individual_test_results.incomplete_count++;


#endif
