/** \file
 * This is the basis for all test suites.
 * This handles the test execution, the test suite is just responsible for producing results.
 *
 * \author Richard Nusser
 * \copyright 2017-2018 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_tests_runner
 *
 * \TODO add pause-on-error/incomplete feature
 * \XXX could add a way to capture error messages and output them later
 */

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <UEFIStarter/core/memory.h>
#include <UEFIStarter/core/logger.h>
#include <UEFIStarter/core/cmdline.h>
#include <UEFIStarter/core/string.h>
#include <UEFIStarter/tests/tests.h>
#include <UEFIStarter/tests/output.h>


/** internal storage for the initial log level to set before each test */
static LOGLEVEL _initial_log_level;

static CHAR16 **_skipped_tests_list; /**< internal storage for the list of skipped tests */
static UINTN _skipped_tests_count;   /**< internal storage for the number of skipped tests */

/** the current test verbosity */
test_verbosity_t test_verbosity;

test_results_t individual_test_results; /**< results for the current test */
test_results_t group_test_results;      /**< results for the current test group */
test_results_t global_test_results;     /**< results for all tests in this suite */


#define ARG_SKIP_TESTS    tests_args[0].value.wcstr  /**< helper macro to access the -skip argument's values */
#define ARG_VERBOSITY     tests_args[1].value.uint64 /**< helper macro to access the -verbosity argument's value */
#define ARG_MULTILINE     tests_args[2].value.uint64 /**< helper macro to access the -multiline argument's value */
#define ARG_NO_COUNTS     tests_args[3].value.uint64 /**< helper macro to access the -no-counts argument's value */
#define ARG_ASSERTIONS    tests_args[4].value.uint64 /**< helper macro to access the -assertions argument's value */
#define ARG_NO_STATISTICS tests_args[5].value.uint64 /**< helper macro to access the -no-statistics argument's value */


/**
 * Validator for the `-verbosity <level>` argument.
 *
 * \param v the input to validate
 * \return whether the given value is a valid verbosity
 */
INT_RANGE_VALIDATOR(validate_verbosity,L"verbosity",1,4);


/** list of command-line arguments */
cmdline_argument_t tests_args[]={
  {{wcstr:L""},ARG_STRING,NULL,              L"-skip",         L"Comma-separated test groups to skip"},
  {{uint64:4}, ARG_INT,   validate_verbosity,L"-verbosity",    L"Set verbosity (the higher the more detailed) [1..4]"},
  {{uint64:0}, ARG_BOOL,  NULL,              L"-multiline",    L"Use multiple lines per test (verbosity 4 only)"},
  {{uint64:0}, ARG_BOOL,  NULL,              L"-no-counts",    L"Disable assertion counts (shown at verbosities 2 and 4 only)"},
  {{uint64:0}, ARG_BOOL,  NULL,              L"-assertions",   L"Show successful assertions (verbosity 4 only)"},
  {{uint64:0}, ARG_BOOL,  NULL,              L"-no-statistics",L"Disable summary statistics"},
};

/** command-line argument group */
ARG_GROUP(tests_arggroup,tests_args,L"Test options")


/**
 * Log printer function writing to STDERR.
 *
 * \param level the log entry's log level
 * \param msg   the log entry's message
 */
void log_errorprint(CHAR16 *level, CHAR16 *msg)
{
  ErrorPrint(L"%s: %s\n",level,msg);
}


/**
 * Initializes a test_results_t structure.
 *
 * \param results the structure to reset
 */
void reset_test_results(test_results_t *results)
{
  results->assert_count=0;
  results->assert_fails=0;
  results->successful_test_count=0;
  results->failed_test_count=0;
  results->incomplete_count=0;
  results->skipped_count=0;
  results->outcome=SUCCESS;
}

/**
 * Internal: combines two test outcomes.
 *
 * \param v1 the first value to compare
 * \param v2 the second value to compare
 * \return the combined outcome
 */
static test_outcome _combine_outcomes(test_outcome v1, test_outcome v2)
{
  if(v1==FAILURE||v2==FAILURE)
    return FAILURE;
  if(v1==INCOMPLETE||v2==INCOMPLETE)
    return INCOMPLETE;
  return SUCCESS;
}

/**
 * Adds test result data to a larger collection of results.
 * This is e.g. used to sum up individual test results into test group results.
 *
 * \param target the larger collection of results to add to
 * \param source the new data to add
 */
void add_test_results(test_results_t *target, test_results_t *source)
{
  target->assert_count+=source->assert_count;
  target->assert_fails+=source->assert_fails;
  target->successful_test_count+=source->successful_test_count;
  target->failed_test_count+=source->failed_test_count;
  target->incomplete_count+=source->incomplete_count;
  target->skipped_count+=source->skipped_count;
  target->outcome=_combine_outcomes(target->outcome,source->outcome);
}

