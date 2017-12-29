/**
 * This is the basis for all test suites.
 * This handles the test execution, the test suite is just responsible for producing results.
 *
 * \author Richard Nusser
 * \copyright 2017 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \link https://github.com/rinusser/UEFIStarter
 */
#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include "../../include/memory.h"
#include "../../include/logger.h"
#include "../../include/cmdline.h"
#include "../../include/string.h"
#include "tests.h"
#include "output.h"

//TODO: add pause-on-error/incomplete feature
//XXX could add a way to capture error messages and output them later


static LOGLEVEL _initial_log_level;

static CHAR16 **_skipped_tests_list;
static UINTN _skipped_tests_count;

test_verbosity_t test_verbosity;

test_results_t individual_test_results;
test_results_t group_test_results;
test_results_t global_test_results;


#define ARG_SKIP_TESTS    tests_args[0].value.wcstr
#define ARG_VERBOSITY     tests_args[1].value.uint64
#define ARG_MULTILINE     tests_args[2].value.uint64
#define ARG_NO_COUNTS     tests_args[3].value.uint64
#define ARG_ASSERTIONS    tests_args[4].value.uint64
#define ARG_NO_STATISTICS tests_args[5].value.uint64


INT_RANGE_VALIDATOR(validate_verbosity,L"verbosity",1,4);


cmdline_argument_t tests_args[]={
  {{wcstr:L""},ARG_STRING,NULL,              L"-skip",         L"Comma-separated test groups to skip"},
  {{uint64:4}, ARG_INT,   validate_verbosity,L"-verbosity",    L"Set verbosity (the higher the more detailed) [1..4]"},
  {{uint64:0}, ARG_BOOL,  NULL,              L"-multiline",    L"Use multiple lines per test (verbosity 4 only)"},
  {{uint64:0}, ARG_BOOL,  NULL,              L"-no-counts",    L"Disable assertion counts (shown at verbosities 2 and 4 only)"},
  {{uint64:0}, ARG_BOOL,  NULL,              L"-assertions",   L"Show successful assertions (verbosity 4 only)"},
  {{uint64:0}, ARG_BOOL,  NULL,              L"-no-statistics",L"Disable summary statistics"},
};

ARG_GROUP(tests_arggroup,tests_args,L"Test options")


void log_errorprint(CHAR16 *level, CHAR16 *msg)
{
  ErrorPrint(L"%s: %s\n",level,msg);
}


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

static test_outcome _combine_outcomes(test_outcome v1, test_outcome v2)
{
  if(v1==FAILURE||v2==FAILURE)
    return FAILURE;
  if(v1==INCOMPLETE||v2==INCOMPLETE)
    return INCOMPLETE;
  return SUCCESS;
}

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

void handle_individual_result()
{
  handle_result(&individual_test_results);
  add_test_results(&group_test_results,&individual_test_results);
  print_individual_result(&individual_test_results);
}

void handle_group_result()
{
  add_test_results(&global_test_results,&group_test_results);
  print_group_result(&group_test_results);
}

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


void run_group(BOOLEAN (*func)())
{
  BOOLEAN ran;
  reset_test_results(&group_test_results);
  ran=func();
  handle_group_result();
  if(ran)
    print_test_group_end();
}

static void _parse_skipped_tests(CHAR16 *str)
{
  if(!str || StrLen(str)<1)
  {
    _skipped_tests_list=NULL;
    return;
  }

  _skipped_tests_count=split_string(&_skipped_tests_list,str,L',');
}

BOOLEAN is_skipped_test(CHAR16 *name)
{
  UINTN tc;
  for(tc=0;tc<_skipped_tests_count;tc++)
    if(StrCmp(name,_skipped_tests_list[tc])==0)
      return TRUE;
  return FALSE;
}

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
