/** \file
 * This application showcases ways to read input from the keyboard.
 *
 * Keep in mind that terminals (and terminal multiplexers) in virtualized environments often don't send the pressed keys'
 * raw scancodes but escape sequences instead. For example you may have to hit the Escape key 2-3 times in succession
 * until it's registered once.
 *
 * \author Richard Nusser
 * \copyright 2017-2018 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_apps
 */

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <UEFIStarter/core.h>

/** helper macro to access the "handle offset" command-line argument's value */
#define ARG_HANDLE           _args[0].value.uint64

/** helper macro to access the alternative wait command-line argument's value */
#define ARG_OTHER_WAIT_EVENT _args[1].value.uint64

/** command-line arguments */
static cmdline_argument_t _args[] = {
  {{uint64:0},ARG_INT, NULL,L"-handle",          L"Use (zero-based) nth handle"},
  {{uint64:0},ARG_BOOL,NULL,L"-other-wait-event",L"Use alternate wait event"},
};

/** command-line argument group */
static ARG_GROUP(_arguments,_args,L"Application-specific options");


/**
 * Reads keyboard input the easiest way: SIMPLE_TEXT_INPUT_PROTOCOL.
 */
void test_simple_input()
{
  EFI_STATUS result;
  EFI_INPUT_KEY key;
  UINTN tc;

  for(tc=0;tc<1000;tc++)
  {
    Print(L"waiting for key (q to exit)... ");
    gST->BootServices->WaitForEvent(1,&gST->ConIn->WaitForKey,&tc);
    ON_ERROR_RETURN(L"WaitForEvent",);
    result=gST->ConIn->ReadKeyStroke(gST->ConIn,&key);
    ON_ERROR_RETURN(L"ReadKeyStroke",);
    Print(L"done: scancode=%04X, char=%1c (%04X)\n",key.ScanCode,key.UnicodeChar>0x32?key.UnicodeChar:L' ',key.UnicodeChar);
    if(key.UnicodeChar==L'q')
      break;
  }
}

/**
 * Locates an UEFI protocol handler, opens and returns it.
 *
 * UEFI protocols can have multiple handlers attached, use the (0-based) "offset" parameter to determine which handler
 * you want. Usually the handler at offset 0 works fine.
 *
 * \param guid   the protocol GUID to locate
 * \param offset makes this function return the (0-based) nth handler for the requested protocol
 * \return the protoco handler's address, or NULL on error
 *
 * \TODO extract to lib and rename to open_protocol()
 */
void *find_device(EFI_GUID *guid, unsigned int offset)
{
  EFI_STATUS result;
  EFI_HANDLE handles[100];
  UINTN handles_size=100*sizeof(EFI_HANDLE);
  UINTN handle_count;
  void *device;

  result=gST->BootServices->LocateHandle(ByProtocol,guid,NULL,&handles_size,(void **)&handles);
  ON_ERROR_RETURN(L"LocateHandle",NULL);
  handle_count=handles_size/sizeof(EFI_HANDLE);
  LOG.debug(L"handles size: %d bytes (%d entries)",handles_size,handle_count);

  if(offset>=handle_count)
  {
    LOG.error(L"cannot get protocol handle, requested offset %d beyond handle count %d",offset,handle_count);
    return NULL;
  }
  LOG.trace(L"handle: %016lX",handles[offset]);
  result=gST->BootServices->OpenProtocol(handles[offset],guid,&device,gImageHandle,NULL,EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
  ON_ERROR_RETURN(L"OpenProtocol",NULL);

  return device;
}

/**
 * Finds the protocol handler for SIMPLE_TEXT_INPUT_EX_PROTOCOL
 *
 * \return the handler's address, or NULL on error
 */
EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *find_input_ex()
{
  EFI_GUID guid=EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL_GUID;
  return (EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *)find_device(&guid,ARG_HANDLE);
}

/**
 * Reads keyboard input with the extended protocol: SIMPLE_TEXT_INPUT_EX_PROTOCOL.
 */
void test_ex_input()
{
  EFI_STATUS result;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *prot;
  UINTN tc;
  EFI_KEY_DATA data;
  EFI_EVENT event;

  if(!(prot=find_input_ex()))
    return;

  event=ARG_OTHER_WAIT_EVENT?&gST->ConIn->WaitForKey:prot->WaitForKeyEx;

  for(tc=0;tc<1000;tc++)
  {
    Print(L"any key, or q to quit... ");
    gST->BootServices->WaitForEvent(1,&event,&tc);
    ON_ERROR_RETURN(L"WaitForEvent",);
    result=prot->ReadKeyStrokeEx(prot,&data);
    ON_ERROR_RETURN(L"ReadKeyStrokeEx",);
    if(data.Key.UnicodeChar==L'q')
    {
      Print(L"\n");
      return;
    }
    Print(L"scancode=%04X, key=%04X, shiftstate=%08X, toggles=%02X\n",data.Key.ScanCode,data.Key.UnicodeChar,data.KeyState.KeyShiftState,data.KeyState.KeyToggleState);
  }
}

/**
 * Callback function for keyboard handler, prints the pressed key(s).
 * This function gets called on individually registered keystrrokes only.
 *
 * \param data the pressed keys' data
 * \return the EFI status code, currently always indicating success
 */
EFI_STATUS EFIAPI ex_notify(EFI_KEY_DATA *data)
{
  Print(L"scancode=%04X, key=%04X, shiftstate=%08X, toggles=%02X\n",data->Key.ScanCode,data->Key.UnicodeChar,data->KeyState.KeyShiftState,data->KeyState.KeyToggleState);
  return EFI_SUCCESS;
}

/**
 * Registers a listener for a specific key combination and waits for that combination to be pressed.
 *
 * There are no wildcards allowing to register multiple keys at once and left/right modifier states (e.g. "left shift"
 * vs. "right shift") have to be registered separately. An application needs to register, remember and then unregister
 * handlers for each combination of keys, so this way of reading the keyboard is probably too cumbersome for large
 * amounts of key combinations.
 */
void test_ex_notify()
{
  EFI_STATUS result;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *prot;
  EFI_KEY_DATA data;
  void *handle;
  unsigned int tc;

  if(!(prot=find_input_ex()))
    return;

  data.Key.ScanCode=0;
  data.Key.UnicodeChar=L'q';
//  data.Key.UnicodeChar=0;
  data.KeyState.KeyShiftState=0;
  data.KeyState.KeyToggleState=0;
  result=prot->RegisterKeyNotify(prot,&data,(EFI_KEY_NOTIFY_FUNCTION)ex_notify,&handle);
  ON_ERROR_RETURN(L"RegisterKeyNotify",);

  Print(L"waiting for 10s, press the 'q' key as often as you want...");
  for(tc=0;tc<100;tc++)
  {
    result=gBS->Stall(100000);
    ON_ERROR_RETURN(L"Stall",);
  }

  result=prot->UnregisterKeyNotify(prot,handle);
  ON_ERROR_RETURN(L"UnregisterKeyNotify",);

  //at this point keys pressed while we waited are still in the keyboard buffer, remove them silently
  drain_key_buffer();
}


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
  if((rv=init(argc,argv,1,&_arguments))!=EFI_SUCCESS)
    return rv;

  test_simple_input();
  test_ex_input();
  test_ex_notify();

  shutdown();
  return EFI_SUCCESS;
}
