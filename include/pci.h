/** \file
 * Functions for accessing PCI devices
 *
 * \author Richard Nusser
 * \copyright 2017 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_lib_pci
 */

#ifndef __PCI_H
#define __PCI_H

#include <Uefi.h>
#include <Protocol/PciIo.h>
#include <IndustryStandard/Pci.h>
#include "logger.h"


/** data type for PCI device subclass names */
typedef struct
{
  UINT8 subclass_code;   /**< the subclass's code */
  UINT16 *subclass_name; /**< the subclass's name, as UTF-16 */
} pci_subclass_name_t;

/** data type for PCI device class names */
typedef struct
{
  UINT8 class_code;                /**< the class's code */
  UINT16 *class_name;              /**< the class's name, as UTF-16 */
  pci_subclass_name_t *subclasses; /**< the class's subclasses */
} pci_class_names_t;


CHAR16 *find_pci_device_name(UINT16 vendor_id, UINT16 device_id, UINT16 subvendor_id, UINT16 subdevice_id);
CHAR16 *find_pci_class_name(UINT8 class_code[3]);
void print_pci_devices();
void print_known_pci_classes();
EFI_PCI_IO_PROTOCOL *find_pci_device(UINT16 vendor_id, UINT16 device_id);

//call before using PCI library
void init_pci_lib();

//call whenever PCI library is used (currently frees device ID file buffer)
void shutdown_pci_lib();

#endif
