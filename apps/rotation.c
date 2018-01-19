/** \file
 * This application demonstrates image rotation and bilinear interpolation.
 * You'll need to run this in a graphics-capable environment, see gop.c
 *
 * \author Richard Nusser
 * \copyright 2017-2018 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_apps
 */

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <math.h>
#include <stdio.h>
#include "../include/graphics.h"
#include "../include/memory.h"
#include "../include/console.h"
#include "../include/cmdline.h"
#include "../include/timestamp.h"
#include "../include/logger.h"


/** additional buffer required to rotate image */
EFI_GRAPHICS_OUTPUT_BLT_PIXEL *buffer2;


/** shortcut macro to access "radius" command-line parameter */
#define ARG_RADIUS args[0].value.uint64

/** list of command-line arguments */
cmdline_argument_t args[]={
  {{uint64:50},ARG_INT,NULL,L"-radius",L"circle radius [px]"}
};

/** application-specific command-line argument group */
ARG_GROUP(arggroup,args,L"Application-specific options");


/**
 * This draws an animated gradient.
 * It's actually a bilinear interpolation between the 4 corners of the screen: the corner colors change between frames.
 */
void draw_gradient()
{
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL corners[4];
  EFI_STATUS result;
  UINTN x, y;
  UINTN tc;
  UINT64 prev_ts, cur_ts;
  UINTN td;

  UINTN rel_pages;
  float *rel_xs, *rel_ys;

  ZeroMem(corners,sizeof(corners));
  corners[0].Red=255;
  corners[1].Blue=255;
  corners[2].Green=255;

  rel_pages=(sizeof(float)*(graphics_fs_width+graphics_fs_height)-1)/4096+1;
  rel_xs=allocate_pages(rel_pages);
  if(!rel_xs)
    return;
  rel_ys=rel_xs+graphics_fs_width;

  for(tc=0;tc<graphics_fs_width;tc++)
    rel_xs[tc]=(float)tc/graphics_fs_width;
  for(tc=0;tc<graphics_fs_height;tc++)
    rel_ys[tc]=(float)tc/graphics_fs_height;

  prev_ts=get_timestamp();
  for(tc=0;tc<256;tc++)
  {
    corners[0].Green=tc;
    corners[1].Red=tc;
    corners[2].Blue=tc;

    x=0;
    y=0;
    for(td=0;td<graphics_fs_pixel_count;td++)
    {
      if(x>=graphics_fs_width)
      {
        x=0;
        y++;
      }
      graphics_fs_buffer[td]=interpolate_4px(corners,2,rel_xs[x],rel_ys[y]);
      x++;
    }

    result=graphics_protocol->Blt(graphics_protocol,graphics_fs_buffer,EfiBltBufferToVideo,0,0,0,0,graphics_fs_width,graphics_fs_height,0);
    ON_ERROR_RETURN(L"graphics_protocol->Blt",);
    cur_ts=get_timestamp();
    gST->ConOut->SetCursorPosition(gST->ConOut,0,0);
    Print(L"%dms",(int)(timestamp_diff_seconds(prev_ts,cur_ts)*1000));
    prev_ts=cur_ts;
  }

  free_pages(rel_xs,rel_pages);

  wait_for_key();
}

/**
 * shortcut macro: defines an EFI_GRAPHICS_OUTPUT_BLT_PIXEL color value
 *
 * \param R red
 * \param G green
 * \param B blue
 * \param A alpha/reserved
 */
#define RGBA(R,G,B,A) {B,G,R,A}

/**
 * internal: draws the image to be rotated to the full-screen graphics buffer
 */
void draw_circle()
{
  EFI_STATUS result;
  INTN x, y;
  UINTN center_x, center_y;
  UINTN y_sq;
  UINTN r_sq;

  COLOR ring=RGBA(92,92,92,0);
  COLOR gray=RGBA(25,25,25,0);
  COLOR orange=RGBA(255,128,0,0);

  INTN outer_circle_max=ARG_RADIUS;
  INTN outer_circle_min=ARG_RADIUS*0.975;
  UINTN outer_circle_max_sq=outer_circle_max*outer_circle_max;
  UINTN outer_circle_min_sq=outer_circle_min*outer_circle_min;
  INTN diameter=outer_circle_max*2+1;

  center_x=ARG_RADIUS;
  center_y=ARG_RADIUS;

  SetMem(graphics_fs_buffer,graphics_fs_pages*4096,0);
  for(y=-outer_circle_max;y<outer_circle_max;y++)
  {
    y_sq=y*y;
    for(x=-outer_circle_max;x<outer_circle_max;x++)
    {
      r_sq=x*x+y_sq;
      if(r_sq>outer_circle_max_sq)
        continue;

      if(r_sq>=outer_circle_min_sq)
        graphics_fs_buffer[(center_y+y)*diameter+center_x+x]=ring;
      else if((x<=0&&y<=0) || (x>0&&y>0))
        graphics_fs_buffer[(center_y+y)*diameter+center_x+x]=orange;
      else
        graphics_fs_buffer[(center_y+y)*diameter+center_x+x]=gray;
    }
  }

  result=graphics_protocol->Blt(graphics_protocol,graphics_fs_buffer,EfiBltBufferToVideo,0,0,0,0,diameter,diameter,0);
  ON_ERROR_RETURN(L"graphics_protocol->Blt",);
}

/**
 * Animates a rotating image.
 */
void rotate_buffer()
{
  EFI_STATUS result;
  float theta;
  INTN radius=ARG_RADIUS;
  UINT64 prev_ts, minimum_frame_ticks;

  set_graphics_sin_func(sin);
  set_graphics_cos_func(cos);
  buffer2=allocate_pages(graphics_fs_pages);
  if(!buffer2)
    return;
  SetMem(buffer2,graphics_fs_pages*4096,0);

  minimum_frame_ticks=get_timestamp_ticks_per_second()/ARG_FPS;
  prev_ts=get_timestamp();
  for(theta=0;theta<=10*M_PI;theta+=M_PI/128)
  {
    rotate_image(graphics_fs_buffer,buffer2,radius,theta);
    result=graphics_protocol->Blt(graphics_protocol,buffer2,EfiBltBufferToVideo,0,0,0,0,2*radius+1,2*radius+1,0);
    ON_ERROR_RETURN(L"graphics_protocol->Blt",);
    gST->ConOut->SetCursorPosition(gST->ConOut,0,0);
    limit_framerate(&prev_ts,minimum_frame_ticks);
  }
  free_graphics_fs_buffer(buffer2);
  wait_for_key();
}


/**
 * Main function, gets invoked by UEFI shell.
 *
 * \param argc       the number of command-line arguments passed
 * \param argv_ascii the command-line arguments passed, as ASCII
 * \return an EFI status code for the shell
 */
int main(int argc, char **argv_ascii)
{
  EFI_STATUS rv;
  CHAR16 **argv;

  argv=argv_from_ascii(argc,argv_ascii);
  rv=init(argc,argv,2,&graphics_arguments,&arggroup);
  free_argv();

  if(rv==EFI_SUCCESS)
    rv=init_graphics();

  if(rv!=EFI_SUCCESS)
  {
    shutdown();
    return rv;
  }

  init_timestamps();
  draw_circle();
  rotate_buffer();
  draw_gradient();

  shutdown_graphics();
  shutdown();
  return EFI_SUCCESS;
}
