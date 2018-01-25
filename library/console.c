/** \file
 * Console initialization and I/O helpers
 *
 * \author Richard Nusser
 * \copyright 2017-2018 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_lib_console
 */

#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include "../include/console.h"
#include "../include/memory.h"
#include "../include/string.h"
#include "../include/logger.h"


/**
 * Prints the list of available console modes.
 * Keep in mind that UEFI requires all systems to list modes 0 and 1, but only 0 and any listed 2+ have to work.
 */
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

/**
 * Sets a new text mode.
 * Keep in mind that the graphics mode takes precedence: if, for example, you set graphics mode 640x480 and then change
 * to a text mode that would require 960x700 pixels you'll get the requested console size but the window will remain
 * at 640x480!
 *
 * \param requested_mode the graphics mode to set
 * \return an EFI status code
 */
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

/**
 * Prints a text with the given color.
 * Takes the same format strings as EDK's print functions.
 *
 * \param color the color value to print with, as a 4-bit number
 * \param fmt   the format string, as UTF-16
 * \param ...   any parameters to the print function
 */
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

/**
 * Empties the keyboard input buffer.
 * Use this e.g. before and after waiting for the much sought after "any" key to prevent previous keystrokes from
 * canceling the wait and the new keystrokes from triggering later input events.
 */
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

/**
 * Waits for any keystroke.
 * Note that if you're in a virtualized environment some keystrokes will not trigger this, depending on your input
 * method, terminal emulation, console multiplexers and so on. This is most apparent with the Escape key: under some
 * circumstances it just won't register as an individual keystroke.
 */
void wait_for_key()
{
  UINTN dummy;

  gST->BootServices->WaitForEvent(1,&gST->ConIn->WaitForKey,&dummy);
  drain_key_buffer();
}


/** internal storage for command line arguments converted to UTF-16 */
static CHAR16 **_argv=NULL;

/** number of memory pages allocated for _argv */
static UINTN _argv_pages=0;


/**
 * Converts ASCII command line parameters to UTF-16.
 * The StdLib's entry function passes arguments as ASCII, you can use this function to convert the arguments list.
 *
 * \param argc       the number of command-line arguments
 * \param argv_ascii the list of command-line arguments, as ASCII
 * \return the same command-line arguments, as UTF-16
 *
 * \TODO move the argv functions to cmdline.c
 */
CHAR16 **argv_from_ascii(int argc, char **argv_ascii)
{
  unsigned int tc;
  unsigned int bytes;
  LOGLEVEL previous_log_level;

  bytes=argc*sizeof(CHAR16 *);
  _argv_pages=(bytes-1)/4096+1;
  previous_log_level=set_log_level(INFO);
  _argv=allocate_pages_ex(_argv_pages,FALSE,AllocateAnyPages,NULL);
  for(tc=0;tc<argc;tc++)
    _argv[tc]=memsprintf(L"%a",argv_ascii[tc]); //this is
  set_log_level(previous_log_level);
  return _argv;
}

/**
 * Frees the memory pages for the internal _argv storage
 *
 * \TODO move the argv functions to cmdline.c
 */
void free_argv()
{
  free_pages_ex(_argv,_argv_pages,FALSE);
}


/**
 * Initializes an UEFIStarter application.
 * Call this as early as possible to gain access to the memory tracking, logging and other features.
 *
 * If this function returns error/warning codes the exact code should tell what went wrong; usually there are log
 * entries as well.
 *
 * Keep in mind that the RV_HELP return code isn't an error per se, it's just to indicate that the user
 * wanted (and received) the help screen.
 *
 * \param argc            the number of command-line arguments
 * \param argv            the list of command-line arguments, as UTF-16
 * \param arg_group_count the number of command-line argument groups
 * \param ...             the list of command-line argument groups (cmdline_argument_group_t *)
 * \return an EFI status code: EFI_SUCCESS if everything went well and application can continue executing
 */
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

  if(set_console_mode(gST->ConOut->Mode->MaxMode-1)!=EFI_SUCCESS) /** \TODO could add cmdline arg */
    print_console_modes();

  return EFI_SUCCESS;
}

/**
 * Shuts the UEFIStarter internals down.
 * This will stop the memory tracker, upon which any unfreed memory gets reported.
 *
 * Unfreed memory will continue to use up memory in the UEFI environment even after the application stopped. If you
 * didn't keep the pages allocated on purpose you'll probably want to free them before calling this.
 *
 * If you're keeping the pages allocated on purpose (e.g. because you need to mark memory as reserved) you can disable
 * tracking (and thus reporting) for them by using allocate_pages_ex() with track=FALSE (if you make it accessible with
 * "extern"). Alternatively you can just use UEFI's AllocatePages() function, it doesn't track allocated pages at all.
 */
void shutdown()
{
  stop_tracking_memory();
}