/**
 * Updates internal test_results_t data after a test.
 *
 * \param result the data to update
 */
void handle_result(test_results_t *result)
{
  if(result->incomplete_count>0 || (result->assert_fails==0&&result->assert_count==0))
  {
    result->incomplete_count=1;
    result->outcome=INCOMPLETE;
  }
  else if(result->assert_fails==0)
  {
    result->successful_test_count++;
    result->outcome=SUCCESS;
  }
  else
  {
    result->failed_test_count++;
    result->outcome=FAILURE;
  }
}

/**
 * Processes an individual test's results.
 */
void handle_individual_result()
{
  handle_result(&individual_test_results);
  add_test_results(&group_test_results,&individual_test_results);
  print_individual_result(&individual_test_results);
}

/**
 * Processes a test group's results.
 */
void handle_group_result()
{
  add_test_results(&global_test_results,&group_test_results);
  print_group_result(&group_test_results);
}

/**
 * Runs an individual test.
 * This function takes care of required setup/teardown around tests.
 *
 * \param func        the test function to execute
 * \param description the test's description
 */
void run_test(void (*func)(), CHAR16 *description)
{
  set_logger_function(log_errorprint);
  set_log_level(_initial_log_level);

  reset_test_results(&individual_test_results);
  print_individual_test_start(description);

  func();

  set_logger_function(log_errorprint);
  set_log_level(_initial_log_level);

  handle_individual_result();

  stop_tracking_memory();
}


/**
 * Runs a test group.
 *
 * \param func the test group to run
 */
void run_group(BOOLEAN (*func)())
{
  BOOLEAN ran;
  reset_test_results(&group_test_results);
  ran=func();
  handle_group_result();
  if(ran)
    print_test_group_end();
}

/**
 * Internal: parses a list of skipped tests into internal storage.
 *
 * \param str the list of tests to skip
 */
static void _parse_skipped_tests(CHAR16 *str)
{
  if(!str || StrLen(str)<1)
  {
    _skipped_tests_list=NULL;
    return;
  }

  _skipped_tests_count=split_string(&_skipped_tests_list,str,L',');
}

/**
 * Tests whether a given test should be skipped.
 *
 * \param name the test's name
 * \return whether the test should be skipped
 */
BOOLEAN is_skipped_test(CHAR16 *name)
{
  UINTN tc;
  for(tc=0;tc<_skipped_tests_count;tc++)
    if(StrCmp(name,_skipped_tests_list[tc])==0)
      return TRUE;
  return FALSE;
}

/**
 * Transforms the -verbosity command-line argument into its internal representation.
 */
void assemble_and_set_verbosity()
{
  UINTN vmask=0;
  UINTN vmask_by_verbosity[]={4,1,5,3};

  if(ARG_VERBOSITY>=1 && ARG_VERBOSITY<=sizeof(vmask_by_verbosity)/sizeof(UINTN))
    vmask|=vmask_by_verbosity[ARG_VERBOSITY-1];
  else
    LOG.error(L"unhandled verbosity: %d",ARG_VERBOSITY);
  *((UINTN *)&test_verbosity)=vmask;

  test_verbosity.multiple_lines_per_test=ARG_VERBOSITY>=4&&ARG_MULTILINE;
  test_verbosity.assertion_counts=(ARG_VERBOSITY==2||ARG_VERBOSITY>=4)&&!ARG_NO_COUNTS;
  test_verbosity.individual_assertions=ARG_VERBOSITY>=4&&ARG_ASSERTIONS;
  test_verbosity.summary_statistics=!ARG_NO_STATISTICS;
}

/**
 * Main function for test suite, gets invoked by UEFI shell.
 *
 * \param argc       the number of command-line arguments passed
 * \param argv_ascii the command-line arguments passed, as ASCII
 * \return an EFI status code for the shell
 */
int main(int argc, char **argv_ascii)
{
  EFI_STATUS rv;
  CHAR16 **argv;

  argv=argv_from_ascii(argc,argv_ascii);
  rv=init(argc,argv,1,&tests_arggroup);
  free_argv();

  if(rv==EFI_SUCCESS)
  {
    assemble_and_set_verbosity();
    reset_test_results(&global_test_results);
    _initial_log_level=get_log_level();
    _parse_skipped_tests(ARG_SKIP_TESTS);

    //init_timestamps();
    run_tests();

    print_test_result_summary(&global_test_results);
    if(_skipped_tests_list)
      FreePool(_skipped_tests_list);
  }

  shutdown();
  return rv;
}
