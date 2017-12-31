/** \file
 * Self-tests for test runner
 * This generates "incomplete" and "failure" results on purpose; the default test runner script will skip this test.
 *
 * \author Richard Nusser
 * \copyright 2017 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 */
#include <Uefi.h>
#include <Library/UefiLib.h>
#include "../../framework/tests.h"


void test_runner_success()
{
  assert_null(NULL,L"should pass");
}

void test_runner_failure()
{
  assert_not_null(NULL,L"should fail");
}

void test_runner_incomplete_success()
{
  test_runner_success();
  mark_test_incomplete();
}

void test_runner_incomplete_failure()
{
  test_runner_failure();
  mark_test_incomplete();
}

void test_runner_empty()
{
  mark_test_incomplete();
}


BOOLEAN run_runner_tests()
{
  INIT_TESTGROUP(L"runner");
  RUN_TEST(test_runner_success,L"runner success");
  RUN_TEST(test_runner_failure,L"runner failure");
  RUN_TEST(test_runner_incomplete_success,L"runner incomplete success");
  RUN_TEST(test_runner_incomplete_failure,L"runner incomplete failure");
  RUN_TEST(test_runner_empty,L"runner without tests");
  FINISH_TESTGROUP();
}
