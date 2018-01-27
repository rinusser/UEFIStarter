/** \file
 * Functions for accessing PCI devices
 *
 * \author Richard Nusser
 * \copyright 2017-2018 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_lib_pci
 *
 * \TODO read classes and subclasses from pci.ids
 */

#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <UEFIStarter/pci.h>
#include <UEFIStarter/core/memory.h>
#include <UEFIStarter/core/files.h>
#include <UEFIStarter/core/string.h>
#include <UEFIStarter/core/logger.h>


/** the highest number of PCI device entries supported */
#define MAX_PCI_DEVICES 100

static EFI_PCI_IO_PROTOCOL *_pci_protocols[MAX_PCI_DEVICES]; /**< the list of PCI UEFI protocol handlers */
static EFI_HANDLE _pci_handles[MAX_PCI_DEVICES];             /**< the list of PCI device handles */
static UINTN _pci_handle_count=0;                            /**< the amount of PCI device handles */
static PCI_TYPE00 _pci_configs[MAX_PCI_DEVICES];             /**< the PCI device's TYPE00 headers */
static file_contents_t *_pci_id_file=NULL;                   /**< the pci.ids file contents */


/** PCI subclasses for Mass Storage Controllers */
static pci_subclass_name_t PCI_CLASS01_SUBCLASSES[]={
  {0,L"SCSI Controller"},
  {1,L"IDE Controller"},
  {2,L"Floppy Disk Controller"},
  {4,L"RAID Controller"},
  {5,L"ATA Controller"},
  {6,L"SATA Controller"},
  {7,L"SAS Controller"},
  {0x80,L"Other"},
  {0,NULL}
};

/** PCI subclasses for Network Controllers */
static pci_subclass_name_t PCI_CLASS02_SUBCLASSES[]={
  {0,L"Ethernet"},
  {0,NULL}
};

/** PCI subclasses for Display Controllers */
static pci_subclass_name_t PCI_CLASS03_SUBCLASSES[]={
  {0,L"VGA"},
  {1,L"XGA"},
  {0x80,L"Other"},
  {0,NULL}
};

/** PCI subclasses for Multimedia devices */
static pci_subclass_name_t PCI_CLASS04_SUBCLASSES[]={
  {0,L"Video Device"},
  {1,L"Audio Device"},
  {0,NULL}
};

/** PCI subclasses for Bridge Devices */
static pci_subclass_name_t PCI_CLASS06_SUBCLASSES[]={
  {0,L"Host/PCI"},
  {1,L"PCI/ISA"},
  {2,L"PCI/EISA"},
  {3,L"PCI/Micro Channel"},
  {4,L"PCI/PCI"},
  {5,L"PCI/PCMCIA"},
  {6,L"PCI/NuBus"},
  {7,L"PCI/CardBus"},
  {0x80,L"Other"},
  {0,NULL}
};

/** PCI subclasses for Base System Peripherals */
static pci_subclass_name_t PCI_CLASS08_SUBCLASSES[]={
  {0x80,L"Other"},
  {0,NULL}
};

/** PCI subclasses for Serial Bus Controllers */
static pci_subclass_name_t PCI_CLASS0C_SUBCLASSES[]={
  {0,L"IEEE 1394 Controller (FireWire)"},
  {3,L"USB Controller"},
  {0,NULL}
};

/** PCI classes */
static pci_class_names_t PCI_CLASSES[]={
  {1,L"Mass Storage Controller",PCI_CLASS01_SUBCLASSES},
  {2,L"Network Controller",PCI_CLASS02_SUBCLASSES},
  {3,L"Display Controller",PCI_CLASS03_SUBCLASSES},
  {4,L"Multimedia",PCI_CLASS04_SUBCLASSES},
  {6,L"Bridge Device",PCI_CLASS06_SUBCLASSES},
  {8,L"Base System Peripheral",PCI_CLASS08_SUBCLASSES},
  {0xc,L"Serial Bus Controller",PCI_CLASS0C_SUBCLASSES},
  {0,NULL,NULL}
};

/**
 * internal: converts a hexadecimal value into a printable hex character.
 *
 * \param value the hex value (0-15) to format
 * \return the lowercase hexadecimal character, or '?' if the input was invalid
 */
static CHAR8 hexdigita(UINT8 value)
{
  if(value<10)
    return '0'+value;
  else if(value<16)
    return 'a'+value-10;
  LOG.error(L"invalid hex digit value: %d",value);
  return '?';
}

/**
 * internal: converts a UINT16 value to a printable lowercase hex string.
 * 5 bytes of output will be written, starting where `buffer` points to. The last bytes is a binary zero for string
 * termination.
 *
 * \param buffer where to write the string to (as ascii)
 * \param value  the value to format
 */
