/** \file
 * AC'97 audio functions
 *
 * \author Richard Nusser
 * \copyright 2017-2018 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_lib_ac97
 */

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include "../include/ac97.h"
#include "../include/memory.h"
#include "../include/string.h"
#include "../include/logger.h"


/**
 * validator for "-volume" command-line argument
 *
 * \param value the value to validate
 * \return whether the valid is a valid volume
 */
BOOLEAN validate_volume(double_uint64_t value)
{
  if(value.dbl>=0.0 && value.dbl<=1.0)
    return TRUE;
  LOG.error(L"volume must be between 0.0 and 1.0 (inclusive)");
  return FALSE;
}

/**
 * validator for "-sample-rate" command-line argument
 *
 * \param value the value to validate
 * \return whether the valid is a valid sample rate
 *
 * \XXX this function could check the input value better, right now it's just a simple 0..65534 range
 */
BOOLEAN validate_sample_rate(double_uint64_t value)
{
  if(value.uint64<65535)
    return TRUE;
  LOG.error(L"sample rate must be <65536, double-rate audio not implemented");
  return FALSE;
}

/** AC'97 command-line argument list */
cmdline_argument_t ac97_argument_list[] = {
  {{uint64:0},    ARG_BOOL,  NULL,                L"-mute",       L"mutes output"},
  {{dbl:0.66},    ARG_DOUBLE,validate_volume,     L"-volume",     L"sets output volume min=0.0, max=1.0"},
  {{uint64:44100},ARG_INT,   validate_sample_rate,L"-sample-rate",L"sets sample rate (only 48000 guaranteed by AC'97 specs)"},
};

/** command-line argument group for AC'97 */
ARG_GROUP(ac97_arguments,ac97_argument_list,L"Audio options");


/**
 * Gets a PCI device handle for the AC'97 device.
 *
 * \return the PCI device handle, or NULL if none found
 */
EFI_PCI_IO_PROTOCOL *find_ac97_device()
{
  return find_pci_device(0x8086,0x2415);
}


/**
 * Initializes AC'97 audio buffers.
 *
 * \param buffers          the buffer structure to initialize
 * \param hardware_address the hardware memory address to write into the descriptor structure
 * \param count            the number of buffers to initialize
 * \return 0 on success, anything else on error
 *
 * \TODO seems like currently there's both a "count" parameter and the number "32" hard-coded. Pick one.
 */
int init_buffers(ac97_buffers_s16_t *buffers, UINT64 hardware_address, int count)
{
  unsigned int tc;
  void *hardware_base_addr;
  void *virtual_base_addr;

  if(hardware_address>0xfffff000) //needs to be castable to 32 bit, probably reduce limit further
    return -1;

  LOG.debug(L"setting up %d audio buffers at virtual %X, hardware %X",count,buffers,hardware_address);

  ZeroMem(buffers->descriptors,sizeof(ac97_buffer_descriptor_t)*32+sizeof(INT16 *)*32);
  hardware_base_addr=(void *)(hardware_address+sizeof(ac97_buffers_s16_t));
  virtual_base_addr=(void *)buffers+sizeof(ac97_buffers_s16_t);

  for(tc=0;tc<count;tc++)
  {
    buffers->descriptors[tc].address=(UINT64)(hardware_base_addr+tc*65536*2);
    buffers->buffers[tc]=virtual_base_addr+tc*65536*2;
    LOG.trace(L"descriptor %02d is at %X; .address=%X, actual buffer points to %X",
        tc,&buffers->descriptors[tc],buffers->descriptors[tc].address,buffers->buffers[tc]);
  }

  return 0;
}

/**
 * Writes to an AC'97 mixer register.
 *
 * \param handle the AC'97 handle to use
 * \param reg    the register to write to
 * \param value  the value to write
 * \return the resulting status, EFI_SUCCESS if everything went well
 */
EFI_STATUS write_mixer_reg(ac97_handle_t *handle, UINT32 reg, UINT16 value)
{
  return handle->pci->Io.Write(handle->pci,EfiPciIoWidthUint16,0,reg,1,&value);
}

/**
 * Reads an AC'97 mixer register.
 *
 * \param handle the AC'97 handle to use
 * \param reg    the register to read from
 * \param value  the output value
 * \return the resulting status, EFI_SUCCESS if everything went well
 */
