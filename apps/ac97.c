/** \file
 * AC'97 audio demo
 *
 * \author Richard Nusser
 * \copyright 2017-2018 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_apps
 *
 * \warning This application accesses your audio hardware directly, take care about the sound volume in particular.
 *          A volume of 100% (the maximum) will set your master and PCM OUT volumes to 100%. If you're running this on
 *          hardware directly (because you e.g. booted a computer into this environment), this means the sound volume
 *          will be as high as it can possibly go (apart from vendor-specific additional volume boosts). Depending on
 *          your audio setup this may even cause damage to your amplifier, speakers or hearing: reduce any attached
 *          devices' (e.g. speakers') volumes before starting this application. If you're running this in a virtual
 *          environment, e.g. VirtualBox, this will output at 100% of the hypervisor's volume.
 */

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/ShellCEntryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include "../include/cmdline.h"
#include "../include/console.h"
#include "../include/ac97.h"
#include "../include/pci.h"
#include "../include/memory.h"
#include "../include/logger.h"


/**
 * Number of samples to use in each buffer.
 * Determines length of individual notes (thus speed of playback). Keep this value below 32767.
 */
#define SAMPLES_PER_BUFFER 10000


/**
 * Shortcut macro to sample a (non-harmonic) frequency.
 *
 * \param FREQ the frequency to use (of no particular unit)
 * \return the generated sample
 */
#define SAMPLES_FREQ(FREQ) ((tc%(FREQ))*60000/(FREQ)-30000)

/**
 * Fills audio buffers with scales, one channel going down, the other channel going up.
 * Does not use the harmonic scale: this will sound bad.
 *
 * \param buffers      the AC'97 buffer list to fill
 * \param start_buffer index of the first buffer to fill
 * \param buffer_count the number of buffers to fill
 * \param loop_offset  offset for loop counter, the higher this is the higher the generated notes will be
 */
void fill_buffers_crossscale(ac97_buffers_s16_t *buffers, unsigned int start_buffer, unsigned int buffer_count, unsigned int loop_offset)
{
  unsigned int tc, td;

  LOG.debug(L"filling buffers with (nonharmonic) scale: start=%d, count=%d, offset=%d",start_buffer,buffer_count,loop_offset);

  for(td=0;td<buffer_count;td++)
  {
    buffers->descriptors[(start_buffer+td)%32].length=SAMPLES_PER_BUFFER*2;
    for(tc=0;tc<SAMPLES_PER_BUFFER;tc++)
    {
      buffers->buffers[(start_buffer+td)%32][tc*2]=SAMPLES_FREQ(31+(td+loop_offset)*3);
      buffers->buffers[(start_buffer+td)%32][tc*2+1]=SAMPLES_FREQ(128-(td+loop_offset)*3);
    }
  }
}


/**
 * Fills audio buffers with harmonic scales.
 *
 * \param buffers      the audio buffers to write to
 * \param start_buffer index of the first buffer to fill
 * \param buffer_count number of buffers to fill
 */
void fill_buffers_harmonic_scale(ac97_buffers_s16_t *buffers, unsigned int start_buffer, unsigned int buffer_count)
{
  unsigned int tc, td;
  float a3=ARG_SAMPLE_RATE/440.0;
  float val_left, val_right;
  float attack_samples=500;

  float harmonic_scale[]={
    1.0,
    1.12246204830937,
    1.25992104989487,
    1.33483985417003,
    1.49830707687668,
    1.68179283050743,
    1.88774862536339,
    2.0
  };

  LOG.debug(L"filling buffers with harmonic scale: start=%d, count=%d",start_buffer,buffer_count);

  if(buffer_count>32)
  {
    LOG.error(L"buffer count too high (max 32, got %d), would wrap around and overwrite start of data",buffer_count);
    return;
  }

  for(td=0;td<buffer_count;td++)
  {
    buffers->descriptors[(start_buffer+td)%32].length=SAMPLES_PER_BUFFER*2;
    for(tc=0;tc<SAMPLES_PER_BUFFER;tc++)
    {
      val_left=tc/a3;
      if((start_buffer+td)%16<8)
      {
        val_right=val_left/2.0;
      }
      else
      {
        val_right=val_left;
        val_left/=2.0;
      }
      if((start_buffer+td)%16>7)
        val_left*=harmonic_scale[7-(start_buffer+td)%8];
      else
        val_left*=harmonic_scale[(start_buffer+td)%8];
      val_left-=(int)val_left;
      val_left-=0.5f;
      val_left*=30000;
      if(tc<attack_samples)
        val_left*=((float)tc)/attack_samples;
      buffers->buffers[(start_buffer+td)%32][tc*2]=val_left;

      if((start_buffer+td)%16>7)
        val_right*=harmonic_scale[7-(start_buffer+td)%8];
      else
        val_right*=harmonic_scale[(start_buffer+td)%8];
      val_right-=(int)val_right;
      val_right-=0.5f;
      val_right*=30000;
      if(tc<attack_samples)
        val_right*=((float)tc)/attack_samples;
      buffers->buffers[(start_buffer+td)%32][tc*2+1]=val_right;
    }
  }
}


/**
 * Fills audio buffers with scales, copies buffers to AC'97 device and starts playback.
 * This is pretty much how you'd output prepared audio, e.g. when playing music.
 *
 * Start the application with `-trace` to see when playback switches to the next buffer.
 *
 * \param handle the AC'97 handle to use
 */