static void ui16tohexa(char *buffer, UINT16 value)
{
  buffer[0]=hexdigita((value&0xf000)>>12);
  buffer[1]=hexdigita((value&0x0f00)>>8);
  buffer[2]=hexdigita((value&0x00f0)>>4);
  buffer[3]=hexdigita((value&0x000f));
  buffer[4]=0;
}

/**
 * Looks up a PCI device's name by vendor ID and device ID.
 * Subvendor and subdevice IDs are not implemented yet, they will be ignored.
 *
 * \param vendor_id    the device's vendor ID
 * \param device_id    the device's device ID
 * \param subvendor_id ignored
 * \param subdevice_id ignored
 * \return the device's name, as UTF-16; returns "(unknown)" if the requested device wasn't found
 *
 * \TODO later: implement sub IDs.
 */
CHAR16 *find_pci_device_name(UINT16 vendor_id, UINT16 device_id, UINT16 subvendor_id, UINT16 subdevice_id)
{
  char unknown_device_str[]="unknown device";
  char vendor_start_str[]="\nXXXX";
  char device_str[]="XXXX";
  char *start;
  char *newline;
  char vendor_name[100];
  char device_name[100];
  int length;
  char eol[]="\n";


  if(!_pci_id_file)
  {
    _pci_id_file=get_file_contents(L"\\pci.ids");
    _pci_id_file->data[_pci_id_file->data_length-1]=0;
  }

  ui16tohexa(vendor_start_str+1,vendor_id);
  start=AsciiStrStr(_pci_id_file->data,vendor_start_str);
  if(!start)
  {
    LOG.debug(L"unknown vendor ID: %04X",vendor_id);
    return memsprintf(L"(unknown)");
  }

  newline=AsciiStrStr(start+1,eol);
  length=newline-start-7;
  if(length>100)
  {
    LOG.warn(L"trimmed vendor name to 100 characters");
    length=100;
  }
//  SetMem(vendor_name,100,0xca);
  CopyMem(vendor_name,start+7,length);
  vendor_name[length]=0;
//  DumpHex(2,0,100,vendor_name); //this should show vendor string starting at 0, followed by \0 character without newline, then 0xCA until the end

  ui16tohexa(device_str,device_id);

  start=NULL;
  while(newline)
  {
    if(newline[1]=='#')
    {
      //current line contains a comment, ignore it
    }
    else if(newline[1]!='\t')
    {
      LOG.debug(L"unknown device ID: %04X",device_id);
      break;
    }
    else if(newline[2]==device_str[0] && newline[3]==device_str[1] && newline[4]==device_str[2] && newline[5]==device_str[3])
    {
      LOG.debug(L"found device ID at pos %X",newline-_pci_id_file->data+1);
      start=newline+8;
      newline=AsciiStrStr(start,eol);
      length=newline-start;
      if(length>100)
      {
        LOG.warn(L"trimmed device name to 100 characters");
        length=100;
      }
//      SetMem(device_name,100,0xca);
      CopyMem(device_name,start,length);
      device_name[length]=0;
//      DumpHex(2,0,100,device_name); //same as other dumphex() above: no trailing whitespace, \0 at string end without newline
      break;
    }
    newline=AsciiStrStr(newline+1,eol);
  }
  if(!start)
    CopyMem(device_name,unknown_device_str,AsciiStrLen(unknown_device_str)+1);

  return memsprintf(L"%a, %a",vendor_name,device_name);
}

/**
 * Finds a PCI class and subclass name by 3-byte class code.
 *
 * \param class_code the PCI class code to look up
 * \return the class name; uses "unknown" to replace unhandled classes or subclasses
 */
CHAR16 *find_pci_class_name(UINT8 class_code[3])
{
  UINT8 base_class=class_code[2];
  UINT8 sub_class=class_code[1];

  UINT16 *base_name=L"unknown";
  UINT16 *sub_name=L"unknown";

  int tc, td;
  for(tc=0;tc<1000;tc++)
  {
    if(!PCI_CLASSES[tc].class_name)
      break;
    if(PCI_CLASSES[tc].class_code!=base_class)
      continue;
    base_name=PCI_CLASSES[tc].class_name;
    if(!PCI_CLASSES[tc].subclasses)
      continue;

    for(td=0;td<1000;td++)
    {
      if(!PCI_CLASSES[tc].subclasses[td].subclass_name)
        break;
      if(PCI_CLASSES[tc].subclasses[td].subclass_code!=sub_class)
        continue;
      sub_name=PCI_CLASSES[tc].subclasses[td].subclass_name;
      break;
    }
    break;
  }

  return memsprintf(L"%s, %s",base_name,sub_name);
}

/**
 * Prints basic information about a PCI device.
 *
 * \param config the PCI's TYPE00 header
 */
