/** \file
 * This application showcases a few ways to use the graphics output.
 * There are a few command-line options you can use to skip individual demos or change graphics settings.
 *
 * You'll need to run this application in a graphics-capable UEFI environment. Virtual machines created with the
 * Vagrantfile included in this repository don't support graphics: the application will still run and expect a few
 * keystrokes but you'll just see a text hint on the console.
 *
 * If you have QEMU installed on a system with e.g. the X11 server (or are using it on Windows) you can execute the
 * target image (target/\*.img) there - see the Makefile's "run" target on how to do that.
 *
 * If you're using VirtualBox the easiest way to run this application is probably by creating a new VM (no harddisk
 * required), enabling EFI (Machine Settings / System / Enable EFI) and mounting the generated ISO image
 * (target/\*.iso). Booting this VM should take you to the UEFI shell as usual.
 *
 * \author Richard Nusser
 * \copyright 2017-2018 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_apps
 */

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/ShellCEntryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include "../include/cmdline.h"
#include "../include/console.h"
#include "../include/graphics.h"
#include "../include/console.h"
#include "../include/cmdline.h"
#include "../include/memory.h"
#include "../include/logger.h"
#include "../include/timestamp.h"
#include "../include/string.h"


#define ARG_SKIP_BARS     _argument_list[0].value.uint64 /**< helper macro to access the "-skip-bars" command-line argument */
#define ARG_SKIP_IMAGES   _argument_list[1].value.uint64 /**< helper macro to access the "-skip-images" command-line argument */
#define ARG_SKIP_FONT     _argument_list[2].value.uint64 /**< helper macro to access the "-skip-font" command-line argument */
#define ARG_SKIP_OBJECTS  _argument_list[3].value.uint64 /**< helper macro to access the "-skip-objects" command-line argument */
#define ARG_SKIP_ANIM     _argument_list[4].value.uint64 /**< helper macro to access the "-skip-anim" command-line argument */

/** list of command-line arguments */
static cmdline_argument_t _argument_list[] = {
  {{uint64:0},ARG_BOOL,NULL,L"-skip-bars",   L"Skip bars test"},
  {{uint64:0},ARG_BOOL,NULL,L"-skip-images", L"Skip images test"},
  {{uint64:0},ARG_BOOL,NULL,L"-skip-font",   L"Skip font test"},
  {{uint64:0},ARG_BOOL,NULL,L"-skip-objects",L"Skip moving objects test"},
  {{uint64:0},ARG_BOOL,NULL,L"-skip-anim",   L"Skip animation test"},
};

/** command-line arguments group */
static ARG_GROUP(_arguments,_argument_list,L"Application-specific options");


/**
 * Produces a triangular waveform going from 0 to 255 and back to 0 again.
 *
 * \param val the position in the triangle function
 * \return the sample at the given position
 */
UINT8 ramp(UINTN val)
{
  int rv;
  rv=val%256;
  if(val%512>=256)
    rv-=2*(val%256)+1;
  return (rv+256)%256;
}

/**
 * Draws a single frame of the progress bar.
 *
 * \param gop      the UEFI graphics output protocol to draw with
 * \param screen_w the screen's width
 * \param screen_h the screen's height
 * \param value    the progress to draw, within [0..1]
 */
void draw_progress_bar(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, unsigned int screen_w, unsigned int screen_h, float value)
{
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL color;
  color.Red=255;
  color.Blue=0;
  color.Green=128;
  color.Reserved=0;
  draw_filled_rect(gop,screen_w/2-100,screen_h/2-20,200,40,&color);
  if(value>=0.005)
  {
    color.Green=255;
    draw_filled_rect(gop,screen_w/2-98,screen_h/2-18,200*value,36,&color);
  }
}


/**
 * Draws an animation with objects moving across the screen.
 * This also limits the frame rate (and waits for vsync if enabled in the command line) to reduce flickering.
 *
 * \param gop the UEFI graphics protocol to draw with
 */
