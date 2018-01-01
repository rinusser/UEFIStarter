/** \file
 * Tests for the command line parameter parser.
 *
 * \author Richard Nusser
 * \copyright 2017 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_lib_cmdline
 */

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include "../../../include/logger.h"
#include "../../../include/string.h"
#include "../../framework/tests.h"


/**
 * A validator function for integers, used in tests.
 *
 * \param val the value to validate
 * \return whether the value is valid
 */
BOOLEAN validate_int(double_uint64_t val)
{
  return val.uint64%2;
}

/**
 * A list of arguments, used in tests.
 */
cmdline_argument_t cmdline_args_list[]=
{
  {{uint64:0},    ARG_BOOL,  NULL,        L"-bool",  L"some boolean"},
  {{uint64:11},   ARG_INT,   validate_int,L"-int",   L"some integer"},
  {{dbl:2.5},     ARG_DOUBLE,NULL,        L"-double",L"some double"},
  {{wcstr:L"foo"},ARG_STRING,NULL,        L"-string",L"some string"},
};

/**
 * An argument group, used in tests.
 */
cmdline_argument_group_t cmdline_args_group=
{
  NULL,
  sizeof(cmdline_args_list)/sizeof(cmdline_argument_t),
  cmdline_args_list
};

/** data type for command-line argument parser testcase */
typedef struct
{
  CHAR16 *message;          /**< a descriptive message for the testcase */
  BOOLEAN expected_success; /**< whether the parser is expected to indicate parsed values were valid */
  BOOLEAN expected_bool;    /**< the boolean argument's expected value */
  INTN expected_int;        /**< the integer argument's expected value */
  double expected_double;   /**< the double argument's expected value */
  CHAR16 *expected_string;  /**< the string argument's expected value */
  CHAR16 *input;            /**< the command-line argument string to pass to the parser */
} cmdline_args_testcase_t;

/** testcases for test_parse_parameters() */
cmdline_args_testcase_t cmdline_args_testcases[]=
{
  {L"empty list",       TRUE, 0,11,  2.5,    L"foo",NULL},
  {L"failing validator",FALSE,0,22,  2.5,    L"foo",L"-int 22"},
  {L"passing all",      TRUE, 1,23,-12.98766,L"bar",L"-double -12.98766 -int 23 -string bar -bool"},
};

/**
 * Runs an individual testcase.
 * This currently doesn't really support multiple argument groups, the vararg setup is just there to convert the
 * argument group to VA_LIST for parse_parameters().
 *
 * \param testcase the testcase to run
 * \param list     the parsed argument's data
 * \param count    the number of argument groups passed
 * \param ...      the list of argument groups (as cmdline_argument_group_t *)
 */
void EFIAPI do_parse_parameters_testcase(cmdline_args_testcase_t *testcase, cmdline_argument_t *list, UINTN count, ...)
{
  BOOLEAN success;
  VA_LIST args;
  UINTN argc;
  CHAR16 **argv;

  argc=split_string(&argv,testcase->input,L' ');
  VA_START(args,count);
  success=parse_parameters(argc,argv,count,args)==EFI_SUCCESS;
  VA_END(args);

  assert_intn_equals(testcase->expected_success,success,L"success");
  assert_intn_equals(testcase->expected_bool,list[0].value.uint64,L"bool");
  assert_intn_equals(testcase->expected_int, list[1].value.uint64,L"int");
  assert_double_near(testcase->expected_double,0.0000001,list[2].value.dbl,L"double");
  assert_wcstr_equals(testcase->expected_string,list[3].value.wcstr,L"string");

  FreePool(argv);
}

/**
 * Makes sure the command-line parser works.
 *
 * \test not passing any parameters is OK
 * \test passing a parameter value a validator deems invalid results in parse failure
 * \test passing all valid values results in parse success
 */
void test_parse_parameters()
{
  UINTN count=sizeof(cmdline_args_testcases)/sizeof(cmdline_args_testcase_t);
  UINTN tc;
  cmdline_args_testcase_t *cases=cmdline_args_testcases;

  for(tc=0;tc<count;tc++)
    do_parse_parameters_testcase(cases+tc,cmdline_args_list,1,&cmdline_args_group);
}


/**
 * Test runner for this group.
 * Gets called via the generated test runner.
 *
 * \return whether the test group was executed
 */
BOOLEAN run_cmdline_tests()
{
  INIT_TESTGROUP(L"command line");
  RUN_TEST(test_parse_parameters,L"parsing parameters");
  FINISH_TESTGROUP();
}
