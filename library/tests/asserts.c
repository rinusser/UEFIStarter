/** \file
 * General assertions for tests
 *
 * \author Richard Nusser
 * \copyright 2017-2018 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_tests_asserts
 */

#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <UEFIStarter/tests/tests.h>
#include <UEFIStarter/tests/asserts.h>
#include <UEFIStarter/tests/output.h>
#include <UEFIStarter/core/logger.h>
#include <UEFIStarter/core/string.h>


/**
 * Inverts the next executed assertion: failure becomes success and vice versa.
 * This automatically gets reset to FALSE after one assertion, regardless of its result.
 */
BOOLEAN invert_next_assert=FALSE;


/**
 * Helper macro to print an assertion result
 *
 * \param SUCCESS the result to print
 */
#define PRINT_ASSERT_RESULT(SUCCESS) \
  VA_START(args,fmt); \
  desc=CatVSPrint(NULL,fmt,args); \
  VA_END(args); \
  print_assertion(SUCCESS,desc,message); \
  FreePool(desc);

/**
 * Internal assertion function.
 * This just checks whether the passed boolean value is true, any more than that is expected to be handled by the
 * caller.
 *
 * \param check   whether the assertion should pass
 * \param message an error description to include in case of failure, e.g. the variable name being checked
 * \param fmt     a descriptive message for the assertion
 * \param ...     any additional parameters for the fmt format string
 * \return whether the assertion passed
 */
static BOOLEAN EFIAPI _simple_assert(BOOLEAN check, CHAR16 *message, CHAR16 *fmt, ...)
{
  CHAR16 *desc;
  VA_LIST args;

  if(invert_next_assert)
  {
    check=!check;
    invert_next_assert=FALSE;
  }

  individual_test_results.assert_count++;
  if(check)
  {
    PRINT_ASSERT_RESULT(TRUE);
    return TRUE;
  }

  PRINT_ASSERT_RESULT(FALSE);

  individual_test_results.assert_fails++;
  return FALSE;
}

/*
 * boolean tests
 */

/**
 * Asserts a boolean value is true.
 *
 * \param actual  the value to check
 * \param message an error message to include on failure
 * \return whether the assertion passed
 */
BOOLEAN assert_true(BOOLEAN actual, CHAR16 *message)
{
  return _simple_assert(actual,message,L"%ld is true",actual);
}

/**
 * Asserts a boolean value is false.
 *
 * \param actual  the value to check
 * \param message an error message to include on failure
 * \return whether the assertion passed
 */
BOOLEAN assert_false(BOOLEAN actual, CHAR16 *message)
{
  return _simple_assert(!actual,message,L"%ld is false",actual);
}


/*
 * numeric tests
 */

/**
 * Asserts an integer value equals an expected value.
 *
 * \param expected the expected value
 * \param actual   the value to check
 * \param message  an error message to include on failure
 * \return whether the assertion passed
 */
BOOLEAN assert_intn_equals(INTN expected, INTN actual, CHAR16 *message)
{
  return _simple_assert(actual==expected,message,L"%ld equals %ld",actual,expected);
}

/**
 * Asserts an integer value is `>=expectation`.
 *
 * \param expected the expected value
 * \param actual   the value to check
 * \param message  an error message to include on failure
 * \return whether the assertion passed
 */
BOOLEAN assert_intn_greater_than_or_equal_to(INTN expected, INTN actual, CHAR16 *message)
{
  return _simple_assert(actual>=expected,message,L"%ld is greater than or equal to %ld",actual,expected);
}

/**
 * Asserts an integer value is `<=expectation`.
 *
 * \param expected the expected value
 * \param actual   the value to check
 * \param message  an error message to include on failure
 * \return whether the assertion passed
 */
BOOLEAN assert_intn_less_than_or_equal_to(INTN expected, INTN actual, CHAR16 *message)
{
  return _simple_assert(actual<=expected,message,L"%ld is less than or equal to %ld",actual,expected);
}

/**
 * Asserts an integer value is within an interval: `min <= input <= max`.
 *
 * \param min     the smallest allowed value
 * \param max     the largest allowed value
 * \param actual  the value to check
 * \param message an error message to include on failure
 * \return whether the assertion passed
 */
BOOLEAN assert_intn_in_closed_interval(INTN min, INTN max, INTN actual, CHAR16 *message)
{
  return _simple_assert(actual>=min&&actual<=max,message,L"%ld is in closed interval [%ld,%ld]",actual,min,max);
}