EFI_STATUS read_mixer_reg(ac97_handle_t *handle, UINT32 reg, UINT16 *value)
{
  return handle->pci->Io.Read(handle->pci,EfiPciIoWidthUint16,0,reg,1,value);
}

/**
 * internal: returns a bus master register's width
 *
 * \param reg the bus master register
 * \return the width, for use in UEFI's Io.Write() and Io.Read()
 */
static inline UINTN get_busmaster_register_width(UINT32 reg)
{
  if(reg==AC97_DESCRIPTOR_PCM_OUT || reg==AC97_GLOBAL_CONTROL)
    return EfiPciIoWidthUint32;
  if(reg==AC97_STATUS_PCM_OUT)
    return EfiPciIoWidthUint16;
  return EfiPciIoWidthUint8;
}


/**
 * Writes to an AC'97 bus master register.
 *
 * \param handle the AC'97 handle to use
 * \param reg    the register to write to
 * \param value  the value to write
 * \return the resulting status, EFI_SUCCESS if everything went well
 */
EFI_STATUS write_busmaster_reg(ac97_handle_t *handle, UINT32 reg, UINTN value)
{
  return handle->pci->Io.Write(handle->pci,get_busmaster_register_width(reg),1,reg,1,&value);
}

/**
 * Reads an AC'97 bus master register.
 *
 * \param handle the AC'97 handle to use
 * \param reg    the register to read from
 * \param value  the output value
 * \return the resulting status, EFI_SUCCESS if everything went well
 */
EFI_STATUS read_busmaster_reg(ac97_handle_t *handle, UINT32 reg, UINTN *value)
{
  return handle->pci->Io.Read(handle->pci,get_busmaster_register_width(reg),1,reg,1,value);
}

/**
 * Takes the "volume" and "mute" command-line arguments and configures the AC'97 PCM OUT channel with them.
 *
 * \param handle the AC'97 handle to use
 * \return the resulting status, EFI_SUCCESS if everything went well
 */
EFI_STATUS set_ac97_cmdline_volume(ac97_handle_t *handle)
{
  UINT8 master_vol, pcm_out_vol;
  EFI_STATUS result;

  if(ARG_VOLUME<0)
  {
    ARG_VOLUME=0.0;
    LOG.warn(L"volume can't be less than 0, set to 0.0");
  }
  else if(ARG_VOLUME>1)
  {
    ARG_VOLUME=1.0;
    LOG.warn(L"volume can't be greater than 1.0, set to 1.0");
  }

  //XXX more granularity would require sqrt() function since output volume (most likely) is master*pcm_out
  pcm_out_vol=31;
  master_vol=(UINT8)(ARG_VOLUME*handle->max_master_vol);
  master_vol=handle->max_master_vol-master_vol;
  pcm_out_vol=31-pcm_out_vol;
  LOG.debug(L"master vol=%d, PCM vol=%d, mute=%d",master_vol,pcm_out_vol,ARG_MUTE);

  result=write_mixer_reg(handle,AC97_MIXER_MASTER,ac97_mixer_value(master_vol,master_vol,ARG_MUTE));
  ON_ERROR_RETURN(L"write_mixer_reg",result);

  result=write_mixer_reg(handle,AC97_MIXER_PCM_OUT,ac97_mixer_value(pcm_out_vol,pcm_out_vol,ARG_MUTE));
  ON_ERROR_RETURN(L"write_mixer_reg",result);

  return result;
}

/**
 * Takes the "sample rate" command-line argument and sets the AC'97 PCM OUT channel's sample rate to that.
 * Careful, this operation resets the "mute" flag on at least the master output channel, so call this before
 * set_ac97_cmdline_volume().
 *
 * \param handle the AC'97 handle to use
 * \return the resulting status, EFI_SUCCESS if everything went well
 */
EFI_STATUS set_ac97_cmdline_sample_rate(ac97_handle_t *handle)
{
  EFI_STATUS result;

  result=write_mixer_reg(handle,AC97_PCM_RATE_FRONT,ARG_SAMPLE_RATE);
  ON_ERROR_RETURN(L"write_mixer_reg",result);
  result=write_mixer_reg(handle,AC97_PCM_RATE_SURROUND,ARG_SAMPLE_RATE);
  ON_ERROR_RETURN(L"write_mixer_reg",result);
  result=write_mixer_reg(handle,AC97_PCM_RATE_LFE,ARG_SAMPLE_RATE);
  ON_ERROR_RETURN(L"write_mixer_reg",result);
  return result;
}

