/** \file
 * Self-tests for assertions
 *
 * \author Richard Nusser
 * \copyright 2017 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 */
#include <Uefi.h>
#include <Library/UefiLib.h>
#include "../../framework/tests.h"


void test_boolean()
{
  assert_true(TRUE,L"should work");
  assert_true(123,L"should work");

  invert_next_assert=TRUE;
  assert_true(FALSE,L"should fail");
  invert_next_assert=TRUE;
  assert_true(0,L"should fail");

  assert_false(FALSE,L"should work");
  assert_false(0,L"should work");
  invert_next_assert=TRUE;
  assert_false(TRUE,L"should fail");
  invert_next_assert=TRUE;
  assert_false(12,L"should fail");
}

void test_integer()
{
  assert_intn_equals(1,1,L"should work");
  invert_next_assert=TRUE;
  assert_intn_equals(5,2,L"should fail");

  assert_intn_greater_than_or_equal_to(3,3,L"should work");
  assert_intn_greater_than_or_equal_to(3,4,L"should work");
  invert_next_assert=TRUE;
  assert_intn_greater_than_or_equal_to(4,3,L"should fail");

  assert_intn_less_than_or_equal_to(3,3,L"should work");
  assert_intn_less_than_or_equal_to(4,3,L"should work");
  invert_next_assert=TRUE;
  assert_intn_less_than_or_equal_to(3,4,L"should fail");

  assert_intn_in_closed_interval(-1,3,-1,L"should work");
  assert_intn_in_closed_interval(-1,3, 0,L"should work");
  assert_intn_in_closed_interval(-1,3, 3,L"should work");

  invert_next_assert=TRUE;
  assert_intn_in_closed_interval(-1,3,-2,L"should fail");
  invert_next_assert=TRUE;
  assert_intn_in_closed_interval(-1,3, 4,L"should fail");
}

void test_double()
{
  assert_double_near(10,0.1,10.0999,L"should pass");
  invert_next_assert=TRUE;
  assert_double_near(10,0.1,10.1001,L"should fail");
  assert_double_near(10,0.1,9.9001,L"should pass");
  invert_next_assert=TRUE;
  assert_double_near(10,0.1,9.8999,L"should fail");

  assert_double_greater_than(-20,-19.9999,L"should pass");
  invert_next_assert=TRUE;
  assert_double_greater_than(-20,-20,L"should fail");
  invert_next_assert=TRUE;
  assert_double_greater_than(-20,-20.0001,L"should fail");

  assert_double_greater_than_or_equal_to(-20,-19.9999,L"should pass");
  assert_double_greater_than_or_equal_to(-20,-20,L"should work");
  invert_next_assert=TRUE;
  assert_double_greater_than_or_equal_to(-20,-20.0001,L"should fail");

  assert_double_less_than(0.5,0.4999,L"should pass");
  invert_next_assert=TRUE;
  assert_double_less_than(0.5,0.5,L"should fail");
  invert_next_assert=TRUE;
  assert_double_less_than(0.5,0.5001,L"should fail");


  assert_double_less_than_or_equal_to(0.5,0.4999,L"should pass");
  assert_double_less_than_or_equal_to(0.5,0.5,L"should work");
  invert_next_assert=TRUE;
  assert_double_less_than_or_equal_to(0.5,0.5001,L"should fail");
}

void test_compounds()
{
  assert_not_null(test_compounds,L"should work");
  invert_next_assert=TRUE;
  assert_not_null(NULL,L"should fail");
}

void test_graphics()
{
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL data[]={{127,127,127,0},{150,127,127,0},{127,65,127,0},{127,127,0,0},{127,127,127,127}};

  assert_pixel(data[0],data[0],L"should work");
  invert_next_assert=TRUE;
  assert_pixel(data[0],data[1],L"should fail");
  invert_next_assert=TRUE;
  assert_pixel(data[0],data[2],L"should fail");
  invert_next_assert=TRUE;
  assert_pixel(data[0],data[3],L"should fail");
  invert_next_assert=TRUE;
  assert_pixel(data[0],data[4],L"should fail");

  assert_pixel_near(data[0],0,data[0],L"should work");
  assert_pixel_near(data[0],23,data[1],L"should work");
  invert_next_assert=TRUE;
  assert_pixel_near(data[0],22,data[1],L"should fail");
}


BOOLEAN run_asserts_tests()
{
  INIT_TESTGROUP(L"asserts");
  RUN_TEST(test_boolean,L"boolean assertions");
  RUN_TEST(test_integer,L"integer assertions");
  RUN_TEST(test_double,L"floating point assertions");
  RUN_TEST(test_compounds,L"compound/pointer assertions");
  RUN_TEST(test_graphics,L"graphics assertions");
  FINISH_TESTGROUP();
}
