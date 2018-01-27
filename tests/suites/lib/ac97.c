/** \file
 * Tests for AC'97 functions.
 *
 * \author Richard Nusser
 * \copyright 2017-2018 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_lib_ac97
 */

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <UEFIStarter/ac97.h>
#include <UEFIStarter/tests/tests.h>


/**
 * Shortcut macro: checks whether symbol has given size.
 *
 * \param NAME the symbol's name
 * \param SIZE the expected size, in bytes
 */
#define STRUCT_SIZE_TEST(NAME,SIZE) assert_intn_equals(SIZE,sizeof(NAME),L"sizeof " #NAME);

/**
 * Makes sure AC'97 built-in structures are sized correctly.
 *
 * \test ac97_bar_t's size is exactly 128 bytes, as required by AC'97 specs
 * \test ac97_buffer_descriptor_t's size is exactly 8 bytes, as required by AC'97 specs
 * \test ac97_busmaster_status_t's size is exactly 2 bytes, as required by AC'97 specs
 */
void test_struct_sizes()
{
  STRUCT_SIZE_TEST(ac97_bar_t,128);
  STRUCT_SIZE_TEST(ac97_buffer_descriptor_t,8);
  STRUCT_SIZE_TEST(ac97_busmaster_status_t,2);
}


/** data type for test_volume_macro() test cases */
typedef struct
{
  UINT16 expected; /**< the expected mixer value */
  UINT8 left;      /**< the left channel's volume to set */
  UINT8 right;     /**< the right channel's volume to set */
  UINT8 mute;      /**< the mute value to set */
} volume_macro_testcase_t;

/** test cases for test_volume_macro() */
volume_macro_testcase_t volume_macro_testcases[]={
  {0x8000,0,0,1},
  {0x0000,0,0,0},
  {0x0808,8,8,0},
  {0x3F3F,0x3F,0x3F,0},
  {0x9F12,0x1F,0x12,7}
};

/**
 * Makes sure ac97_mixer_value() works.
 *
 * \test ac97_mixer_value() handles left and right channel separately
 * \test ac97_mixer_value() converts mute value to 1 bit
 */
void test_volume_macro()
{
  INTN tc, count;
  volume_macro_testcase_t *cases=volume_macro_testcases;
  count=sizeof(volume_macro_testcases)/sizeof(volume_macro_testcase_t);

  for(tc=0;tc<count;tc++)
    assert_intn_equals(cases[tc].expected,ac97_mixer_value(cases[tc].left,cases[tc].right,cases[tc].mute),L"volume");
}


/**
 * Test runner for this group.
 * Gets called via the generated test runner.
 *
 * \return whether the test group was executed
 */
BOOLEAN run_ac97_tests()
{
  INIT_TESTGROUP(L"AC97");
  RUN_TEST(test_struct_sizes,L"struct sizes");
  RUN_TEST(test_volume_macro,L"volume register macro");
  FINISH_TESTGROUP();
}