/**
 * Internal: determines the maximum master volume value.
 * The master volume register is either 5 or 6 bits wide. If the 6th bit is written to but not supported the first
 * 5 bits will be set to 1 instead.
 *
 * \param handle the AC'97 handle to use and update
 */
static void _determine_maximum_master_volume(ac97_handle_t *handle)
{
  EFI_STATUS result;
  UINT16 written_value;
  UINT16 read_value;

  handle->max_master_vol=0x1f; //when in doubt, default to safer maximum

  written_value=ac97_mixer_value(0x20,0x20,1);
  result=write_mixer_reg(handle,AC97_MIXER_MASTER,written_value);
  ON_ERROR_RETURN(L"write_mixer_reg",);
  read_mixer_reg(handle,AC97_MIXER_MASTER,&read_value);
  ON_ERROR_RETURN(L"read_mixer_reg",);

  if(written_value==read_value)
    handle->max_master_vol=0x3f;
}

/**
 * Initializes an AC'97 handle.
 *
 * \param handle the AC'97 handle to initialize
 * \param pip    the UEFI PCI I/O protocol to use
 * \return the handle on success, NULL otherwise
 */
void *init_ac97_handle(ac97_handle_t *handle, EFI_PCI_IO_PROTOCOL *pip)
{
  int buffer_count=32;
  UINTN pages;
  EFI_STATUS result;
  UINTN bufsize;

  handle->pci=pip;

  bufsize=buffer_count*65536*2+sizeof(ac97_buffers_s16_t);
  pages=bufsize/4096+1;

  handle->buffer_pages=0;

  /** \TODO restrict maximum address, something like 2^32-4096*pages */
  if((handle->buffers=allocate_pages(pages))==NULL)
    return NULL;
  handle->buffer_pages=pages;

  result=pip->Map(pip,EfiPciIoOperationBusMasterWrite,handle->buffers,&bufsize,&handle->device_address,&handle->mapping);
  ON_ERROR_RETURN(L"pip->Map",NULL);
  LOG.debug(L"bytes mapped: %d, device address: %016lX",bufsize,handle->device_address);

  if(handle->device_address>0xffffffff)
  {
    LOG.error(L"device address too high, can't possibly be a valid 32 bit address");
    return NULL;
  }

  init_buffers(handle->buffers,handle->device_address,buffer_count);

  //write buffer descriptors base
  result=write_busmaster_reg(handle,AC97_DESCRIPTOR_PCM_OUT,handle->device_address);
  ON_ERROR_RETURN(L"NABMBAR.POBAR Io.Write",NULL);

  _determine_maximum_master_volume(handle);

  return handle;
}

/**
 * Destroys an AC'97 handle.
 *
 * \param handle the AC'97 handle to destroy
 */
void close_ac97_handle(ac97_handle_t *handle)
{
  EFI_STATUS result;

  result=handle->pci->Unmap(handle->pci,handle->mapping);
  ON_ERROR_WARN(L"could not unmap AC'97 PCI memory");

  if(!free_pages(handle->buffers,handle->buffer_pages))
    LOG.warn(L"could not free AC'97 data buffer");
}

/**
 * Flushes all pending AC'97 memory writes.
 *
 * \param handle the AC'97 handle to use
 * \return the resulting status, EFI_SUCCESS if everything went well
 */
EFI_STATUS flush_ac97_output(ac97_handle_t *handle)
{
//  return handle->pci->Flush(handle->pci,handle->mapping);
  return handle->pci->Flush(handle->pci);
}

/**
 * Starts AC'97 playback.
 *
 * \param handle the AC'97 handle to use
 * \return the resulting status, EFI_SUCCESS if everything went well
 */
EFI_STATUS ac97_play(ac97_handle_t *handle)
{
  EFI_STATUS result;

  LOG.debug(L"starting playback...");

  result=write_busmaster_reg(handle,AC97_STATUS_PCM_OUT,0x1C);
  ON_ERROR_WARN(L"could not reset PCM OUT status flags");

  result=write_busmaster_reg(handle,AC97_CONTROL_PCM_OUT,0x15);
  ON_ERROR_RETURN(L"write_busmaster_reg",result);

  return result;
}

/**
 * internal: dumps a bus master status register's content
 *
 * \param name  the register's name to display
 * \param value the register's value
 */