void output_audio(ac97_handle_t *handle)
{
  EFI_STATUS result;

  fill_buffers_harmonic_scale(handle->buffers,0,32);

  result=flush_ac97_output(handle);
  ON_ERROR_RETURN(L"flush_ac97_output",);

  //write last valid index
  result=write_busmaster_reg(handle,AC97_LVI_PCM_OUT,31);
  ON_ERROR_RETURN(L"write_busmaster_reg",);

  //start playback
  if(ac97_play(handle)!=EFI_SUCCESS)
    return;

  result=handle->pci->Unmap(handle->pci,handle->mapping);
  ON_ERROR_RETURN(L"handle->pci->Unmap",);

  LOG.info(L"starting playback...%s",ARG_MUTE?L" (muted)":L"");
}


/**
 * This fills audio buffers while they're being played.
 * This is how you'd output audio on the fly, e.g. sound effects that depend on user inputs.
 *
 * \param handle the AC'97 handle to use
 *
 * \TODO increase timing granularity: actual length should be somewhere between 150ms and 200ms.
 */
void loop_civ(ac97_handle_t *handle)
{
  unsigned int tc, td;
  EFI_STATUS result;
  UINTN last=0xff;
  UINTN value=0, value2=0xff;
  EFI_EVENT event;
  UINTN index;
  UINT8 volume_left, volume_right;

  result=gST->BootServices->CreateEvent(EVT_TIMER,TPL_CALLBACK,NULL,NULL,&event);
  ON_ERROR_RETURN(L"CreateEvent",);
  result=gST->BootServices->SetTimer(event,TimerPeriodic,50*1000*10); //50ms
  ON_ERROR_RETURN(L"SetTimer",);

  for(tc=0;tc<64;tc++)
  {
    for(td=0;td<20;td++)
    {
      result=gST->BootServices->WaitForEvent(1,&event,&index);
      ON_ERROR_RETURN(L"WaitForEvent",);

      result=read_busmaster_reg(handle,AC97_CIV_PCM_OUT,&value);
      ON_ERROR_RETURN(L"read_busmaster_reg",);
      if(last==value)
        continue;
      LOG.trace(L"civ=%02d (%3dms)",value,td*50);

      if(value==31 && tc<40)
      {
        fill_buffers_crossscale(handle->buffers,0,16,0);
        value2=0;
      }
      else if(value==0 && tc>1 && tc<40)
      {
        fill_buffers_crossscale(handle->buffers,16,16,16);
        value2=31;
      }

      if(tc==16)
        LOG.debug(L"starting master volume panning");

      if(tc>=16 && tc<31)
      {
        volume_left=(value-16)*4+3;
        volume_right=66-volume_left;
        if(handle->max_master_vol<63)
        {
          volume_left/=2;
          volume_right/=2;
        }
        result=write_mixer_reg(handle,AC97_MIXER_MASTER,ac97_mixer_value(volume_left,volume_right,ARG_MUTE));
        ON_ERROR_RETURN(L"  write_mixer_reg",);
        LOG.trace(L"wrote master volume values: left=%02d, right=%02d",volume_left,volume_right);
      }
      else if(tc==31)
      {
        write_mixer_reg(handle,AC97_MIXER_MASTER,ac97_mixer_value(8,8,ARG_MUTE));
        LOG.debug(L"reset master volume");
      }

      if(value2<32)
      {
        result=write_busmaster_reg(handle,AC97_LVI_PCM_OUT,value2);
        ON_ERROR_RETURN(L"  write_busmaster_reg",);
        LOG.debug(L"wrote %d to PCM OUT LVI",value2);
        value2=0xff;
      }
      last=value;
      break;
    }
  }
  Print(L"Press any key to continue...\n");
  wait_for_key();
}

/**
 * Handles entire lifetime of AC'97 audio output.
 * This shows the high-level interface to the AC'97 library: find the audio device, initialize the handle, configure
 * audio output, fill buffers, start playback and finally close the handle.
 *
 * \param audio the UEFI PCI I/O protocol for the AC'97 chipset
 * \return the resulting status, EFI_SUCCESS if everything went OK
 */
EFI_STATUS run_audio_stuff(EFI_PCI_IO_PROTOCOL *audio)
{
  EFI_STATUS result;
  ac97_handle_t handle;

  if(!init_ac97_handle(&handle,audio))
  {
    LOG.error(L"could not initialize output handle");
    return EFI_UNSUPPORTED;
  }

  result=set_ac97_cmdline_sample_rate(&handle);
  ON_ERROR_WARN(L"could not set sample rate");

  result=set_ac97_cmdline_volume(&handle);
  ON_ERROR_WARN(L"could not set volume");

  dump_audio_registers(&handle,AC97_DUMP_ALL);

  output_audio(&handle);
  loop_civ(&handle);
  close_ac97_handle(&handle);
  return EFI_SUCCESS;
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
  EFI_PCI_IO_PROTOCOL *audio;
  EFI_STATUS rv;
  if((rv=init(argc,argv,1,&ac97_arguments))!=EFI_SUCCESS)
    return rv;
  init_pci_lib();

  audio=find_ac97_device();
  if(audio)
    rv=run_audio_stuff(audio);
  else
  {
    LOG.error(L"did not find AC97 device");
    rv=EFI_UNSUPPORTED;
  }

  shutdown_pci_lib();
  shutdown();
  return rv;
}
