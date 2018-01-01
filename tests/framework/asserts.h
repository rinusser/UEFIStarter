/** \file
 * General assertions for tests
 *
 * \author Richard Nusser
 * \copyright 2017 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_tests_asserts
 */

#ifndef __TEST_ASSERTS_H
#define __TEST_ASSERTS_H

#include <Uefi.h>
#include <Library/UefiLib.h>


//invert next assertion check - there probably won't be a use case for this other than testing assertions themselves
extern BOOLEAN invert_next_assert;

//boolean

BOOLEAN assert_true(BOOLEAN actual, CHAR16 *message);
BOOLEAN assert_false(BOOLEAN actual, CHAR16 *message);


//numbers

BOOLEAN assert_intn_equals(INTN expected, INTN actual, CHAR16 *message);
BOOLEAN assert_intn_greater_than_or_equal_to(INTN expected, INTN actual, CHAR16 *message);
BOOLEAN assert_intn_less_than_or_equal_to(INTN expected, INTN actual, CHAR16 *message);
BOOLEAN assert_intn_in_closed_interval(INTN min, INTN max, INTN actual, CHAR16 *message);

BOOLEAN assert_uint64_equals(UINT64 expected, UINT64 actual, CHAR16 *message);

BOOLEAN assert_double_near(double expected, double epsilon, double actual, CHAR16 *message);
BOOLEAN assert_double_greater_than(double threshold, double actual, CHAR16 *message);
BOOLEAN assert_double_greater_than_or_equal_to(double threshold, double actual, CHAR16 *message);
BOOLEAN assert_double_less_than(double threshold, double actual, CHAR16 *message);
BOOLEAN assert_double_less_than_or_equal_to(double threshold, double actual, CHAR16 *message);


//pointers and compounds

BOOLEAN assert_null(void *actual, CHAR16 *message);
BOOLEAN assert_not_null(void *actual, CHAR16 *message);

BOOLEAN assert_uint8_array(UINTN size, UINT8 *expected, UINT8 *actual, CHAR16 *message);
BOOLEAN assert_wcstr_equals(CHAR16 *expected, CHAR16 *actual, CHAR16 *message);

BOOLEAN assert_pixel(EFI_GRAPHICS_OUTPUT_BLT_PIXEL expected, EFI_GRAPHICS_OUTPUT_BLT_PIXEL actual, CHAR16 *message);
BOOLEAN assert_pixel_values(UINT8 red, UINT8 green, UINT8 blue, UINT8 reserved, EFI_GRAPHICS_OUTPUT_BLT_PIXEL actual, CHAR16 *message);
BOOLEAN assert_pixel_near(EFI_GRAPHICS_OUTPUT_BLT_PIXEL expected, INTN epsilon, EFI_GRAPHICS_OUTPUT_BLT_PIXEL actual, CHAR16 *message);
BOOLEAN assert_pixel_values_near(UINT8 red, UINT8 green, UINT8 blue, UINT8 reserved, INTN epsilon, EFI_GRAPHICS_OUTPUT_BLT_PIXEL actual, CHAR16 *message);


#endif
