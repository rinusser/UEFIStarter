/**
 * Console initialization and I/O helpers
 *
 * \author Richard Nusser
 * \copyright 2017 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \link https://github.com/rinusser/UEFIStarter
 */
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include "../include/console.h"
#include "../include/memory.h"
#include "../include/string.h"
#include "../include/logger.h"


void print_console_modes()
{
  int tc;
  EFI_STATUS result;
  UINTN cols, rows;

  Print(L"number of console modes: %d\n",gST->ConOut->Mode->MaxMode);
  for(tc=0;tc<gST->ConOut->Mode->MaxMode;tc++)
  {
    result=gST->ConOut->QueryMode(gST->ConOut,tc,&cols,&rows);
    if(result!=EFI_SUCCESS)
      Print(L"  %02d: (error)",tc);
    else
      Print(L"  %02d: %3dx%3d",tc,cols,rows);
    if(tc%5==4)
      Print(L"\n");
  }
  if(tc%5!=4)
    Print(L"\n");
}

EFI_STATUS set_console_mode(unsigned int requested_mode)
{
  EFI_STATUS result;

  if(requested_mode > gST->ConOut->Mode->MaxMode-1)
  {
    LOG.error(L"requested mode %d, but %d is max",requested_mode,gST->ConOut->Mode->MaxMode-1);
    return EFI_UNSUPPORTED;
  }
  if(gST->ConOut->Mode->Mode==requested_mode)
  {
    LOG.debug(L"already at console mode %d",requested_mode);
    return EFI_SUCCESS;
  }

  result=gST->ConOut->SetMode(gST->ConOut,requested_mode);
  if(result!=EFI_SUCCESS)
    return result;

  LOG.debug(L"switched to console mode %d",requested_mode);
  return result;
}

void EFIAPI color_print(UINTN color, CHAR16 *fmt, ...)
{
  CHAR16 *str;
  UINTN attr=gST->ConOut->Mode->Attribute;
  VA_LIST args;

  VA_START(args,fmt);
  str=CatVSPrint(NULL,fmt,args);
  VA_END(args);

  gST->ConOut->SetAttribute(gST->ConOut,(attr&0xFFFFFFF0)+(color&0x0F));
  Print(str);
  gST->ConOut->SetAttribute(gST->ConOut,attr);

  FreePool(str);
}

void drain_key_buffer()
{
  EFI_INPUT_KEY key;
  EFI_STATUS result;
  UINTN tc;

  for(tc=0;tc<50;tc++)
  {
    result=gST->ConIn->ReadKeyStroke(gST->ConIn,&key);
    if(result!=EFI_SUCCESS)
      return;
  }
}

void wait_for_key()
{
  UINTN dummy;

  gST->BootServices->WaitForEvent(1,&gST->ConIn->WaitForKey,&dummy);
  drain_key_buffer();
}


//TODO: move the argv functions to cmdline.c

static CHAR16 **_argv=NULL;
static UINTN _argv_pages=0;

CHAR16 **argv_from_ascii(int argc, char **argv_ascii)
{
  unsigned int tc;
  unsigned int bytes;
  LOGLEVEL previous_log_level;

  bytes=argc*sizeof(CHAR16 *);
  _argv_pages=(bytes-1)/4096+1;
  previous_log_level=set_log_level(INFO);
  _argv=allocate_pages_ex(_argv_pages,FALSE);
  for(tc=0;tc<argc;tc++)
    _argv[tc]=memsprintf(L"%a",argv_ascii[tc]); //this is
  set_log_level(previous_log_level);
  return _argv;
}

void free_argv()
{
  free_pages_ex(_argv,_argv_pages,FALSE);
}


EFI_STATUS EFIAPI init(INTN argc, CHAR16 **argv, UINTN arg_group_count, ...)
{
  EFI_STATUS result;
  VA_LIST args;

//  InitializeLib(image, est);
//  _uefi_image=image;
  reset_memory_tracking();
  init_tracking_memory();
  reset_logger_entry_counts();
  VA_START(args,arg_group_count);
  result=parse_parameters(argc,argv,arg_group_count,args);
  VA_END(args);
  if(result!=EFI_SUCCESS)
    return result;

  if(set_console_mode(gST->ConOut->Mode->MaxMode-1)!=EFI_SUCCESS) //XXX could add cmdline arg
    print_console_modes();

  return EFI_SUCCESS;
}

void shutdown()
{
  stop_tracking_memory();
}
