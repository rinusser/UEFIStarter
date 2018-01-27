/** \file
 * This application will halt the UEFI environment.
 * QEMU will exit and return to the system shell, VirtualBox will halt the VM.
 *
 * \author Richard Nusser
 * \copyright 2017-2018 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_apps
 */

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/ShellCEntryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <UEFIStarter/core.h>

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
  if((rv=init(argc,argv,0))!=EFI_SUCCESS)
    return rv;

  Print(L"shutting down...\n");
  gST->RuntimeServices->ResetSystem(EfiResetShutdown,EFI_SUCCESS,0,NULL);

  shutdown();
  return EFI_UNSUPPORTED;
}
