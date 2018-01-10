/** \file
 * Tests for the PCI library functions.
 *
 * \author Richard Nusser
 * \copyright 2017-2018 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_lib_pci
 */

#include <Uefi.h>
#include <Library/UefiLib.h>
#include "../../../include/pci.h"
#include "../../framework/tests.h"


/** data type for test_find_pci_device_name() testcases */
typedef struct
{
  UINT16 vendor_id;      /**< the input vendor ID */
  UINT16 device_id;      /**< the input device ID */
  CHAR16 *expected_name; /**< the expected PCI device name */
} pci_device_name_testcase_t;

/** testcases for test_find_pci_device_name() */
pci_device_name_testcase_t pci_device_name_testcases[]=
{
  {0x106b,0x003f,L"Apple Inc., KeyLargo/Intrepid USB"}, //first entry in shortened pci.ids, there was a strstr() bug affecting this
  {0x8086,0x2415,L"Intel Corporation, 82801AA AC'97 Audio Controller"},
  {0x0000,0x0000,L"(unknown)"},
  {0x8086,0x0000,L"Intel Corporation, unknown device"}
};

/**
 * Makes sure determining PCI device names works.
 *
 * \test find_pci_device_name() finds entries with known vendor and device IDs
 * \test find_pci_device_name() finds known vendor entries and marks unknown device IDs
 * \test find_pci_device_name() marks unknown vendor IDs
 * \test find_pci_device_name() isn't affected by old strstr() bug
 */
void test_find_pci_device_name()
{
  UINTN count=sizeof(pci_device_name_testcases)/sizeof(pci_device_name_testcase_t);
  UINTN tc;
  pci_device_name_testcase_t *cases=pci_device_name_testcases;

  init_pci_lib();

  for(tc=0;tc<count;tc++)
    assert_wcstr_equals(cases[tc].expected_name,find_pci_device_name(cases[tc].vendor_id,cases[tc].device_id,0,0),L"device name");

  shutdown_pci_lib();
}


/** data type for test_find_pci_class_name() testcases */
typedef struct
{
  UINT8 class;            /**< the input class code */
  UINT8 subclass;         /**< the input subclass code */
  CHAR16 *expected_class; /**< the expected PCI class name, as UTF-16 */
} pci_class_name_testcase_t;

/** testcases for test_find_pci_class_name() */
pci_class_name_testcase_t pci_class_name_testcases[]=
{
  {1, 6, L"Mass Storage Controller, SATA Controller"},
  {4, 1, L"Multimedia, Audio Device"},
  {4, 99,L"Multimedia, unknown"},
  {99,0, L"unknown, unknown"},
};

/**
 * Makes sure PCI class/subclass codes are mapped to names properly.
 *
 * \test find_pci_class_name() finds entries with known class and subclass codes
 * \test find_pci_class_name() finds entries with known class and marks unknown subclasses
 * \test find_pci_class_name() marks unknown entries as such
 */
void test_find_pci_class_name()
{
  UINTN tc;
  UINTN count=sizeof(pci_class_name_testcases)/sizeof(pci_class_name_testcase_t);
  UINT8 val[3];
  pci_class_name_testcase_t *cases=pci_class_name_testcases;

  val[0]=0; //this is the device's "function id" iirc - currently ignored by pci lib anyways
  for(tc=0;tc<count;tc++)
  {
    val[1]=cases[tc].subclass;
    val[2]=cases[tc].class;
    assert_wcstr_equals(cases[tc].expected_class,find_pci_class_name(val),L"class name");
  }
}


/**
 * Test runner for this group.
 * Gets called via the generated test runner.
 *
 * \return whether the test group was executed
 */
BOOLEAN run_pci_tests()
{
  INIT_TESTGROUP(L"PCI");
  RUN_TEST(test_find_pci_device_name,L"find PCI device name");
  RUN_TEST(test_find_pci_class_name,L"find PCI class name");
  FINISH_TESTGROUP();
}