static void _trace_busmaster_status_register(CHAR16 *name, ac97_busmaster_status_t value)
{
  LOG.trace(L"%s status: %04X (fifoe=%d, bcis=%d, lvbci=%d, celv=%d, dch=%d",name,value.raw,
      value.fifo_error,value.buffer_completion_interrupt,value.last_valid_buffer_completion_interrupt,
      value.current_equals_last_valid,value.dma_controller_halted);
}

/**
 * Waits until the AC'97 codec signaled the "last valid buffer completion" event.
 *
 * \param handle  the AC'97 handle to use
 * \param timeout the (approximate) number of milliseconds to wait before aborting
 */
void ac97_wait_until_last_buffer_sent(ac97_handle_t *handle, UINTN timeout)
{
  UINTN interval=30000; //microseconds
  UINTN timeout_iteration=(timeout*1000)/interval;
  UINTN tc;
  EFI_STATUS result;
  ac97_busmaster_status_t value;
  UINTN regval;

  for(tc=0;tc<timeout_iteration;tc++)
  {
    result=read_busmaster_reg(handle,AC97_STATUS_PCM_OUT,&regval);
    value.raw=regval;
    ON_ERROR_RETURN(L"could not read PCM OUT status register",);
    _trace_busmaster_status_register(L"PCM OUT",value);
    if(value.last_valid_buffer_completion_interrupt)
      break;
    if(gBS->Stall(interval)!=EFI_SUCCESS)
      return;
  }
}

/**
 * Prints a volume register's contents.
 *
 * \param text  the register's name to print
 * \param value the register's value
 */
void print_volume_register(UINT16 *text, UINT16 value)
{
  int mute=value&0x8000;
  int left=((value&0x3f00)>>8);
  int right=(value&0x3f);
  Print(L"%s=%04X: l=%d%%,r=%d%%%s\n",text,value,100-100*left/63,100-100*right/63,mute?L" (muted)":L"");
}

/**
 * Prints a mono volume register's contents.
 *
 * \param text  the register's name to print
 * \param value the register's value
 */
void print_volume_register_mono(UINT16 *text, UINT16 value)
{
  int mute=value&0x8000;
  int vol=value&0x3f;
  Print(L"%s=%04X: vol=%d%%%s\n",text,value,100-100*vol/63,mute?L" (muted)":L"");
}

/** internal: list of AC'97 revision ID strings */
static CHAR16 *AC97_REVISION_IDS[]={
  L"Revision 2.1 or earlier",
  L"Revision 2.2",
  L"Revision 2.3",
  L"Reserved"
};

/** internal: list of AC'97 code configuration ID string */
static CHAR16 *AC97_CODEC_CONFIGURATION_IDS[]={
  L"Primary",
  L"Secondary",
  L"Secondary",
  L"Secondary"
};

/**
 * Prints some of the AC'97 registers.
 *
 * \param handle the AC'97 handle to use
 * \param flags  which registers to print, see the AC97_DUMP_* flags
 */
