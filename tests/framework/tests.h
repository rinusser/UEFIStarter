/**
 * Basis for test suites.
 *
 * \author Richard Nusser
 * \copyright 2017 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \link https://github.com/rinusser/UEFIStarter
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


void run_tests();
void run_test(void (*func)(), CHAR16 *description);
void run_group(BOOLEAN (*func)());
BOOLEAN is_skipped_test(CHAR16 *name);

#define INIT_TESTGROUP(NAME) \
  if(is_skipped_test(NAME)) \
  { \
    global_test_results.skipped_count++; \
    return FALSE; \
  } \
  print_test_group_start(NAME);

#define FINISH_TESTGROUP() return TRUE;

#define RUN_TEST(FUNC,DESC) run_test(FUNC,DESC);

#define mark_test_incomplete() individual_test_results.incomplete_count++;


#endif