void describe_pci_device(PCI_TYPE00 *config)
{
  CHAR16 *classname;
  CHAR16 *name;

  name=find_pci_device_name(config->Hdr.VendorId,config->Hdr.DeviceId,config->Device.SubsystemVendorID,config->Device.SubsystemID);
  classname=find_pci_class_name(config->Hdr.ClassCode);

  Print(L"[%04X:%04X] %s\n",config->Hdr.VendorId,config->Hdr.DeviceId,name);
  if(classname!=NULL)
    Print(L"       type: %s\n",classname);
  Print(L"       status=%04X, command=%04X\n",config->Hdr.Status,config->Hdr.Command);
  Print(L"       prog_if=%02X, baseclass_code=%02X, subclass_code=%02X, revision_id=%02X\n",
      config->Hdr.ClassCode[0],config->Hdr.ClassCode[2],config->Hdr.ClassCode[1],config->Hdr.RevisionID);
}

/**
 * Assembles the internal list of PCI devices.
 *
 * \return 0 on success, -1 on failure.
 */
int enumerate_pci_devices()
{
  EFI_STATUS result;
  UINTN handles_size=MAX_PCI_DEVICES*sizeof(EFI_HANDLE);
  unsigned int tc;
  EFI_PCI_IO_PROTOCOL *pip;
  CHAR16 textbuffer[256];
  EFI_GUID guid=EFI_PCI_IO_PROTOCOL_GUID;

  if(gST->BootServices->LocateHandle(ByProtocol,&guid,NULL,&handles_size,(void **)&_pci_handles)!=EFI_SUCCESS)
    return -1;

  _pci_handle_count=handles_size/sizeof(EFI_HANDLE);
  LOG.debug(L"handles size: %d bytes (%d entries)",handles_size,_pci_handle_count);

  for(tc=0;tc<_pci_handle_count;tc++)
  {
    _pci_protocols[tc]=NULL;
    _pci_configs[tc].Hdr.VendorId=0xFFFF;
    _pci_configs[tc].Hdr.DeviceId=0xFFFF;

    result=gST->BootServices->OpenProtocol(_pci_handles[tc],&guid,(void **)&pip,gImageHandle,NULL,EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    if(result!=EFI_SUCCESS)
    {
      sprint_status(L"OpenProtocol",result);
      LOG.warn(textbuffer);
      continue;
    }

    result=pip->Pci.Read(pip,EfiPciIoWidthUint8,0,sizeof(PCI_TYPE00),&_pci_configs[tc]);
    if(result!=EFI_SUCCESS)
    {
      sprint_status(L"Pci.Read",result);
      LOG.warn(textbuffer);
      continue;
    }

    _pci_protocols[tc]=pip;
  }
  return 0;
}

/**
 * Fetches a PCI device's protocol handle.
 * This is currently the easiest way to access a specific PCI device.
 *
 * \param vendor_id the device's vendor ID
 * \param device_id the device's device ID
 * \return the protocol handle if the device was found, NULL otherwise
 */
EFI_PCI_IO_PROTOCOL *find_pci_device(UINT16 vendor_id, UINT16 device_id)
{
  unsigned int tc;

  if(_pci_handle_count<1)
    enumerate_pci_devices(gImageHandle);

  for(tc=0;tc<_pci_handle_count;tc++)
    if(_pci_configs[tc].Hdr.VendorId==vendor_id && _pci_configs[tc].Hdr.DeviceId==device_id)
      return _pci_protocols[tc];

  return NULL;
}


/**
 * Prints a short description of all connected PCI devices.
 */
void print_pci_devices()
{
  int tc;

  if(_pci_handle_count<1)
    enumerate_pci_devices(gImageHandle);

  for(tc=0;tc<_pci_handle_count;tc++)
  {
    Print(L"  #%02d: ",tc);
    describe_pci_device(&_pci_configs[tc]);
  }
}

/**
 * Prints a list of known PCI classes and subclasses.
 * This is mostly for debugging.
 */
void print_known_pci_classes()
{
  int tc, td;

  for(tc=0;tc<1000;tc++)
  {
    if(!PCI_CLASSES[tc].class_name)
      break;
    Print(L"PCI class %02X: %s\n",PCI_CLASSES[tc].class_code,PCI_CLASSES[tc].class_name);
    if(!PCI_CLASSES[tc].subclasses)
    {
      Print(L"  (no subclass entries)\n");
      continue;
    }
    for(td=0;td<1000;td++)
    {
      if(!PCI_CLASSES[tc].subclasses[td].subclass_name)
        break;
      Print(L"  subclass %02X: %s\n",PCI_CLASSES[tc].subclasses[td].subclass_code,PCI_CLASSES[tc].subclasses[td].subclass_name);
    }
  }
}

/**
 * Initializes the PCI library.
 * Call this before using the other library functions.
 */
void init_pci_lib()
{
  _pci_handle_count=0;
  _pci_id_file=NULL;
}

/**
 * Shuts down the PCI library.
 * Call this when you're done with PCI functions.
 */
void shutdown_pci_lib()
{
  if(_pci_id_file)
  {
    free_pages(_pci_id_file,_pci_id_file->memory_pages);
    _pci_id_file=NULL;
  }
}
