/** \file
 * General assertions for tests
 *
 * \author Richard Nusser
 * \copyright 2017 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 */
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include "tests.h"
#include "asserts.h"
#include "output.h"
#include "../../include/logger.h"
#include "../../include/string.h"


BOOLEAN invert_next_assert=FALSE;


#define PRINT_ASSERT_RESULT(SUCCESS) \
  VA_START(args,fmt); \
  desc=CatVSPrint(NULL,fmt,args); \
  VA_END(args); \
  print_assertion(SUCCESS,desc,message); \
  FreePool(desc);

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


//boolean

BOOLEAN assert_true(BOOLEAN actual, CHAR16 *message)
{
  return _simple_assert(actual,message,L"%ld is true",actual);
}

BOOLEAN assert_false(BOOLEAN actual, CHAR16 *message)
{
  return _simple_assert(!actual,message,L"%ld is false",actual);
}


//numeric

BOOLEAN assert_intn_equals(INTN expected, INTN actual, CHAR16 *message)
{
  return _simple_assert(actual==expected,message,L"%ld equals %ld",actual,expected);
}

BOOLEAN assert_intn_greater_than_or_equal_to(INTN expected, INTN actual, CHAR16 *message)
{
  return _simple_assert(actual>=expected,message,L"%ld is greater than or equal to %ld",actual,expected);
}

BOOLEAN assert_intn_less_than_or_equal_to(INTN expected, INTN actual, CHAR16 *message)
{
  return _simple_assert(actual<=expected,message,L"%ld is less than or equal to %ld",actual,expected);
}

BOOLEAN assert_intn_in_closed_interval(INTN min, INTN max, INTN actual, CHAR16 *message)
{
  return _simple_assert(actual>=min&&actual<=max,message,L"%ld is in closed interval [%ld,%ld]",actual,min,max);
}


BOOLEAN assert_uint64_equals(UINT64 expected, UINT64 actual, CHAR16 *message)
{
  return _simple_assert(actual==expected,message,L"%lu equals %lu",actual,expected);
}


BOOLEAN assert_double_near(double expected, double epsilon, double actual, CHAR16 *message)
{
  double delta=expected-actual;
  return _simple_assert(delta>=-epsilon&&delta<=epsilon,message,L"%s near %s+-%s",ftowcs(actual),ftowcs(expected),ftowcs(epsilon));
}

BOOLEAN assert_double_greater_than(double threshold, double actual, CHAR16 *message)
{
  return _simple_assert(actual>threshold,message,L"%s greater than %s",ftowcs(actual),ftowcs(threshold));
}

BOOLEAN assert_double_greater_than_or_equal_to(double threshold, double actual, CHAR16 *message)
{
  return _simple_assert(actual>=threshold,message,L"%s greater than or equal to %s",ftowcs(actual),ftowcs(threshold));
}

BOOLEAN assert_double_less_than(double threshold, double actual, CHAR16 *message)
{
  return _simple_assert(actual<threshold,message,L"%s less than %s",ftowcs(actual),ftowcs(threshold));
}

BOOLEAN assert_double_less_than_or_equal_to(double threshold, double actual, CHAR16 *message)
{
  return _simple_assert(actual<=threshold,message,L"%s less than or equal to %s",ftowcs(actual),ftowcs(threshold));
}


//compounds and pointers

BOOLEAN assert_null(void *actual, CHAR16 *message)
{
  return _simple_assert(actual==NULL,message,L"value %016lX is NULL",actual);
}

BOOLEAN assert_not_null(void *actual, CHAR16 *message)
{
  return _simple_assert(actual!=NULL,message,L"value is not NULL");
}

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

BOOLEAN assert_wcstr_equals(CHAR16 *expected, CHAR16 *actual, CHAR16 *message)
{
  return _simple_assert(StrCmp(expected,actual)==0,message,L"string \"%s\" matches expected \"%s\"",actual,expected);
}


BOOLEAN assert_pixel_values(UINT8 red, UINT8 green, UINT8 blue, UINT8 reserved, EFI_GRAPHICS_OUTPUT_BLT_PIXEL act, CHAR16 *message)
{
  return _simple_assert(red==act.Red && green==act.Green && blue==act.Blue && reserved==act.Reserved,
                        message,
                        L"RGBA (%d,%d,%d,%d) matches expected (%d,%d,%d,%d)",act.Red,act.Green,act.Blue,act.Reserved,red,green,blue,reserved);
}

BOOLEAN assert_pixel(EFI_GRAPHICS_OUTPUT_BLT_PIXEL exp, EFI_GRAPHICS_OUTPUT_BLT_PIXEL act, CHAR16 *message)
{
  return assert_pixel_values(exp.Red,exp.Green,exp.Blue,exp.Reserved,act,message);
}

#define ADD_CHANNEL_DELTA(EXP,CHAN) \
  delta=EXP; \
  delta-=act.CHAN; \
  if(delta<0) \
    delta=-delta; \
  sum+=delta;

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

BOOLEAN assert_pixel_near(EFI_GRAPHICS_OUTPUT_BLT_PIXEL exp, INTN epsilon, EFI_GRAPHICS_OUTPUT_BLT_PIXEL act, CHAR16 *message)
{
  return assert_pixel_values_near(exp.Red,exp.Green,exp.Blue,exp.Reserved,epsilon,act,message);
}
