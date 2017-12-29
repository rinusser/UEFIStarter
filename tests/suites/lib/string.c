/**
 * String functions tests
 *
 * \author Richard Nusser
 * \copyright 2017 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \link https://github.com/rinusser/UEFIStarter
 */
#include <Uefi.h>
#include <Library/UefiLib.h>
#include "../../../include/string.h"
#include "../../../include/logger.h"
#include "../../../include/memory.h"
#include "../../../include/cmdline.h"
#include "../../framework/tests.h"


typedef struct
{
	BOOLEAN expected;
	CHAR16 *input;
} wctype_testcase_t;

wctype_testcase_t wctype_floatint_testcases[]={
  {FALSE,NULL},
  {FALSE,L""},
  {FALSE,L"-a"},
  {FALSE,L"-"},
  {FALSE,L"1a"},
  {FALSE,L"-1a"},
  {TRUE, L"1"},
  {TRUE, L"1123"},
  {TRUE, L"-1123"},
  {FALSE,L"-1123a"},
  {FALSE,L"abcd12"}
};

wctype_testcase_t wctype_int_testcases[]={
  {FALSE,L"1.0"},
};

wctype_testcase_t wctype_float_testcases[]={
  {TRUE, L"1.1"},
  {TRUE, L"-1.1"},
  {TRUE, L"0.123456789"},
  {TRUE, L"-99999999.9"},
  {FALSE,L"1.1.1"},
  {FALSE,L"1."}
};

void _run_wctype_testcases(UINTN count, wctype_testcase_t *cases, BOOLEAN (*func)(CHAR16 *))
{
  UINTN tc;
  for(tc=0;tc<count;tc++)
    assert_intn_equals(cases[tc].expected,func(cases[tc].input),L"wctype");
}

#define RUN_WCTYPE_TESTCASES(FUNC,CASES) _run_wctype_testcases(sizeof(CASES)/sizeof(wctype_testcase_t),CASES,FUNC);

void test_wctype_int()
{
  RUN_WCTYPE_TESTCASES(wctype_int,wctype_floatint_testcases);
  RUN_WCTYPE_TESTCASES(wctype_int,wctype_int_testcases);
}

void test_wctype_float()
{
  RUN_WCTYPE_TESTCASES(wctype_float,wctype_floatint_testcases);
  RUN_WCTYPE_TESTCASES(wctype_float,wctype_float_testcases);
}


typedef struct
{
  CHAR16 *expectation;
  double input;
} ftowcs_testcase_t;

ftowcs_testcase_t ftowcs_testcases[]=
{
  {L"-1000000000.00",-1000000000.00},
  {L"-999999999.99",-999999999.99},
  {L"-123456789.01",-123456789.012},
  {L"-10.00",-10.001},
  {L"-10.00",-10.000},
  {L"-10.00",-9.999},
  {L"-10.00",-9.995001},
  {L"-9.99",-9.9949},
  {L"-9.00",-9.0},
  {L"-0.51",-0.51},
  {L"-0.50",-0.5},
  {L"-0.49",-0.49},
  {L"-0.00",-0.001},
  {L"0.00",0.0},
  {L"0.00",0.001},
  {L"0.45",0.449},
  {L"0.45",0.45},
  {L"10.00",9.999},
  {L"10.00",10.00},
  {L"99999.99",99999.99},
  {L"100000.00",100000.00},
  {L"999999999.99",999999999.99},
  {L"1000000000.00",1000000000.00},
};

void test_ftowcs()
{
  UINTN tc;
  UINTN count=sizeof(ftowcs_testcases)/sizeof(ftowcs_testcase_t);
  ftowcs_testcase_t *cases=ftowcs_testcases;

  for(tc=0;tc<count;tc++)
    assert_wcstr_equals(cases[tc].expectation,ftowcs(cases[tc].input),L"wcstr");
}

void test_ftowcs_boundaries()
{
  LOGLEVEL previous_log_level;
  INTN prev_error_count, log_error_count;

  previous_log_level=get_log_level();
  set_log_level(OFF);
  prev_error_count=get_logger_entry_count(ERROR);
  ftowcs(-1E100);
  log_error_count=get_logger_entry_count(ERROR)-prev_error_count;
  assert_intn_equals(1,log_error_count,L"expected 1 conversion error");

  prev_error_count=get_logger_entry_count(ERROR);
  ftowcs(1E100);
  log_error_count=get_logger_entry_count(ERROR)-prev_error_count;
  set_log_level(previous_log_level);
  assert_intn_equals(1,log_error_count,L"expected 1 conversion error");
}


typedef struct {
  double expectation;
  CHAR16 *input;
} wcstof_testcase_t;

wcstof_testcase_t wcstof_testcases[]=
{
  {1.2,L"1.2"},
  {-3.1,L"-3.1"},
  {4321.987,L"4321.987"},
  {654321.654,L"654321.654"},
  {87654321.321,L"87654321.321"},
  {123456789.0123,L"123456789.0123"}
};

#define WCSTOF_EPSILON 0.0000001