/**
 * Asserts a UINT64 equals an expected value.
 *
 * \param expected the expected value
 * \param actual   the value to check
 * \param message  an error message to include on failure
 * \return whether the assertion passed
 */
BOOLEAN assert_uint64_equals(UINT64 expected, UINT64 actual, CHAR16 *message)
{
  return _simple_assert(actual==expected,message,L"%lu equals %lu",actual,expected);
}


/**
 * Asserts a double value is within an epsilon radius around an expected value.
 * To be exact this must hold true: `expected-epsilon <= actual <= expected+epsilon`
 *
 * \param expected the expected value
 * \param epsilon  the highest absolute difference allowed
 * \param actual   the value to check
 * \param message  an error message to include on failure
 * \return whether the assertion passed
 */
BOOLEAN assert_double_near(double expected, double epsilon, double actual, CHAR16 *message)
{
  double delta=expected-actual;
  return _simple_assert(delta>=-epsilon&&delta<=epsilon,message,L"%s near %s+-%s",ftowcs(actual),ftowcs(expected),ftowcs(epsilon));
}

/**
 * Asserts a double value is `>expectation`.
 *
 * \param threshold the threshold the checked value must be higher than
 * \param actual    the value to check
 * \param message   an error message to include on failure
 * \return whether the assertion passed
 */
BOOLEAN assert_double_greater_than(double threshold, double actual, CHAR16 *message)
{
  return _simple_assert(actual>threshold,message,L"%s greater than %s",ftowcs(actual),ftowcs(threshold));
}

/**
 * Asserts a double value is `>=expectation`.
 *
 * \param threshold the lowest allowed value
 * \param actual    the value to check
 * \param message   an error message to include on failure
 * \return whether the assertion passed
 */
BOOLEAN assert_double_greater_than_or_equal_to(double threshold, double actual, CHAR16 *message)
{
  return _simple_assert(actual>=threshold,message,L"%s greater than or equal to %s",ftowcs(actual),ftowcs(threshold));
}

/**
 * Asserts a double value is `<expectation`.
 *
 * \param threshold the threshold the checked value must be lower than
 * \param actual    the value to check
 * \param message   an error message to include on failure
 * \return whether the assertion passed
 */
BOOLEAN assert_double_less_than(double threshold, double actual, CHAR16 *message)
{
  return _simple_assert(actual<threshold,message,L"%s less than %s",ftowcs(actual),ftowcs(threshold));
}

/**
 * Asserts a double value is `<=expectation`.
 *
 * \param threshold the highest allowed value
 * \param actual    the value to check
 * \param message   an error message to include on failure
 * \return whether the assertion passed
 */
BOOLEAN assert_double_less_than_or_equal_to(double threshold, double actual, CHAR16 *message)
{
  return _simple_assert(actual<=threshold,message,L"%s less than or equal to %s",ftowcs(actual),ftowcs(threshold));
}


/*
 * compounds and pointer tests
 */

/**
 * Asserts a pointer is NULL.
 *
 * \param actual  the pointer to check
 * \param message an error message to include on failure
 * \return whether the assertion passed
 */
BOOLEAN assert_null(void *actual, CHAR16 *message)
{
  return _simple_assert(actual==NULL,message,L"value %016lX is NULL",actual);
}

/**
 * Asserts a pointer is anything but NULL.
 *
 * \param actual  the pointer to check
 * \param message an error message to include on failure
 * \return whether the assertion passed
 */
BOOLEAN assert_not_null(void *actual, CHAR16 *message)
{
  return _simple_assert(actual!=NULL,message,L"value is not NULL");
}

/**
 * Asserts an array of UINT8 values matches an expected array's values.
 *
 * \param size     the expected array size
 * \param expected the expected values
 * \param actual   the values to check
 * \param message  an error message to include on failure
 * \return whether the assertion passed
 */
BOOLEAN assert_uint8_array(UINTN size, UINT8 *expected, UINT8 *actual, CHAR16 *message)
{
  INTN tc;
  INTN pos=-1;
  UINT8 exp_val, act_val;

  for(tc=0;tc<size;tc++)
  {
    if(expected[tc]!=actual[tc])
    {
      pos=tc;
      exp_val=expected[tc];
      act_val=actual[tc];
      break;
    }
  }
  return _simple_assert(pos<0,message,L"error at position %d: expected %d, got %d",pos,exp_val,act_val);
}

/**
 * Asserts a string equals an expected string.
 *
 * \param expected the expected string
 * \param actual   the string to check
 * \param message  an error message to include on failure
 * \return whether the assertion passed
 */
