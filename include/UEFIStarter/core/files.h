/** \file
 * File handling functions
 *
 * \author Richard Nusser
 * \copyright 2017-2018 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_lib_files
 */

#ifndef __FILES_H
#define __FILES_H

#include <Uefi.h>
#include <Guid/FileInfo.h>
#include <Protocol/SimpleFileSystem.h>

/**
 * Data structure for file contents.
 * This gets allocated dynamically, make sure you free the memory pages when you're done with it.
 */
typedef struct {
  UINTN memory_pages; /**< the number of memory pages required to hold the current instance */
  UINT64 data_length; /**< the file's content length */
  char data[];        /**< the file's content data */
} file_contents_t;

EFI_FILE_HANDLE find_root_volume();
EFI_FILE_HANDLE find_file(CHAR16 *pathname);
file_contents_t *get_file_contents(CHAR16 *filename);


#endif