void draw_moving_objects(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop)
{
  EFI_STATUS result;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *buffer;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *sprite;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL bg;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL fg;
  UINT32 col32;
  unsigned int x, y, tc;
  unsigned int limit;

  unsigned int width=gop->Mode->Info->HorizontalResolution;
  unsigned int height=gop->Mode->Info->VerticalResolution;
  unsigned int pages=width*height*sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL)/4096+1;

  UINT64 previous_ts;
  UINT64 minimum_frame_ticks;

  if((buffer=allocate_pages(pages))==NULL)
    return;
  if((sprite=allocate_pages(4))==NULL)
    return;

  SetMem(buffer,pages*4096,0);
  SetMem(sprite,4*4096,0);

  bg.Red=0;
  bg.Blue=0;
  bg.Green=0;
  bg.Reserved=0;

  result=gop->Blt(gop,&bg,EfiBltVideoFill,0,0,0,0,width,height,0);
  ON_ERROR_RETURN(L"gop->Blt",);

  init_timestamps();
  previous_ts=get_timestamp();
  minimum_frame_ticks=get_timestamp_ticks_per_second()/ARG_FPS;

  fg.Red=255;
  fg.Blue=255;
  fg.Green=255;
  fg.Reserved=0;

  col32=*((UINT32*)&fg);
  for(y=10;y<54;y++)
    for(x=10;x<54;x++)
      *((UINT32 *)sprite+y*64+x)=col32;

  limit=width>height?height:width;
  limit-=64;
  LOG.debug(L"limit: %d",limit);
  for(tc=0;tc<limit;tc++)
  {
    limit_framerate(&previous_ts,minimum_frame_ticks);
    result=gop->Blt(gop,sprite,EfiBltBufferToVideo,0,0,tc,tc,64,64,0);
    ON_ERROR_RETURN(L"gop->Blt",);
    result=gop->Blt(gop,sprite,EfiBltBufferToVideo,0,0,tc+100,tc,64,64,0);
    ON_ERROR_RETURN(L"gop->Blt",);
    result=gop->Blt(gop,sprite,EfiBltBufferToVideo,0,0,tc,height-64-tc,64,64,0);
    ON_ERROR_RETURN(L"gop->Blt",);
    result=gop->Blt(gop,sprite,EfiBltBufferToVideo,0,0,tc+100,height-64-tc,64,64,0);
    ON_ERROR_RETURN(L"gop->Blt",);
  }

  free_pages(sprite,4);
  free_pages(buffer,pages);
}

/**
 * Draws an animated progress bar, then a full-screen animation.
 * This actually prepares a background buffer that's twice the screen height and then scrolls down to simulate movement.
 *
 * \param gop the UEFI graphics protocol to draw with
 */
void draw_prepared_fs_anim(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop)
{
  EFI_STATUS result;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *buffer;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *current;
  unsigned int x, y, tc;
  unsigned int val;

  unsigned int width=gop->Mode->Info->HorizontalResolution;
  unsigned int height=gop->Mode->Info->VerticalResolution;
  unsigned int buffer_height=height+512;
  unsigned int r, g, b;
  int count=1000;
  unsigned int pages;
  unsigned int progress_bar_interval=width/10;

  UINT64 times[4];
  UINT64 previous_ts;
  UINT64 minimum_frame_ticks;

  unsigned int buffer_size_bytes=width*buffer_height*sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL);

  pages=buffer_size_bytes/4096+1;
  if((buffer=allocate_pages(pages))==NULL)
    return;

  draw_progress_bar(gop,width,height,0.0);
  init_timestamps();
  draw_progress_bar(gop,width,height,0.1);
  minimum_frame_ticks=get_timestamp_ticks_per_second()/ARG_FPS;

  times[0]=get_timestamp();
  SetMem(buffer,buffer_size_bytes,0);

  times[1]=get_timestamp();
  for(x=0;x<width;x++)
  {
    if((x%progress_bar_interval)==0)
      draw_progress_bar(gop,width,height,0.2+(float)x/width*0.8);
    for(y=0;y<buffer_height;y++)
    {
      val=y*2+x;
      r=ramp(val);
      g=ramp(val+25+y);
      b=ramp(val+50-y);

      current=buffer+y*width+x;
      current->Red=r;
      current->Green=g;
      current->Blue=b;
    }
  }
  times[2]=get_timestamp();

  previous_ts=times[0];
  for(tc=0;tc<count;tc++)
  {
    limit_framerate(&previous_ts,minimum_frame_ticks);
    result=gop->Blt(gop,buffer+(tc%512)*width,EfiBltBufferToVideo,0,0,0,0,width,height,0);
    ON_ERROR_RETURN(L"gop->Blt",);
  }

  times[3]=get_timestamp();

  double prepare_time=timestamp_diff_seconds(times[1],times[2]);
  double run_time=timestamp_diff_seconds(times[2],times[3]);
  gST->ConOut->SetCursorPosition(gST->ConOut,0,0);
  Print(L"took %ss to prepare image, %ss to run %d frames (%s fps)\n",ftowcs(prepare_time),ftowcs(run_time),count,ftowcs(count/run_time));

  free_pages(buffer,pages);
}


