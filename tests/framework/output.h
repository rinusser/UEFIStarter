/** \file
 * Presentation logic for tests
 *
 * \author Richard Nusser
 * \copyright 2017 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 */
#ifndef __TEST_OUTPUT_H
#define __TEST_OUTPUT_H

#include <Uefi.h>
#include "types.h"


void print_test_group_start(CHAR16 *name);
void print_group_result(test_results_t *results);
void print_test_group_end();

void print_individual_test_start(CHAR16 *description);
void print_individual_result(test_results_t *results);
void print_assert_counts(INT64 fails, INT64 asserts);
void print_test_result_summary(test_results_t *results);
void print_assertion(BOOLEAN success, CHAR16 *description, CHAR16 *message);

void debug_print_verbosity();
void debug_print_results(test_results_t *results);


#endif