void test_wcstof()
{
  UINTN tc;
  UINTN count=sizeof(wcstof_testcases)/sizeof(wcstof_testcase_t);
  wcstof_testcase_t *cases=wcstof_testcases;

  for(tc=0;tc<count;tc++)
    assert_double_near(cases[tc].expectation,WCSTOF_EPSILON,_wcstof(cases[tc].input),L"wcstof");
}


typedef struct
{
  UINT64 expectation;
  char *input;
} atoui64_testcase_t;

atoui64_testcase_t atoui64_testcases[]=
{
  {0,"0"},
  {12,"12"},
  {4294967295,"4294967295"}, //2^32-1
  {4294967296,"4294967296"}, //2^32
  {4294967297,"4294967297"}, //2^32+1
  {9223372036854775807,"9223372036854775807"}, //2^63-1
  {9223372036854775808u,"9223372036854775808"}, //2^63
  {9223372036854775809u,"9223372036854775809"}, //2^63+1
  {18446744073709551614u,"18446744073709551614"}, //2^64-2
  {18446744073709551615u,"18446744073709551615"}, //2^64-1
};

void test_atoui64()
{
  UINTN tc;
  UINTN count=sizeof(atoui64_testcases)/sizeof(atoui64_testcase_t);
  atoui64_testcase_t *cases=atoui64_testcases;

  for(tc=0;tc<count;tc++)
    assert_uint64_equals(cases[tc].expectation,atoui64(cases[tc].input),L"uint64");
}


typedef struct
{
  EFI_STATUS code;
  CHAR16 *function_name;
  CHAR16 *expected_message;
} sprint_status_testcase_t;

sprint_status_testcase_t sprint_status_testcases[]=
{
  {EFI_UNSUPPORTED,      L"case1",L"case1() returned status 3 (Unsupported)"},
  {EFI_INVALID_PARAMETER,L"case2",L"case2() returned status 2 (Invalid Parameter)"},
};

void test_sprint_status()
{
  UINTN tc;
  UINTN count=sizeof(sprint_status_testcases)/sizeof(sprint_status_testcase_t);
  sprint_status_testcase_t *cases=sprint_status_testcases;

  for(tc=0;tc<count;tc++)
    assert_wcstr_equals(cases[tc].expected_message,sprint_status(cases[tc].function_name,cases[tc].code),L"status message");
}


void test_memsprintf()
{
  free_pool_memory_entries();
  assert_intn_equals(0,free_pool_memory_entries(),L"control test");

  memsprintf(L"1st: plain call");
  memsprintf(L"%dst: %s %d %a",1,L"passing",4,"arguments");
  assert_intn_equals(2,free_pool_memory_entries(),L"2 calls should result in 2 pool entries");

  memsprintf(L"2nd");
  assert_intn_equals(1,free_pool_memory_entries(),L"1 call should result in 1 pool entry");
}


void test_split_string()
{
  CHAR16 **list;
  CHAR16 *input=L"this|is|a|list";
  CHAR16 *emptyinput=L"~~";

  assert_intn_equals(0,split_string(&list,NULL,L'|'),L"NULL input");
  assert_null(list,L"NULL input");

  if(assert_intn_equals(4,split_string(&list,input,L'|'),L"4 strings") && assert_not_null(list,L"4 strings"))
  {
    assert_wcstr_equals(L"this",list[0],L"4 strings, 1st part");
    assert_wcstr_equals(L"is",  list[1],L"4 strings, 2nd part");
    assert_wcstr_equals(L"a",   list[2],L"4 strings, 3rd part");
    assert_wcstr_equals(L"list",list[3],L"4 strings, 4th part");
  }

  if(assert_intn_equals(3,split_string(&list,emptyinput,L'~'),L"3 empty strings") && assert_not_null(list,L"3 empty strings"))
  {
    assert_wcstr_equals(L"",list[0],L"3 empty strings, 1st part");
    assert_wcstr_equals(L"",list[1],L"3 empty strings, 2nd part");
    assert_wcstr_equals(L"",list[2],L"3 empty strings, 3rd part");
  }

  if(assert_intn_equals(1,split_string(&list,L"",L'|'),L"no input") && assert_not_null(list,L"no input"))
  {
    assert_wcstr_equals(L"",list[0],L"no input");
  }
}


BOOLEAN run_string_tests()
{
  INIT_TESTGROUP(L"string");
  RUN_TEST(test_wctype_int,L"wctype_int");
  RUN_TEST(test_wctype_float,L"wctype_float");
  RUN_TEST(test_ftowcs,L"ftowcs conversions");
  RUN_TEST(test_ftowcs_boundaries,L"ftowcs boundaries");
  RUN_TEST(test_wcstof,L"wcstof");
  RUN_TEST(test_atoui64,L"atoui64");
  RUN_TEST(test_sprint_status,L"sprint_status");
  RUN_TEST(test_memsprintf,L"memsprintf");
  RUN_TEST(test_split_string,L"split_string");
  FINISH_TESTGROUP();
}
