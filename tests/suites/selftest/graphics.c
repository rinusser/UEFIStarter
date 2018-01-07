/** \file
 * Self-tests for graphics test assertions/utilities
 *
 * \author Richard Nusser
 * \copyright 2017 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_tests_selftest
 */
#include <Uefi.h>
#include <Library/UefiLib.h>
#include "../../framework/tests.h"
#include "../../framework/graphics.h"


/**
 * Makes sure the difference tests and bounding box functions work.
 *
 * \test find_bounding_box_for_changes() finds initial bounding box if there were no changes
 * \test find_bounding_box_for_changes() finds 1x1 bounding box if just one pixel changed
 * \test find_bounding_box_for_changes() finds narrow bounding box if a line of pixel changed
 * \test find_bounding_box_for_changes() finds arbitrary bounding box for given changes
 * \test find_bounding_box_for_changes() finds image-sized bounding box if changes span entire image
 * \test assert_box_equals() compares all struct members
 */
void test_bounding_box()
{
  graphics_difftest_t difftest;
  init_graphics_difftest(&difftest,20,20);

  find_bounding_box_for_changes(&difftest);
  assert_box_equals(&difftest.box,-1,-1,-1,-1,L"no differences");

  difftest.after->data[20*10+7].Red=123;
  find_bounding_box_for_changes(&difftest);
  assert_box_equals(&difftest.box,7,10,7,10,L"1px difference");

  difftest.after->data[20*10+8].Red=123;
  find_bounding_box_for_changes(&difftest);
  assert_box_equals(&difftest.box,7,10,8,10,L"2x1px difference");

  difftest.after->data[0].Red=123;
  find_bounding_box_for_changes(&difftest);
  assert_box_equals(&difftest.box,0,0,8,10,L"9x11px difference");

  difftest.after->data[19*20+19].Red=123;
  find_bounding_box_for_changes(&difftest);
  assert_box_equals(&difftest.box,0,0,19,19,L"full image size difference");

  destroy_graphics_difftest(&difftest);
}


/**
 * Test runner for this group.
 * Gets called via the generated test runner.
 *
 * \return whether the test group was executed
 */
BOOLEAN run_graphics_tests()
{
  INIT_TESTGROUP(L"graphics");
  RUN_TEST(test_bounding_box,L"bounding box");
  FINISH_TESTGROUP();
}
