/** \file
 * Tests for the command line parameter parser.
 *
 * \author Richard Nusser
 * \copyright 2017-2018 Richard Nusser
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
BOOLEAN validate_int(cmdline_value_t val)
{
  return !(val.uint64%2);
}

/**
 * A list of arguments, used in tests.
 */
cmdline_argument_t cmdline_args_list[]=
{
  {{uint64:0},    ARG_BOOL,  NULL,        L"-bool",  L"some boolean"},
  {{uint64:12},   ARG_INT,   validate_int,L"-int",   L"some integer"},
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
  {L"empty list",       TRUE, 0,12,  2.5,    L"foo",NULL},
  {L"disable logging",  TRUE, 0,12,  2.5,    L"foo",L"-no-log "},         //keep whitespace: compiler breaks test otherwise
  {L"negative uint64 1",FALSE,0,12,  2.5,    L"foo",L"-int -1 -no-log"},
  {L"negative uint64 2",FALSE,0,12,  2.5,    L"foo",L"-int -2  -no-log"}, //keep whitespace: compiler breaks test otherwise
  {L"failing validator",FALSE,0,23,  2.5,    L"foo",L"-int 23"},
  {L"passing all",      TRUE, 1,22,-12.98766,L"bar",L"-double -12.98766 -int 22 -string bar -bool"},
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
  LOGLEVEL previous_log_level;

  argc=split_string(&argv,testcase->input,L' ');
  previous_log_level=get_log_level();

  VA_START(args,count);
  success=parse_parameters(argc,argv,count,args)==EFI_SUCCESS;
  VA_END(args);

  set_log_level(previous_log_level);

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
 * \test passing negative numbers to unsigned integer parameters results in parse failure
 * \test can supply logging parameters on command-line
 */
void test_parse_parameters()
{
  UINTN count=sizeof(cmdline_args_testcases)/sizeof(cmdline_args_testcase_t);
  UINTN tc;
  cmdline_args_testcase_t *cases=cmdline_args_testcases;

  for(tc=0;tc<count;tc++)
    do_parse_parameters_testcase(cases+tc,cmdline_args_list,1,&cmdline_args_group);
}


/** data type for command-line argument validator testcases */
typedef struct
{
  CHAR16 *message;         /**< a descriptive message for the testcase */
  BOOLEAN expected_result; /**< whether the validator is expected to accept the value */
  cmdline_value_t input;   /**< the input value */
  cmdline_value_t min;     /**< the lowest allowed value */
  cmdline_value_t max;     /**< the highest allowed value */
} validate_range_testcase_t;

/** double test cases for test_validate_ranges */
validate_range_testcase_t double_range_testcases[]=
{
  //msg       expct  input        min          max
  {L"below",  FALSE,{dbl:-10.0}, {dbl:-5.0},  {dbl:5.0}},
  {L"min",    TRUE, {dbl:-5.0},  {dbl:-5.0},  {dbl:5.0}},
  {L"between",TRUE, {dbl:-123.0},{dbl:-124.0},{dbl:-122.0}},
  {L"max",    TRUE, {dbl:5.0},   {dbl:-5.0},  {dbl:5.0}},
  {L"above",  FALSE,{dbl:1.0},   {dbl:1.2},   {dbl:1.3}},
  {L"at",     TRUE, {dbl:12.3},  {dbl:12.3},  {dbl:12.3}},
};

/** UINT64 test cases for test_validate_ranges */
validate_range_testcase_t uint64_range_testcases[]=
{
  //msg       expct  input       min         max
  {L"below",  FALSE,{uint64:1}, {uint64:5}, {uint64:10}},
  {L"min",    TRUE, {uint64:2}, {uint64:2}, {uint64:6}},
  {L"between",TRUE, {uint64:50},{uint64:10},{uint64:100}},
  {L"max",    TRUE, {uint64:15},{uint64:5}, {uint64:15}},
  {L"above",  FALSE,{uint64:3}, {uint64:1}, {uint64:2}},
  {L"at",     TRUE, {uint64:12},{uint64:12},{uint64:12}},
};


/**
 * Makes sure the double and uint64 range validators work
 *
 * \test validate_double_range() and validate_uint64_range allow values only if they're in the closed interval between minimum and maximum
 * \test validate_double_range() and validate_uint64_range still work if minimum=maximum
 */
void test_validate_ranges()
{
  UINTN count=sizeof(double_range_testcases)/sizeof(validate_range_testcase_t);
  UINTN tc;
  LOGLEVEL previous_log_level=get_log_level();
  validate_range_testcase_t *cases=double_range_testcases;

  set_log_level(OFF);

  for(tc=0;tc<count;tc++)
    assert_intn_equals(cases[tc].expected_result,validate_double_range(cases[tc].input,L"",cases[tc].min.dbl,cases[tc].max.dbl),cases[tc].message);

  count=sizeof(uint64_range_testcases)/sizeof(validate_range_testcase_t);
  cases=uint64_range_testcases;
  for(tc=0;tc<count;tc++)
    assert_intn_equals(cases[tc].expected_result,validate_uint64_range(cases[tc].input,L"",cases[tc].min.uint64,cases[tc].max.uint64),cases[tc].message);

  set_log_level(previous_log_level);
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
  RUN_TEST(test_validate_ranges,L"validating value ranges");
  FINISH_TESTGROUP();
}