/** data type for image parser function pointers */
typedef image_t *image_parser_f(file_contents_t *);

/**
 * Loads an image and then draws it to the screen.
 *
 * \TODO remove parser function pointer and use newer load_netpbm_file() instead
 *
 * \param gop      the UEFI graphics protocol to draw with
 * \param filename the image's filename
 * \param parser   the image parser function
 */
void draw_image(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, CHAR16 *filename, image_parser_f parser)
{
  file_contents_t *contents;
  image_t *image;
  EFI_STATUS result;

  contents=get_file_contents(filename);
  if(!contents)
    return;

  image=parser(contents);
  if(!free_pages(contents,contents->memory_pages))
    return;
  if(!image)
    return;

  result=gop->Blt(gop,image->data,EfiBltBufferToVideo,0,0,0,0,image->width,image->height,0);
  ON_ERROR_RETURN(L"gop->Blt",);
  free_pages(image,image->memory_pages);
}

/**
 * Draws a few vertical bars onto the screen.
 *
 * \param gop the UEFI graphics protocol to draw with
 */
void draw_bars(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop)
{
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL pixel;
  unsigned int tc;
  unsigned int width=gop->Mode->Info->HorizontalResolution;
  unsigned int height=gop->Mode->Info->VerticalResolution;
  unsigned int hspan, barwidth, barheight, barheight_step, left, top;

  hspan=width/130;
  barwidth=width/72;
  left=(width-38*barwidth-37*hspan)/2;
  barheight=height/5*3;
  barheight_step=barheight/60;
  top=(height-barheight)/2;

  pixel.Red=0;
  pixel.Blue=0;
  pixel.Green=0;
  pixel.Reserved=0;
  draw_filled_rect(gop,0,0,width,height,&pixel);

  for(tc=0;tc<38;tc++)
  {
    pixel.Red=tc*6;
    pixel.Green=222-tc*6;
    pixel.Blue=0;
    draw_filled_rect(gop,left+(barwidth+hspan)*tc,top+tc*barheight_step,barwidth,barheight-tc*barheight_step,&pixel);
  }
}


/** shortcut macro to access a CPU register */
#define REG(NAME) register long NAME asm (#NAME);

/**
 * Draws white text on a blue background.
 *
 * \param gop the UEFI graphics protocol to draw with
 */
