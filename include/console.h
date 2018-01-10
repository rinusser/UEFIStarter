/** \file
 * CLI initialization and I/O helpers
 *
 * \author Richard Nusser
 * \copyright 2017-2018 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_lib_console
 */

#ifndef __CONSOLE_H
#define __CONSOLE_H

#include <Uefi.h>
#include "cmdline.h"

CHAR16 **argv_from_ascii(int argc, char **argv_ascii);
void free_argv();

EFI_STATUS EFIAPI init(INTN argc, CHAR16 **argv, UINTN arg_group_count, ...);
void shutdown();

void print_console_modes();
EFI_STATUS set_console_mode(unsigned int requested_mode);
void EFIAPI color_print(UINTN color, CHAR16 *fmt, ...);


void drain_key_buffer();
void wait_for_key();

#endif
