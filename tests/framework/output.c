/**
 * Presentation logic for tests: generates output with verbosity level in mind
 *
 * \author Richard Nusser
 * \copyright 2017 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \link https://github.com/rinusser/UEFIStarter
 */
#include "output.h"
#include "tests.h"


#if 0
  #define TRACE_STARTTYPE(NAME) ErrorPrint(L"[%s]",NAME);
  #define TRACE_ENDTYPE(NAME)   ErrorPrint(L"[/%s]",NAME);
#else
  #define TRACE_STARTTYPE(NAME)
  #define TRACE_ENDTYPE(NAME)
#endif

extern test_verbosity_t test_verbosity;


static void _print_optional_multiline_test_prefix_or(CHAR16 *or)
{
  if(test_verbosity.multiple_lines_per_test)
    Print(L"\n    ");
  else if(or)
    Print(or);
}


void print_test_group_start(CHAR16 *name)
{
  TRACE_STARTTYPE(L"grp start");
  if(test_verbosity.individual_groups)
  {
    Print(L"running %s tests: ",name);
  }
  TRACE_ENDTYPE(L"grp start");
}

void print_test_group_end()
{
  TRACE_STARTTYPE(L"grp end");
  if(test_verbosity.individual_groups)
    Print(L"\n");
  TRACE_ENDTYPE(L"grp end");
}

void print_assert_counts(INT64 fails, INT64 asserts)
{
  TRACE_STARTTYPE(L"assert cnt");
  if(test_verbosity.assertion_counts)
  {
    if(fails==0 && asserts!=0)
      Print(L": %ld assertion%s passed",asserts,asserts==1?L"":L"s");
    else if(asserts!=0)
      Print(L": %ld out of %ld assertion%s failed",fails,asserts,asserts==1?L"":L"s");
    else
      Print(L": no assertions");
  }
  TRACE_ENDTYPE(L"assert cnt");
}

static void _print_outcome(test_outcome outcome)
{
  if(outcome==SUCCESS)
    color_print(EFI_LIGHTGREEN,L"SUCCESS");
  else if(outcome==INCOMPLETE)
    color_print(EFI_YELLOW,L"INCOMPLETE");
  else
    color_print(EFI_LIGHTRED,L"ERROR");
}

static void _print_1char_outcome(test_outcome outcome)
{
  if(outcome==SUCCESS)
    Print(L".");
  else if(outcome==INCOMPLETE)
    Print(L"I");
  else
    Print(L"E");
}

void print_group_result(test_results_t *results)
{
  TRACE_STARTTYPE(L"grp result");
  if(!test_verbosity.individual_tests && !test_verbosity.one_char_per_test)
  {
    _print_outcome(results->outcome);
    print_assert_counts(results->assert_fails,results->assert_count);
  }
  TRACE_ENDTYPE(L"grp result");
}

void print_individual_result(test_results_t *results)
{
  TRACE_STARTTYPE(L"indiv result");
  if(test_verbosity.one_char_per_test)
    _print_1char_outcome(results->outcome);
  else if(test_verbosity.individual_tests)
  {
    _print_optional_multiline_test_prefix_or(NULL);
    _print_outcome(results->outcome);
    print_assert_counts(results->assert_fails,results->assert_count);
  }
  TRACE_ENDTYPE(L"indiv result");
}

void print_test_result_summary(test_results_t *results)
{
  TRACE_STARTTYPE(L"res summary");
  Print(L"\nResult: ");
  _print_outcome(results->outcome);
  Print(L"\n");

  if(test_verbosity.summary_statistics)
  {
    Print(L"\n");
    Print(L"Successful tests: %u\n",  results->successful_test_count);
    Print(L"Failed tests:     %u\n",  results->failed_test_count);
    Print(L"Incomplete tests: %u\n",  results->incomplete_count);
    Print(L"Skipped groups:   %u\n\n",results->skipped_count);
  }
  TRACE_ENDTYPE(L"res summary");
}

void print_individual_test_start(CHAR16 *description)
{
  TRACE_STARTTYPE(L"indiv start");
  if(test_verbosity.individual_tests && !test_verbosity.one_char_per_test)
  {
    Print(L"\n  testing %s: ",description);
  }
  TRACE_ENDTYPE(L"indiv start");
}

void print_assertion(BOOLEAN success, CHAR16 *description, CHAR16 *message)
{
  TRACE_STARTTYPE(L"assert");
  if(!test_verbosity.one_char_per_test)
  {
    if(success)
    {
      if(test_verbosity.individual_assertions)
      {
        _print_optional_multiline_test_prefix_or(NULL);
        color_print(EFI_GREEN,L"asserted %s (%s)",description,message);
        if(!test_verbosity.multiple_lines_per_test || !test_verbosity.individual_tests)
          Print(L"  ");
      }
    }
    else
    {
      _print_optional_multiline_test_prefix_or(NULL);
      color_print(EFI_RED,L"failed asserting %s (%s)",description,message);
      if(!test_verbosity.multiple_lines_per_test || !test_verbosity.individual_tests)
        Print(L"  ");
    }
  }
  TRACE_ENDTYPE(L"assert");
}


void debug_print_verbosity()
{
  ErrorPrint(L"individual groups=%d tests=%d asserts=%d, 1c/t=%d, assertion_cnt=%d, multiline/t=%d, stats=%d\n",
             test_verbosity.individual_groups,
             test_verbosity.individual_tests,
             test_verbosity.individual_assertions,
             test_verbosity.one_char_per_test,
             test_verbosity.assertion_counts,
             test_verbosity.multiple_lines_per_test,
             test_verbosity.summary_statistics);
}

void debug_print_results(test_results_t *results)
{
  ErrorPrint(L"assert_count=%d, assert_fails=%d, successful_test_count=%d, failed_test_count=%d, incomplete_count=%d, skipped_count=%d, outcome=",
             results->assert_count,
             results->assert_fails,
             results->successful_test_count,
             results->failed_test_count,
             results->incomplete_count,
             results->skipped_count);
  if(results->outcome==SUCCESS)
    ErrorPrint(L"SUCCESS");
  else if(results->outcome==INCOMPLETE)
    ErrorPrint(L"INCOMPLETE");
  else if(results->outcome==FAILURE)
    ErrorPrint(L"FAILURE");
  else
    ErrorPrint(L"unknown (%d)",results->outcome);

  ErrorPrint(L"\n");
}
