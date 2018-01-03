/** \file
 * This application is a simple example of how to read command line arguments and output to console.
 *
 * \author Richard Nusser
 * \copyright 2017 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_apps
 *
 * \TODO pkg/mod config must be incomplete, why do we need relative paths for includes?!
 */

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/ShellCEntryLib.h>
#include "../include/logger.h"
#include "../include/cmdline.h"
#include "../include/string.h"
#include "../include/console.h"
#include "../include/files.h"


/**
 * Validator function for "-int" parameter.
 * Values must be greater than or equal to 2 to be valid.
 *
 * \param value the value to validate
 * \return whether the value is >=2
 */
BOOLEAN validate_int(double_uint64_t value)
{
  if(value.uint64>=2)
    return TRUE;
  LOG.error(L"int must be >=2");
  return FALSE;
}

/** command-line arguments in group 1 */
cmdline_argument_t args1[] = {
  {{uint64:0},ARG_BOOL,  NULL,L"-bool",L"boolean parameter"},
  {{dbl:0.66},ARG_DOUBLE,NULL,L"-dbl", L"double parameter"},
};

/** command-line argument group 1 */
ARG_GROUP(_arg_group1,args1,L"Group 1");

/** command-line arguments in group 2 */
cmdline_argument_t args2[] = {
  {{uint64:2},ARG_INT,validate_int,L"-int",L"integer parameter"},
};

/** command-line argument group 2 */
ARG_GROUP(_arg_group2,args2,L"Group 2");


/**
 * Main function, gets invoked by UEFI shell.
 *
 * \param argc the number of command-line arguments passed
 * \param argv the command-line arguments passed
 * \return an EFI status code for the shell
 */
INTN EFIAPI ShellAppMain(UINTN argc, CHAR16 **argv)
{
  EFI_STATUS result;
  Print(L"Greetings, non-spherical habitation rock!\n");

  if((result=init(argc,argv,2,&_arg_group1,&_arg_group2))!=EFI_SUCCESS)
    return result;

  Print(L"\nThere's a  -help  parameter that'll show command line options!\n\n");

  Print(L"effective argument values after defaults:\n");
  Print(L"  -bool: %d\n",args1[0].value.uint64);
  Print(L"  -dbl:  %s\n",ftowcs(args1[1].value.dbl));
  Print(L"  -int:  %d\n\n",args2[0].value.uint64);

  shutdown();
  return EFI_SUCCESS;
}
