/** \file
 * AC'97 audio functions
 *
 * Check these documents for details on how AC'97 works (they're both available from Intel):
 * * Audio Codec '97 specification
 * * Intel I/O Controller Hub 6 (ICH6) High Definition Audio / AC '97 Programmer's Reference Manual
 *
 * \author Richard Nusser
 * \copyright 2017-2018 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_lib_ac97
 *
 * \TODO maybe combine find and init functions
 */

#ifndef __AC97_H
#define __AC97_H

#include <UEFIStarter/pci.h>
#include <UEFIStarter/core/cmdline.h>


#define AC97_BUFFER_COUNT 32 /**< number of audio data buffers, up to 32 supported by AC'97 specs */


#define ARG_MUTE   ac97_argument_list[0].value.uint64      /**< shortcut macro to access "mute" argument */
#define ARG_VOLUME ac97_argument_list[1].value.dbl         /**< shortcut macro to access "volume" argument */
#define ARG_SAMPLE_RATE ac97_argument_list[2].value.uint64 /**< shortcut macro to access "sample rate" argument */
extern cmdline_argument_t ac97_argument_list[];
extern cmdline_argument_group_t ac97_arguments;


#define AC97_MIXER_RESET       0x00 /**< "reset" mixer register */
#define AC97_MIXER_MASTER      0x02 /**< "master volume" mixer register */
#define AC97_MIXER_PCM_OUT     0x18 /**< "PCM OUT volume" mixer register */
#define AC97_PCM_RATE_FRONT    0x2C /**< "PCM front channel DAC sample rate" mixer register */
#define AC97_PCM_RATE_SURROUND 0x2E /**< "PCM surround channel DAC sample rate" mixer register */
#define AC97_PCM_RATE_LFE      0x30 /**< "PCM LFE channel DAC sample rate" mixer register */


//XXX when adding bus master registers, remember to add non-byte widths to getter function
#define AC97_DESCRIPTOR_PCM_OUT 0x10 /**< "PCM OUT descriptor base address" bus master register */
#define AC97_CIV_PCM_OUT        0x14 /**< "PCM OUT current index value" bus master register */
#define AC97_LVI_PCM_OUT        0x15 /**< "PCM OUT last valid index" bus master register */
#define AC97_STATUS_PCM_OUT     0x16 /**< "PCM OUT status" bus master register */
#define AC97_CONTROL_PCM_OUT    0x1B /**< "PCM OUT control" bus master register */
#define AC97_GLOBAL_CONTROL     0x2C /**< "global control" bus master register */


/**
 * Data type for an AC'97 "baseline audio register set".
 * The AC'97 specification contains a description of these registers.
 */
typedef struct
{
  UINT16 reset;              /**< offset 0x00 */
  UINT16 master_vol;         /**< offset 0x02 */
  UINT16 aux_out_vol;        /**< offset 0x04 */
  UINT16 mono_vol;           /**< offset 0x06 */
  UINT16 master_tone;        /**< offset 0x08 */
  UINT16 pc_beep_vol;        /**< offset 0x0A */
  UINT16 phone_vol;          /**< offset 0x0C */
  UINT16 mic_vol;            /**< offset 0x0E */
  UINT16 line_in_vol;        /**< offset 0x10 */
  UINT16 cd_vol;             /**< offset 0x12 */
  UINT16 video_vol;          /**< offset 0x14 */
  UINT16 aux_in_vol;         /**< offset 0x16 */
  UINT16 pcm_out_vol;        /**< offset 0x18 */
  UINT16 record_select;      /**< offset 0x1A */
  UINT16 record_gain;        /**< offset 0x1C */
  UINT16 record_gain_mic;    /**< offset 0x1E */
  UINT16 general_purpose;    /**< offset 0x20 */
  UINT16 three_d_control;    /**< offset 0x22 */
  UINT16 _reserved24;        /**< offset 0x24 */
  UINT16 powerdown_ctrlstat; /**< offset 0x26 */
  union
  {
    UINT16 raw; /**< raw access */
    struct
    {
      UINT16 vra:1;       /**< variable rate PCM audio support */
      UINT16 dra:1;       /**< double-rate PCM audio support */
      UINT16 spdif:1;     /**< S/PDIF support */
      UINT16 vrm:1;       /**< variable rate MIC input support */
      UINT16 dsa:2;       /**< DAC slot assignments */
      UINT16 cdac:1;      /**< PCM center DAC support */
      UINT16 sdac:1;      /**< PCM surround DAC support */
      UINT16 ldac:1;      /**< PCM LFE DAC support */
      UINT16 amap:1;      /**< slot/DAC mappings support */
      UINT16 rev:2;       /**< codec revision */
      UINT16 _reserved:2; /**< (reserved) */
      UINT16 id:2;        /**< codec configuration */
    }; /**< structured access */
  } extended_audio_id;       /**< offset 0x28 */
  union
  {
    UINT16 raw; /**< raw access */
    struct
    {
      UINT16 vra:1;   /**< VRA enabled */
      UINT16 dra:1;   /**< DRA enabled */
      UINT16 spdif:1; /**< S/PDIF enabled */
      UINT16 vrm:1;   /**< VRM enabled */
      UINT16 spsa:2;  /**< S/PDIF source */
      UINT16 cdac:1;  /**< PCM center DAC ready */
      UINT16 sdac:1;  /**< PCM surround DACs ready */
      UINT16 ldac:1;  /**< PCM LFE DAC ready */
      UINT16 madc:1;  /**< MIC ADC ready */
      UINT16 spcv:1;  /**< S/PDIF configuration valid */
      UINT16 pri:1;   /**< PCM center DAC suppressed */
      UINT16 prj:1;   /**< PCM surround DACs suppressed */
      UINT16 prk:1;   /**< PCM LFE DACs suppressed */
      UINT16 prl:1;   /**< MIC ADC suppressed */
      UINT16 vcfg:1;  /**< S/PDIF idle configuration */
    };  /**< structured access */
  } extended_audio_statctrl; /**< offset 0x2A */
  UINT16 pcm_front_dac_rate; /**< offset 0x2C */
  UINT16 pcm_surr_dac_rate;  /**< offset 0x2E */
  UINT16 pcm_lfe_dac_rate;   /**< offset 0x30 */
  UINT16 pcm_lr_adc_rate;    /**< offset 0x32 */
  UINT16 pcm_mic_adc_rate;   /**< offset 0x34 */
  UINT16 _unhandled3[5];     /**< offset 0x36 */
  UINT16 _unhandled4[8];     /**< offset 0x40 */
  UINT16 _unhandled5[8];     /**< offset 0x50 */
  UINT16 _unhandled6[8];     /**< offset 0x60 */
  UINT16 _unhandled7[6];     /**< offset 0x70 */
  UINT16 vendor_id1;         /**< offset 0x7C */
  UINT16 vendor_id2;         /**< offset 0x7E */
} ac97_bar_t;

