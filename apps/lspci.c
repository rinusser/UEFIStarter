/** \file
 * This application lists PCI devices.
 * The built-in static/pci.ids contains just a tiny subset of possible device names. The file can be replaced with the
 * full version of pci.ids.
 *
 * \author Richard Nusser
 * \copyright 2017 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_apps
 */

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/ShellCEntryLib.h>
#include "../include/console.h"
#include "../include/pci.h"


/** helper macro to access the `-print-classes` argument value */
#define ARG_PRINT_CLASSES _args[0].value.uint64

/** list of command-line arguments */
static cmdline_argument_t _args[] = {
  {{uint64:0},ARG_BOOL,NULL,L"-print-classes",L"Prints known PCI device classes"},
};

/** command-line argument group */
static ARG_GROUP(_argsgroup,_args,L"Application-specific options");

/**
 * Main function, gets invoked by UEFI shell.
 *
 * \param argc the number of command-line arguments passed
 * \param argv the command-line arguments passed
 * \return an EFI status code for the shell
 */
INTN EFIAPI ShellAppMain(UINTN argc, CHAR16 **argv)
{
  EFI_STATUS rv;
  if((rv=init(argc,argv,1,&_argsgroup))!=EFI_SUCCESS)
    return rv;
  init_pci_lib();

  if(ARG_PRINT_CLASSES)
    print_known_pci_classes();

  print_pci_devices();

  shutdown_pci_lib();
  shutdown();
  return EFI_SUCCESS;
}
