/** \file
 * File handling functions
 *
 * \author Richard Nusser
 * \copyright 2017 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_lib_files
 */

#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/SimpleFileSystem.h>
#include <Guid/FileInfo.h>
#include "../include/files.h"
#include "../include/memory.h"
#include "../include/logger.h"

/**
 * Opens a handle for the first filesystem root.
 * In the UEFI shell, this will most likely be FS0:.
 *
 * \return the root volume's handle on success, NULL otherwise
 */
EFI_FILE_HANDLE find_root_volume()
{
  EFI_FILE_IO_INTERFACE *protocol;
  EFI_FILE_HANDLE root;
  EFI_GUID guid=SIMPLE_FILE_SYSTEM_PROTOCOL;
  EFI_HANDLE handles[100];
  UINTN handles_size=100*sizeof(EFI_HANDLE);
  UINTN handle_count;

  if(gST->BootServices->LocateHandle(ByProtocol,&guid,NULL,&handles_size,(void**)&handles)!=EFI_SUCCESS)
    return NULL;
  handle_count=handles_size/sizeof(EFI_HANDLE);
  LOG.debug(L"handles size: %d bytes (%d entries)",handles_size,handle_count);

  if(gST->BootServices->OpenProtocol(handles[0],&guid,(void **)&protocol,gImageHandle,NULL,EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL)!=EFI_SUCCESS)
    return NULL;
  if(protocol->OpenVolume(protocol,&root)!=EFI_SUCCESS)
    return NULL;

  return root;
}

/**
 * Opens a file handle, if the given file exists.
 * This assumes the file is on the first root volume (usually FS0:).
 *
 * \param pathname the file's full path within the volume, e.g. "\\startup.nsh" to get the startup script.
 * \return a handle to the file on success, NULL otherwise
 */
EFI_FILE_HANDLE find_file(CHAR16 *pathname)
{
  EFI_FILE_HANDLE root, file;

  root=find_root_volume();
  if(!root)
    return NULL;
  LOG.trace(L"found root volume, looking for %s...",pathname);
  if(root->Open(root,&file,pathname,EFI_FILE_MODE_READ,0)!=EFI_SUCCESS)
    return NULL;
  LOG.trace(L"found requested file, closing...");
  root->Close(root);
  return file;
}

/**
 * Reads a file's contents.
 * If you just want to read files you'll probably want to use this function: it performs all the UEFI overhead for you
 * already. All you have to do is free the returned pointer's memory pages when you're done.
 *
 * \param filename the file's full path within the volume, e.g. "\\startup.nsh" to get the startup script.
 * \return a pointer to the file content descriptor, or NULL on error
 */
file_contents_t *get_file_contents(CHAR16 *filename)
{
  EFI_FILE_HANDLE file;
  UINTN bufsize=SIZE_OF_EFI_FILE_INFO+200;
  char buffer[bufsize];
  EFI_FILE_INFO *info=(EFI_FILE_INFO *)buffer;
  EFI_GUID info_guid=EFI_FILE_INFO_ID;
  UINTN pages;
  file_contents_t *file_contents=NULL;

  file=find_file(filename);
  if(!file)
    return NULL;

  if(file->GetInfo(file,&info_guid,&bufsize,info)!=EFI_SUCCESS)
    return NULL;
  LOG.trace(L"filename: %s (%ld bytes)",info->FileName,info->FileSize);

  pages=(info->FileSize+sizeof(file_contents_t))/4096+1;
  if((file_contents=allocate_pages(pages))==NULL)
    return NULL;

  file_contents->memory_pages=pages;
  file_contents->data_length=info->FileSize;
  bufsize=pages*4096;

  /** \TODO start a list of pitfalls, e.g. this: bufsize was too small (unsigned int instead of UINTN), so this call
            changed the last declared uninitialized variable in this function */
  if(file->Read(file,&bufsize,file_contents->data)!=EFI_SUCCESS)
    return NULL;

  file->Close(file);
  return file_contents;
}
