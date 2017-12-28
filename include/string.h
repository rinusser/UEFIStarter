/**
 * String functions missing from EDK
 *
 * \author Richard Nusser
 * \copyright 2017 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \link https://github.com/rinusser/UEFIStarter
 */
#ifndef __STRING_H
#define __STRING_H

#include <Uefi.h>

CHAR16 *ftowcs(double value);
CHAR16 *sprint_status(CHAR16 *funcname, EFI_STATUS status);

BOOLEAN ctype_whitespace(char ch);
UINT64 atoui64(char *str);

CHAR16* EFIAPI memsprintf(const CHAR16 *fmt, ...);

UINTN split_string(CHAR16 ***list, CHAR16 *input, CHAR16 separator);

#endif