BOOLEAN assert_wcstr_equals(CHAR16 *expected, CHAR16 *actual, CHAR16 *message)
{
  return _simple_assert(StrCmp(expected,actual)==0,message,L"string \"%s\" matches expected \"%s\"",actual,expected);
}


/**
 * Asserts an EFI_GRAPHICS_OUTPUT_BLT_PIXEL has expected channel values.
 *
 * \param red      the expected red color value
 * \param green    the expected green color value
 * \param blue     the expected blue color value
 * \param reserved the expected reserved color value
 * \param act      the value to check
 * \param message  an error message to include on failure
 * \return whether the assertion passed
 */
BOOLEAN assert_pixel_values(UINT8 red, UINT8 green, UINT8 blue, UINT8 reserved, EFI_GRAPHICS_OUTPUT_BLT_PIXEL act, CHAR16 *message)
{
  return _simple_assert(red==act.Red && green==act.Green && blue==act.Blue && reserved==act.Reserved,
                        message,
                        L"RGBA (%d,%d,%d,%d) matches expected (%d,%d,%d,%d)",act.Red,act.Green,act.Blue,act.Reserved,red,green,blue,reserved);
}

/**
 * Asserts an EFI_GRAPHICS_OUTPUT_BLT_PIXEL equals an expected pixel.
 * This also checks the .Reserved channel.
 *
 * \param exp     the expected pixel
 * \param act     the pixel to check
 * \param message an error message to include on failure
 * \return whether the assertion passed
 */
BOOLEAN assert_pixel(EFI_GRAPHICS_OUTPUT_BLT_PIXEL exp, EFI_GRAPHICS_OUTPUT_BLT_PIXEL act, CHAR16 *message)
{
  return assert_pixel_values(exp.Red,exp.Green,exp.Blue,exp.Reserved,act,message);
}

/**
 * Helper macro to calculate the absolute difference between expected and actual channel values
 *
 * \param EXP  the expected channel value
 * \param CHAN the actual pixel's color channel to look in
 */
#define ADD_CHANNEL_DELTA(EXP,CHAN) \
  delta=EXP; \
  delta-=act.CHAN; \
  if(delta<0) \
    delta=-delta; \
  sum+=delta;

/**
 * Asserts an EFI_GRAPHICS_OUTPUT_BLT_PIXEL has expected channel values or the difference is within a given radius.
 * The difference between two pixels is calculated as the sum of absolute differences in each channel.
 *
 * \param red      the expected red color value
 * \param green    the expected green color value
 * \param blue     the expected blue color value
 * \param reserved the expected reserved color value
 * \param epsilon  the highest absolute differences sum over all color channels allowed
 * \param act      the pixel to check
 * \param message  an error message to include on failure
 * \return whether the assertion passed
 */
BOOLEAN assert_pixel_values_near(UINT8 red, UINT8 green, UINT8 blue, UINT8 reserved, INTN epsilon, EFI_GRAPHICS_OUTPUT_BLT_PIXEL act, CHAR16 *message)
{
  INTN sum=0;
  INTN delta;

  ADD_CHANNEL_DELTA(red,Red);
  ADD_CHANNEL_DELTA(green,Green);
  ADD_CHANNEL_DELTA(blue,Blue);
  ADD_CHANNEL_DELTA(reserved,Reserved);

  return _simple_assert(sum<=epsilon,
                        message,
                        L"RGBA (%d,%d,%d,%d) matches expected (%d,%d,%d,%d) within epsilon=%d",act.Red,act.Green,act.Blue,act.Reserved,red,green,blue,reserved,epsilon);
}

/**
 * Asserts an EFI_GRAPHICS_OUTPUT_BLT_PIXEL equals an expected pixel or the difference is within a given radius.
 * The difference between two pixels is calculated as the sum of absolute differences in each channel.
 *
 * This also checks the .Reserved channel.
 *
 * \param exp     the expected pixel
 * \param epsilon the highest absolute differences sum allowed
 * \param act     the pixel to check
 * \param message an error message to include on failure
 * \return whether the assertion passed
 */
BOOLEAN assert_pixel_near(EFI_GRAPHICS_OUTPUT_BLT_PIXEL exp, INTN epsilon, EFI_GRAPHICS_OUTPUT_BLT_PIXEL act, CHAR16 *message)
{
  return assert_pixel_values_near(exp.Red,exp.Green,exp.Blue,exp.Reserved,epsilon,act,message);
}