/** data type for AC'97 buffer descriptor */
typedef struct
{
  UINT32 address; /**< start of buffer */
  UINT16 length;  /**< length of buffer, in bytes */
  union
  {
    UINT16 raw; /**< raw access to buffer configuration */
    struct
    {
      UINT16 _reserved:14; /**< (reserved) */
      UINT16 bup:1; /**< buffer underrun policy */
      UINT16 ioc:1; /**< interrupt on completion */
    }; /**< structured access to buffer configuration */
  } control; /**< buffer configuration */
} ac97_buffer_descriptor_t;

/**
 * data type for signed 16-bit integer audio buffers descriptor
 *
 * \TODO ac97_buffers_s16_t.buffers doesn't need to be DMA transferred, change this to e.g. an output of init_buffers()
 */
typedef struct
{
  ac97_buffer_descriptor_t descriptors[32]; /**< the list of buffer descriptors */
  INT16 *buffers[32];                       /**< pointers to buffer contents */
} ac97_buffers_s16_t;

/** AC'97 handle, this is the high-level handle for library use */
typedef struct
{
  ac97_buffers_s16_t *buffers;         /**< ring buffer for audio output */
  UINTN buffer_pages;                  /**< number of memory pages allocated for audio buffers */
  EFI_PHYSICAL_ADDRESS device_address; /**< physical memory address to access the AC'97 codec */
  void *mapping;                       /**< DMA memory mapping for transferring audio data to AC'97 */
  EFI_PCI_IO_PROTOCOL *pci;            /**< UEFI PCI handle to use */
  UINT8 max_master_vol;                /**< maximum master volume, either 31 or 63, depending on hardware */
} ac97_handle_t;

/** data type for bus master status register */
typedef union
{
  UINT16 raw; /**< raw access */
  struct
  {
    UINT16 dma_controller_halted:1;                  /**< DCH */
    UINT16 current_equals_last_valid:1;              /**< CELV */
    UINT16 last_valid_buffer_completion_interrupt:1; /**< LVBCI */
    UINT16 buffer_completion_interrupt:1;            /**< BCIS */
    UINT16 fifo_error:1;                             /**< FIFOE */
    UINT16 _reserved:11;                             /**< (reserved) */
  }; /**< structured access */
} ac97_busmaster_status_t;


EFI_PCI_IO_PROTOCOL *find_ac97_device();
void *init_ac97_handle(ac97_handle_t *handle, EFI_PCI_IO_PROTOCOL *pip);
void close_ac97_handle(ac97_handle_t *handle);


/**
 * Generates an AC'97 mixer value.
 *
 * \param LEFT  the left channel's value, within [0..63]
 * \param RIGHT the right channel's value, within [0..63]
 * \param MUTE  whether to mute (0 for no, anything else for yes)
 * \return the mixer value, ready to write into an AC'97 mixer register
 */
#define ac97_mixer_value(LEFT,RIGHT,MUTE) ((((LEFT)&0x3F)<<8)|((RIGHT)&0x3F)|((MUTE)?0x8000:0))

EFI_STATUS write_mixer_reg(ac97_handle_t *handle, UINT32 reg, UINT16 value);
EFI_STATUS read_mixer_reg(ac97_handle_t *handle, UINT32 reg, UINT16 *value);
void print_volume_register(UINT16 *text, UINT16 value);
void print_volume_register_mono(UINT16 *text, UINT16 value);
EFI_STATUS set_ac97_cmdline_volume(ac97_handle_t *handle);
EFI_STATUS set_ac97_cmdline_sample_rate(ac97_handle_t *handle);

EFI_STATUS write_busmaster_reg(ac97_handle_t *handle, UINT32 reg, UINTN value);
EFI_STATUS read_busmaster_reg(ac97_handle_t *handle, UINT32 reg, UINTN *value);

EFI_STATUS flush_ac97_output(ac97_handle_t *handle);
EFI_STATUS ac97_play(ac97_handle_t *handle);
void ac97_wait_until_last_buffer_sent(ac97_handle_t *handle, UINTN timeout_in_milliseconds);


#define AC97_DUMP_VOLUME  0x00000001 /**< flag for dump_audio_registers(): dump volume registers */
#define AC97_DUMP_OTHER   0x80000000 /**< flag for dump_audio_registers(): dump other registers */
#define AC97_DUMP_ALL     -1         /**< flag for dump_audio_registers(): dump all registers */
void dump_audio_registers(ac97_handle_t *handle, UINTN flags);


#endif