void draw_font(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop)
{
  EFI_STATUS result;
  unsigned int width=gop->Mode->Info->HorizontalResolution;
  unsigned int height=gop->Mode->Info->VerticalResolution;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL bg, fg;
  CHAR16 *outtext;
  glyph_list_t *glyphs;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *buffer;
  unsigned int buffer_pages=width*height*sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL)/4096+1;
  unsigned int x, y, tc, cols;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *row;

  glyphs=load_font();
  if(!glyphs)
    return;

  if((buffer=allocate_pages(buffer_pages))==NULL)
    return;

  bg.Reserved=0;
  fg.Reserved=0;
  fg.Red=255;
  fg.Blue=255;
  fg.Green=255;

  for(y=0;y<16;y++)
  {
    row=buffer+y*width;
    for(x=0;x<width;x++)
    {
      row[x].Red=0;
      row[x].Blue=0;
      row[x].Green=0;
      row[x].Reserved=0;
    }
  }
  draw_text(buffer,width,glyphs,1,1,fg,L"font blending test:");

  for(y=16;y<height;y++)
  {
    row=buffer+y*width;
    for(x=0;x<width;x++)
    {
      row[x].Red=(y+x)%128;
      row[x].Blue=(256+y-x)%128;
      row[x].Green=(512-y-x)%128;
      row[x].Reserved=0;
    }
  }

  for(y=85;y<235;y++)
  {
    row=buffer+y*width;
    for(x=230;x<370;x++)
    {
      row[x].Red=0;
      row[x].Blue=0;
      row[x].Green=0;
    }
    for(x=380;x<520;x++)
    {
      row[x].Red=255;
      row[x].Blue=255;
      row[x].Green=255;
    }
  }

  cols=glyphs->glyph_count/8+1;
  for(tc=0;tc<glyphs->glyph_count;tc++)
  {
    x=tc%cols;
    y=(tc-x)/cols;
    fg.Red=255*x/cols;
    fg.Green=255-31*y;
    fg.Blue=128-y*15+x*15;
    draw_glyph(buffer+width*100+100+width*y*15+x*8,width,glyphs->glyphs+tc,fg);
    draw_glyph(buffer+width*100+250+width*y*15+x*8,width,glyphs->glyphs+tc,fg);
    draw_glyph(buffer+width*100+400+width*y*15+x*8,width,glyphs->glyphs+tc,fg);
  }
  result=gop->Blt(gop,buffer,EfiBltBufferToVideo,0,0,0,0,width,height,0);
  ON_ERROR_RETURN(L"gop->Blt",);

  wait_for_key();

  bg.Red=0;
  bg.Blue=128;
  bg.Green=0;

  fg.Red=255;
  fg.Blue=255;
  fg.Green=255;

  result=gop->Blt(gop,&bg,EfiBltVideoFill,0,0,0,0,width,height,0);
  ON_ERROR_RETURN(L"gop->Blt",);
  result=gop->Blt(gop,buffer,EfiBltVideoToBltBuffer,0,0,0,0,width,height,0);
  ON_ERROR_RETURN(L"gop->Blt",);

  REG(rax);
  REG(rbx);
  REG(rcx);
  REG(rdx);

  outtext=memsprintf(L"A problem has been detected and Sunlight has been shut down to prevent damage\n\
to your planet.\n\
\n\
The problem seems to be caused by the following file: GOP.EFI\n\
\n\
BLUE_SCREEN_IN_WINDOWS_FREE_AREA\n\
\n\
If this is the first time you've seen a Stop error screen,\n\
what have you been doing all this time? If this screen\n\
appears again, follow these steps:\n\
\n\
 1. find someone to show this screen to\n\
 2. watch their confusion\n\
\n\
Technical information:\n\
\n\
*** STOP: 0x499602D2   (rax=%016lX\n\
  rbx=%016lX, rcx=%016lX, rdx=%016lX)",rax,rbx,rcx,rdx);
  draw_text(buffer,width,glyphs,1,1,fg,outtext);
  result=gop->Blt(gop,buffer,EfiBltBufferToVideo,0,0,0,0,width,height,0);
  ON_ERROR_RETURN(L"gop->Blt",);
  if(!free_pages(glyphs,glyphs->memory_pages))
    return;
  if(!free_pages(buffer,buffer_pages))
    return;
}


/**
 * Performs all the enabled graphics demonstrations
 */
void do_graphics_stuff()
{
  EFI_GRAPHICS_OUTPUT_PROTOCOL *gop=get_graphics_protocol();

  print_graphics_modes(gop);
  Print(L"press any key...\n");
  wait_for_key();
  if(set_graphics_mode(gop,ARG_MODE)!=EFI_SUCCESS)
  {
    print_graphics_modes(gop);
    return;
  }
  Print(L"(if you can read this you're probably in a text console - just hit a few random keys over the next ~20s)\n");

  if(!ARG_SKIP_BARS)
  {
    draw_bars(gop);
    wait_for_key();
  }
  if(!ARG_SKIP_IMAGES)
  {
    draw_image(gop,L"\\demoimg.ppm",parse_ppm_image_data);
    wait_for_key();
  }
  if(!ARG_SKIP_FONT)
  {
    draw_font(gop);
    wait_for_key();
  }
  if(!ARG_SKIP_OBJECTS)
  {
    draw_moving_objects(gop);
    wait_for_key();
  }
  if(!ARG_SKIP_ANIM)
  {
    draw_prepared_fs_anim(gop);
  }
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
  if((rv=init(argc,argv,2,&graphics_arguments,&_arguments))!=EFI_SUCCESS)
    return rv;

  do_graphics_stuff();

  shutdown();
  return EFI_SUCCESS;
}
