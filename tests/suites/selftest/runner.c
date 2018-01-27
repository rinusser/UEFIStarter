/** \file
 * Self-tests for test runner.
 * This generates "incomplete" and "failure" results on purpose; the default test runner script will skip this test.
 * You'll have to check the test's result yourself, the console should show results matching the descriptions.
 *
 * \author Richard Nusser
 * \copyright 2017-2018 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_tests_selftest
 */
#include <Uefi.h>
#include <Library/UefiLib.h>
#include <UEFIStarter/tests/tests.h>


/**
 * Should produce a SUCCESS result in the test runner.
 *
 * \test making a successful assertion results in SUCCESS for the test
 */
void test_runner_success()
{
  assert_null(NULL,L"should pass");
}

/**
 * Should produce an ERROR result in the test runner.
 *
 * \test making a failing assertion results in FAILURE for the test
 */
void test_runner_failure()
{
  assert_not_null(NULL,L"should fail");
}

/**
 * Should produce an INCOMPLETE but successful result in the test runner.
 *
 * \test marking a test incomplete when there are successful assertions results in INCOMPLETE but success for the test
 */
void test_runner_incomplete_success()
{
  assert_null(NULL,L"should pass");
  mark_test_incomplete();
}

/**
 * Should produce an INCOMPLETE but unsucessful result in the test runner.
 *
 * \test marking a test incomplete when there are failed assertions results in INCOMPLETE but failure for the test
 */
void test_runner_incomplete_failure()
{
  assert_not_null(NULL,L"should fail");
  mark_test_incomplete();
}

/**
 * Should produce an INCOMPLETE (no assertions) result in the test runner.
 *
 * \test tests without any assertions are marked as incomplete
 */
void test_runner_empty()
{
}


/**
 * Test runner for this group.
 * Gets called via the generated test runner.
 *
 * \return whether the test group was executed
 */
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