void dump_audio_registers(ac97_handle_t *handle, UINTN flags)
{
  EFI_STATUS result;
  ac97_bar_t bar;

  Print(L"audio device:\n");

  result=handle->pci->Io.Read(handle->pci,EfiPciIoWidthUint16,0,0,sizeof(ac97_bar_t)/2,&bar);
  ON_ERROR_RETURN(L"Io.Read",);

  if(flags&AC97_DUMP_OTHER)
    Print(L"  reset=%04X\n",bar.reset);
  if(flags&AC97_DUMP_VOLUME)
  {
    print_volume_register(L"  master_vol",bar.master_vol);
    print_volume_register(L"  aux_out_vol",bar.aux_out_vol);
    print_volume_register_mono(L"  mono_vol",bar.mono_vol);
    Print(L"  master_tone=%04X\n",bar.master_tone);
    Print(L"  pc_beep_vol=%04X\n",bar.pc_beep_vol);
    Print(L"  phone_vol=%04X\n",bar.phone_vol);
    Print(L"  mic_vol=%04X\n",bar.mic_vol);
    Print(L"  line_in_vol=%04X\n",bar.line_in_vol);
    Print(L"  cd_vol=%04X\n",bar.cd_vol);
    Print(L"  video_vol=%04X\n",bar.video_vol);
    Print(L"  aux_in_vol=%04X\n",bar.aux_in_vol);
    Print(L"  pcm_out_vol=%04X\n",bar.pcm_out_vol);
  }
  if(flags&AC97_DUMP_OTHER)
  {
    Print(L"  general_purpose=%04X\n",bar.general_purpose);
    Print(L"  vendor_id=");
    AsciiPrint((CHAR8 *)"%c%c%c",bar.vendor_id1>>8,bar.vendor_id1&0xFF,bar.vendor_id2>>8);
    Print(L", device_id=%02X (raw: %04X%04X)\n",bar.vendor_id2&0xFF,bar.vendor_id1,bar.vendor_id2);
    Print(L"  extended_audio_id=%04X\n",bar.extended_audio_id.raw,4);
    Print(L"    VRA (Variable Rate Audio) support: %d\n",           bar.extended_audio_id.vra);
    Print(L"    DRA (Double-Rate Audio) support: %d\n",             bar.extended_audio_id.dra);
    Print(L"    SPDIF support: %d\n",                               bar.extended_audio_id.spdif);
    Print(L"    VRM (VRA for Mic) support: %d\n",                   bar.extended_audio_id.vrm);
    Print(L"    DSA (DAC Slot Assignment): %d\n",                   bar.extended_audio_id.dsa);
    Print(L"    CDAC (Center DAC) support: %d\n",                   bar.extended_audio_id.cdac);
    Print(L"    SDAC (Surround DAC) support: %d\n",                 bar.extended_audio_id.sdac);
    Print(L"    LDAC (LFE DAC) support: %d\n",                      bar.extended_audio_id.ldac);
    Print(L"    AMAP (slot/DAC mappings by codec id) support: %d\n",bar.extended_audio_id.amap);
    Print(L"    REV (Revision): %d (%s)\n",                         bar.extended_audio_id.rev,AC97_REVISION_IDS[bar.extended_audio_id.rev]);
    Print(L"    ID: %d\n",                                          bar.extended_audio_id.id,AC97_CODEC_CONFIGURATION_IDS[bar.extended_audio_id.id]);
    Print(L"  extended_audio_statctrl=%04X\n",bar.extended_audio_statctrl.raw);
    Print(L"    VRA (Variable Rate Audio) enabled: %d\n",bar.extended_audio_statctrl.vra);
    Print(L"    DRA (Double-Rate Audio) enabled: %d\n",  bar.extended_audio_statctrl.dra);
    Print(L"    SPDIF enabled: %d\n",                    bar.extended_audio_statctrl.spdif);
    Print(L"    VRM (VRA for Mic) enabled: %d\n",        bar.extended_audio_statctrl.vrm);
    Print(L"    SPSA (AC-link Slot Assignment): %d\n",   bar.extended_audio_statctrl.spsa);
    Print(L"    CDAC (Center DAC) ready: %d\n",          bar.extended_audio_statctrl.cdac);
    Print(L"    SDAC (Surround DAC) ready: %d\n",        bar.extended_audio_statctrl.sdac);
    Print(L"    LDAC (LFE DAC) ready: %d\n",             bar.extended_audio_statctrl.ldac);
    Print(L"    MADC (Mic ADC ready) ready: %d\n",       bar.extended_audio_statctrl.madc);
    Print(L"    SPCV (current SPDIF config) valid: %d\n",bar.extended_audio_statctrl.spcv);
    Print(L"    PRI (Center DAC powerdown): %d\n",       bar.extended_audio_statctrl.pri);
    Print(L"    PRJ (Surround DAC powerdown): %d\n",     bar.extended_audio_statctrl.prj);
    Print(L"    PRK (LFE DAC powerdown): %d\n",          bar.extended_audio_statctrl.prk);
    Print(L"    PRL (Mic ADC powerdown): %d\n",          bar.extended_audio_statctrl.prl);
    Print(L"    VCFG (SPDIF validity config): %d\n",     bar.extended_audio_statctrl.vcfg);
    Print(L"  PCM Front DAC Rate: %dHz\n",bar.pcm_front_dac_rate);
    Print(L"  PCM Surround DAC Rate: %dHz\n",bar.pcm_surr_dac_rate);
    Print(L"  PCM LFE DAC Rate: %dHz\n",bar.pcm_lfe_dac_rate);
    Print(L"  PCM L/R ADC Rate: %dHz\n",bar.pcm_lr_adc_rate);
    Print(L"  Mic ADC Rate: %dHz\n",bar.pcm_mic_adc_rate);
  }
}
