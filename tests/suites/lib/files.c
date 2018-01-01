/** \file
 * File I/O tests
 *
 * \author Richard Nusser
 * \copyright 2017 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 */

#include <Uefi.h>
#include <Library/UefiLib.h>
#include "../../../include/files.h"
#include "../../../include/memory.h"
#include "../../framework/tests.h"

void test_get_file_contents()
{
  file_contents_t *contents=get_file_contents(L"\\startup.nsh"); //XXX ideally this would be the currently running .efi - do we have access to args globally?

  if(!assert_not_null(contents,L"error reading file"))
    return;
  if(!assert_intn_greater_than_or_equal_to(9,contents->data_length,L"minimum file length"))
  {
    free_pages(contents,contents->memory_pages);
    return;
  }
  assert_intn_equals('@',contents->data[0],L"first character");
  assert_intn_equals('e',contents->data[1],L"second character");
  free_pages(contents,contents->memory_pages);
}

BOOLEAN run_files_tests()
{
  INIT_TESTGROUP(L"files");
  RUN_TEST(test_get_file_contents,L"get_file_contents");
  FINISH_TESTGROUP();
}
