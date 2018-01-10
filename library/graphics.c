/** \file
 * Functions for creating and showing graphics
 *
 * \author Richard Nusser
 * \copyright 2017-2018 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_lib_graphics
 */

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include "../include/graphics.h"
#include "../include/memory.h"
#include "../include/logger.h"
#include "../include/cmdline.h"
#include "../include/timestamp.h"
#include "../include/string.h"


EFI_GRAPHICS_OUTPUT_PROTOCOL *graphics_protocol;     /**< UEFI's graphics output protocol */
EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *graphics_info; /**< UEFI's graphics info */
UINTN graphics_fs_width;                             /**< screen width, in pixels */
UINTN graphics_fs_height;                            /**< screen height, in pixels */
EFI_GRAPHICS_OUTPUT_BLT_PIXEL *graphics_fs_buffer;   /**< full-screen output buffer */
UINTN graphics_fs_pages;                             /**< number of pages for output buffer */
UINTN graphics_fs_pixel_count;                       /**< number of pixels in output buffer */


static trig_func *_sin=NULL; /**< pointer to sin() function */
static trig_func *_cos=NULL; /**< pointer to cos() function */

/**
 * Sets the sin() function pointer to use in graphics operations
 *
 * \param f pointer to the function to use
 */
void set_graphics_sin_func(trig_func *f)
{
  _sin=f;
}

/**
 * Sets the cos() function pointer to use in graphics operations
 *
 * \param f pointer to the function to use
 */
void set_graphics_cos_func(trig_func *f)
{
  _cos=f;
}


/**
 * Validates the "vsync mode" command-line parameter
 *
 * \param value the input to check
 * \return whether the input is a valid vsync mode
 */
BOOLEAN _validate_vsync(double_uint64_t value)
{
  if(value.uint64>=0 && value.uint64<=3)
    return TRUE;
  LOG.error(L"vsync mode must be 0..3, got %d instead",value.uint64);
  return FALSE;
}

/**
 * Validates the "framerate limit" command-line parameter
 *
 * \param value the input to check
 * \return whether the input is a valid framerate limit
 */
BOOLEAN _validate_fps(double_uint64_t value)
{
  if(value.uint64>0)
    return TRUE;
  LOG.error(L"fps must be greater than 0");
  return FALSE;
}


/** list of graphics-related command-line arguments */
cmdline_argument_t graphics_argument_list[] = {
  {{uint64:2},ARG_INT,NULL,L"-mode",L"Select graphics mode"}, //can't validate at start, needs graphics output protocol handle to check available modes
  {{uint64:0},ARG_INT,_validate_vsync,L"-vsync",L"Select vsync mode: 0=off, 1,2=either, 3=both"},
  {{uint64:100},ARG_INT,_validate_fps,L"-fps",L"Set approximate frames per second limit"},
};

/** group for graphics-related arguments */
ARG_GROUP(graphics_arguments,graphics_argument_list,L"Graphics options");


/*********
 * Images
 */

/** internal: data type for netpbm parser functions */
typedef void netpbm_pixel_parser_f(char *in, EFI_GRAPHICS_OUTPUT_BLT_PIXEL *out, unsigned int pixels, unsigned int width);

/**
 * internal: parses PPM pixel data
 *
 * \param in     the pixel data to parse, as bytes
 * \param out    the output sprite to write to
 * \param pixels the number of pixels in the image
 * \param width  (unused) the image's width, in pixels
 */
static void _parse_ppm_pixel_data(char *in, EFI_GRAPHICS_OUTPUT_BLT_PIXEL *out, unsigned int pixels, unsigned int width)
{
  unsigned int tc;

  for(tc=0;tc<pixels;tc++)
  {
    out[tc].Red=in[tc*3];
    out[tc].Green=in[tc*3+1];
    out[tc].Blue=in[tc*3+2];
    out[tc].Reserved=0;
  }
}

/**
 * internal: parses PGM pixel data
 *
 * \param in     the pixel data to parse, as bytes
 * \param out    the output sprite to write to
 * \param pixels the number of pixels in the image
 * \param width  (unused) the image's width, in pixels
 */
static void _parse_pgm_pixel_data(char *in, EFI_GRAPHICS_OUTPUT_BLT_PIXEL *out, unsigned int pixels, unsigned int width)
{
  unsigned int tc;
  for(tc=0;tc<pixels;tc++)
  {
    out[tc].Red=in[tc];
    out[tc].Green=in[tc];
    out[tc].Blue=in[tc];
    out[tc].Reserved=0;
  }
}

/**
 * internal: parses PBM pixel data
 *
 * \param in     the pixel data to parse, as bytes
 * \param out    the output sprite to write to
 * \param pixels the number of pixels in the image
 * \param width  the image's width, in pixels
 */
static void _parse_pbm_pixel_data(char *in, EFI_GRAPHICS_OUTPUT_BLT_PIXEL *out, unsigned int pixels, unsigned int width)
{
  int tc, byte_offset, bit_offset, pixel_in_row;
  unsigned int value;
  unsigned char mask;
  unsigned int row;
  unsigned int bytes_per_row;

  bytes_per_row=(width-1)/8+1;

  for(tc=0;tc<pixels;tc++)
  {
    row=tc/width;
    pixel_in_row=tc%width;
    bit_offset=pixel_in_row%8;
    byte_offset=pixel_in_row/8+row*bytes_per_row;

    mask=0x80>>bit_offset;
    value=(in[byte_offset]&mask)==0?255:0;
//    LOG.trace(L"pixel %4d: byte %3d (%lX:%s), bit %d => mask %s, value %3d",tc,byte_offset,in+byte_offset,HEX(0,in[byte_offset]&0xff,2),bit_offset,HEX(10,mask,2),value);
    out[tc].Red=value;
    out[tc].Green=value;
    out[tc].Blue=value;
    out[tc].Reserved=0;
  }
}


/**
 * Allocates and initializes an image
 *
 * \param width  the image's width
 * \param height the image's height
 * \return the image
 */
image_t *create_image(INTN width, INTN height)
{
  image_t *image;
  INTN pages=(sizeof(image_t)+(width*height*sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL))-1)/4096+1;

  if((image=allocate_pages(pages))==NULL)
    return NULL;
  image->memory_pages=pages;

  image->width=width;
  image->height=height;

  return image;
}

/**
 * internal: parses netpbm file contents into an image
 *
 * \param contents the file contents to parse
 * \param pixel_parser the pixel parser function to use
 * \param magic_digit the file's expected netpbm magic digit (indicates pixel format)
 * \param has_maxval_row whether the file has a row indicating pixels' maximum values
 * \return the parsed image, or NULL on error
 */
static image_t *_parse_netpbm_image_data(file_contents_t *contents, netpbm_pixel_parser_f *pixel_parser, char magic_digit, char has_maxval_row)
{
  image_t *image;

  unsigned int start;
  UINT64 tc;
  unsigned int width, height;
  char *data=contents->data;
  unsigned int length=contents->data_length;

  LOG.debug(L"data length: %d",length);
//  DumpHex(2,(UINT64)data,length>256?256:length,data);
  if(data[0]!='P' || data[1]!=magic_digit || !ctype_whitespace(data[2]))
  {
    LOG.error(L"data doesn't start with PPM magic value");
    return NULL;
  }
  start=3;
  while(ctype_whitespace(data[start]))
    start++;
  if(data[start]=='#')
    while(data[start]!='\n')
      start++;
  start++;
  for(tc=start;tc<length;tc++)
    if(ctype_whitespace(data[tc]))
      break;
  data[tc]=0;
  width=atoui64(data+start);
  start=tc+1;
  for(tc=start;tc<length;tc++)
    if(ctype_whitespace(data[tc]))
      break;
  data[tc]=0;
  height=atoui64(data+start);
  LOG.debug(L"width=%d, height=%d",width,height);
  start=tc+1;

  //XXX ignoring maxval

  if(has_maxval_row)
  {
    LOG.debug(L"skipping maxval row");
    for(tc=start;tc<length;tc++)
      if(ctype_whitespace(data[tc]))
        break;
    start=tc+1;
  }

  image=create_image(width,height);
  if(image)
    pixel_parser(&data[start],image->data,width*height,width);
  return image;
}

/**
 * Parses a PPM (color) image.
 *
 * \param contents the file contents to parse
 * \return the parsed images, or NULL on error
 */
image_t *parse_ppm_image_data(file_contents_t *contents)
{
  return _parse_netpbm_image_data(contents,_parse_ppm_pixel_data,'6',1);
}

/**
 * Parses a PGM (grayscale) image.
 *
 * \param contents the file contents to parse
 * \return the parsed images, or NULL on error
 */
image_t *parse_pgm_image_data(file_contents_t *contents)
{
  return _parse_netpbm_image_data(contents,_parse_pgm_pixel_data,'5',1);
}

/**
 * Parses a PBM (black/white bitmap) image.
 *
 * \param contents the file contents to parse
 * \return the parsed images, or NULL on error
 */
image_t *parse_pbm_image_data(file_contents_t *contents)
{
  return _parse_netpbm_image_data(contents,_parse_pbm_pixel_data,'4',0);
}


/**
 * shortcut macro for weighted mixing of 4 pixels' values
 *
 * \param CH the color channel's name
 */
#define MIX_CHANNEL(CH) rv.CH = corners[0].CH*sa + corners[1].CH*sb + corners[row_width].CH*sc + corners[row_width+1].CH*sd

/**
 * Performs bilinear interpolation of a pixel value within a square of 4 surrounding pixels.
 * This illustrates the required parameters to interpolate a new pixel P:
 *
 *     corners[0]
 *       |
 *       |  x
 *       v--|
 *      -0         1         px        px
 *     y|
 *      -   P
 *
 *       2         3         px        px
 *       |------------------------------|
 *                  row_width
 *
 * \param corners   pointer to the pixel data to interpolate between, starting at top left pixel's offset
 * \param row_width the source image's row width, in pixels
 * \param x         the horizontal position to interpolate at, within [0..1], 0 being left
 * \param y         the vertical position to interpolate at, within [0..1], 0 being top
 * \return the interpolated pixel
 */
COLOR interpolate_4px(COLOR *corners, UINTN row_width, float x, float y)
{
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL rv;
  rv.Reserved=0;

  if(x<0||x>1||y<0||y>1)
  {
    LOG.error(L"coords out of bounds");
    return rv;
  }

  float ix=1-x;
  float iy=1-y;

  float sa=ix*iy;
  float sb=x*iy;
  float sc=ix*y;
  float sd=x*y;

  MIX_CHANNEL(Red);
  MIX_CHANNEL(Blue);
  MIX_CHANNEL(Green);
  return rv;
}

/**
 * Performs linear interpolation of a pixel value between 2 pixels.
 * Works like interpolate_2px(), only with 1 dimension.
 *
 * \param colors pointer to 2 pixels to interpolate between
 * \param ratio  the position to interpolate at, within [0..1], 0.0 being colors[0] and 1.0 being colors[1]
 * \return the interpolated pixel
 */
COLOR interpolate_2px(COLOR *colors, float ratio)
{
  return interpolate_4px(colors,0,ratio,0);
}


/**
 * Rotates an image by an arbitrary angle.
 *
 * Make sure you set trigonometry function pointers with set_graphics_sin_func() and set_graphics_cos_func() before using this.
 * This is necessary to avoid having to link StdLib into all graphical applications.
 *
 * \param original the source image to rotate, must be square with 2*radius+1 pixels width and height
 * \param rotated  the output image to write the rotated image to
 * \param radius   the inner circle's radius, should be (square's side length-1)/2
 * \param theta    the clockwise angle to rotate by, in radians
 */
void rotate_image(SPRITE original, SPRITE rotated, INTN radius, float theta)
{
  float cost=_cos(theta);
  float sint=_sin(theta);
  float xrot, yrot;
  INTN x, y;
  INTN xrot_int, yrot_int;
  INTN center_x=radius;
  INTN center_y=radius;
  INTN eff_x, eff_y;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL black={0,0,0,0};
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL col;
  INTN diameter=2*radius+1;

  if(_cos==NULL || _sin==NULL)
  {
    LOG.error(L"trigonometry functions unset, can't rotate");
    return;
  }

  for(x=-radius;x<radius;x++)
  {
    for(y=-radius;y<radius;y++)
    {
      xrot= cost*x+sint*y;
      yrot=-sint*x+cost*y;
      xrot_int=(int)xrot;
      if(xrot<0)
        xrot_int--;
      yrot_int=(int)yrot;
      if(yrot<0)
        yrot_int--;
      eff_x=xrot_int+center_x;
      eff_y=yrot_int+center_y;
      if(eff_x>=0&&eff_x<diameter && eff_y>=0&&eff_y<diameter)
        col=interpolate_4px(original+eff_y*diameter+eff_x,diameter,xrot-xrot_int,yrot-yrot_int);
      else
        col=black;
      rotated[(y+center_y)*diameter+center_x+x]=col;
    }
  }
}

/**
 * internal: loads a netpbm image file.
 *
 * \param filename the filename to read the image from
 * \param parse_f  the parser function to use
 * \return the loaded image, or NULL on error
 */
static image_t *_load_netpbm_file(CHAR16 *filename, image_t *parse_f(file_contents_t *))
{
  image_t *rv=NULL;
  file_contents_t *data;

  data=get_file_contents(filename);
  if(data)
  {
    rv=parse_f(data);
    free_pages(data,data->memory_pages);
  }
  else
    LOG.warn(L"could not load netpbm file '%s'",filename);
  return rv;
}

/**
 * Reads a netpbm PPM (color) file.
 *
 * \param filename the image's filename
 * \return the image, or NULL on error
 */
image_t *load_ppm_file(CHAR16 *filename)
{
  return _load_netpbm_file(filename,parse_ppm_image_data);
}

/**
 * Reads a netpbm PGM (grayscale) file.
 *
 * \param filename the image's filename
 * \return the image, or NULL on error
 */
image_t *load_pgm_file(CHAR16 *filename)
{
  return _load_netpbm_file(filename,parse_pgm_image_data);
}

/**
 * Reads a netpbm PBM (black/white bitmap) file.
 *
 * \param filename the image's filename
 * \return the image, or NULL on error
 */
image_t *load_pbm_file(CHAR16 *filename)
{
  return _load_netpbm_file(filename,parse_pbm_image_data);
}

/**
 * Reads a netpbm file, determines the image format by the file's extension.
 *
 * \param filename the image's filename
 * \return the image, or NULL on error
 */
image_t *load_netpbm_file(CHAR16 *filename)
{
  INTN len;
  image_t *(*loader)(CHAR16 *filename);
  CHAR16 *ext;

  len=StrLen(filename);
  if(len<4)
  {
    LOG.error(L"cannot determine extension of '%s'",filename);
    return NULL;
  }
  ext=filename+len-4;
  if(StrCmp(ext,L".pgm")==0)
    loader=load_pgm_file;
  else if(StrCmp(ext,L".ppm")==0)
    loader=load_ppm_file;
  else if(StrCmp(ext,L".pbm")==0)
    loader=load_pbm_file;
  else
  {
    LOG.error(L"unknown file extension of '%s'",filename);
    return NULL;
  }
  return loader(filename);
}

/**
 * Frees an image resource.
 *
 * \param image the image to free
 */
void free_image(image_t *image)
{
  if(image==NULL)
  {
    LOG.error(L"asked to free NULL image");
    return;
  }
  free_pages(image,image->memory_pages);
}

/**
 * Loads a list of image assets.
 * Image assets are a handy way of loading e.g. sprites in a graphical application.
 *
 * \param count  the number of assets to load
 * \param assets the list of assets to load
 */
void load_image_assets(UINTN count, image_asset_t *assets)
{
  UINTN tc;

  for(tc=0;tc<count;tc++)
    *assets[tc].image=load_netpbm_file(assets[tc].filename);
}

/**
 * Frees a list of image assets.
 *
 * \param count  the number of assets to free
 * \param assets the list of assets to free
 */
void free_image_assets(UINTN count, image_asset_t *assets)
{
  UINTN tc;
  for(tc=0;tc<count;tc++)
    if(*assets[tc].image!=NULL)
      free_image(*assets[tc].image);
}


/**********
 * General
 */

/**
 * Prints the list of available graphics modes.
 *
 * \param gop the UEFI graphics output protocol handle
 */
void print_graphics_modes(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop)
{
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
  UINTN info_size;
  int tc;
  EFI_STATUS result;

  Print(L"number of modes: %d\n",gop->Mode->MaxMode);
  for(tc=0;tc<gop->Mode->MaxMode;tc++)
  {
    result=gop->QueryMode(gop,tc,&info_size,&info);
    if(result!=EFI_SUCCESS)
      continue;
    Print(L"  %02d: %4dx%4d",tc,info->HorizontalResolution,info->VerticalResolution);
    if(tc%4==3)
      Print(L"\n");
  }
  if(tc%4!=3)
    Print(L"\n");
}

/**
 * Draws an unbuffered rectangle to the screen.
 *
 * \param gop    the UEFI graphics output protocol handle
 * \param x      the rectangle's left x coordinate
 * \param y      the rectangle's top y coordinate
 * \param width  the rectangle's width, in pixels
 * \param height the rectangle's height, in pixels
 * \param color  the rectangle's color
 * \return the EFI status, EFI_SUCCESS on success
 */
EFI_STATUS draw_filled_rect(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, UINTN x, UINTN y, UINTN width, UINTN height, COLOR *color)
{
  EFI_STATUS result;
  result=gop->Blt(gop,color,EfiBltVideoFill,0,0,x,y,width,height,0);
  return result;
}

/**
 * Fetches the UEFI graphics output protocol handle.
 * This is the main access point for UEFI graphics functions.
 *
 * \return the protocol handle, or NULL on error.
 */
EFI_GRAPHICS_OUTPUT_PROTOCOL *get_graphics_protocol()
{
  EFI_GUID gop_guid=EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
  EFI_STATUS result;
  EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
  EFI_HANDLE handles[100];
  UINTN handles_size=100*sizeof(EFI_HANDLE);
  UINTN handle_count;

  result=gST->BootServices->LocateHandle(ByProtocol,&gop_guid,NULL,&handles_size,(void **)&handles);
  ON_ERROR_RETURN(L"LocateHandle",NULL);
  handle_count=handles_size/sizeof(EFI_HANDLE);
  LOG.debug(L"handles size: %d bytes (%d entries)",handles_size,handle_count);

  LOG.trace(L"handle: %16lX",handles[0]);
  result=gST->BootServices->OpenProtocol(handles[0],&gop_guid,(void **)&gop,gImageHandle,NULL,EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
  ON_ERROR_RETURN(L"OpenProtocol",NULL);
  return gop;
}

/**
 * Sets the graphics mode.
 *
 * \param gop  the UEFI graphics output protocol handle
 * \param mode the graphics mode to set
 * \return the resulting status code
 */
EFI_STATUS set_graphics_mode(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int mode)
{
  EFI_STATUS result;
  if(gop->Mode->MaxMode<(mode+1))
  {
    LOG.error(L"requested mode %d above maximum (%d)",mode,gop->Mode->MaxMode-1);
    return EFI_UNSUPPORTED;
  }
  result=gop->SetMode(gop,mode);
  return result;
}

/**
 * Fetches information about the current graphics mode.
 *
 * \param gop  the UEFI graphics output protocol handle
 * \param info the graphics mode info structure to write to
 * \return the resulting status code
 */
EFI_STATUS query_current_mode(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, EFI_GRAPHICS_OUTPUT_MODE_INFORMATION **info)
{
  EFI_STATUS result;
  UINTN size;

  result=gop->QueryMode(gop,gop->Mode->Mode,&size,info);
  return result;
}


/**
 * internal: parses a character from a font sprite sheet into glyph data.
 *
 * \XXX this currently only reads the red color channel, the source image should be grayscale anyway. Could check or average all channels just to be safe.
 *
 * \param data  the glyph data to write to
 * \param image the sprite sheet to read from
 * \param left  the left offset to start reading from
 * \param top   the top offset to start reading from
 */
static void _parse_glyph_data(UINT8 *data, image_t *image, unsigned int left, unsigned int top)
{
  unsigned int tc, tr;
  unsigned int row;

  for(tr=0;tr<15;tr++)
  {
    row=top+tr;
    for(tc=0;tc<8;tc++)
      data[tr*8+tc]=image->data[row*image->width+left+tc].Red;
  }
}

/**
 * Parses a font sprite sheet.
 *
 * \param image the sprite sheet to read from
 * \param text  the text printed in the sprite sheet, use `\n` to indicate line breaks
 * \return the parsed list of glyphs, make sure to free this when you're done
 */
glyph_list_t *parse_glyphs(image_t *image, CHAR16 *text)
{
  unsigned int tc;
  unsigned int col=0, row=0;
  unsigned int length=StrLen(text);
  CHAR16 chr;
  glyph_list_t *glyphs;
  UINT32 pages;

  pages=(sizeof(glyph_list_t)+sizeof(glyph_t)*length)/4096+1;
  if((glyphs=allocate_pages(pages))==NULL)
    return NULL;
  glyphs->memory_pages=pages;
  glyphs->glyph_count=0;

  for(tc=0;tc<length;tc++)
  {
    chr=text[tc];
    if(chr==L'\n')
    {
      col=0;
      row++;
      continue;
    }

    glyphs->glyphs[glyphs->glyph_count].chr=chr;
    _parse_glyph_data(glyphs->glyphs[glyphs->glyph_count++].data,image,8*col,15*row);

    col++;
  }
  return glyphs;
}

/**
 * Parses the font file.
 * There's currently just one font file with a hardcoded text.
 * The font file is a netpbm image with known text printed in it.
 *
 * \return the loaded font, or NULL on error
 */
glyph_list_t *load_font()
{
  file_contents_t *contents;
  image_t *image;
  glyph_list_t *glyphs;

  CHAR16 text[]=L"ABCDEFGHIJKLMNOPQRSTUVWXYZ(){}$&\nabcdefghijklmnopqrstuvwxyz[]%#^@\n0123456789.:,;+-*/_'\"\\!?=<>~| ";

  contents=get_file_contents(L"\\font815.pgm");
  if(!contents)
    return NULL;

  image=parse_pgm_image_data(contents);

  if(!free_pages(contents,contents->memory_pages))
    return NULL;

  if(!image)
    return NULL;
  glyphs=parse_glyphs(image,text);

  if(!free_pages(image,image->memory_pages))
    return NULL;

  return glyphs;
}

/**
 * Draws a single glyph to a sprite or output buffer.
 *
 * \param start        the first pixel to write to
 * \param buffer_width the width of the output buffer, in pixels
 * \param glyph        the glyph to draw
 * \param color        the color to draw with
 */
void draw_glyph(SPRITE start, UINTN buffer_width, glyph_t *glyph, COLOR color)
{
  unsigned int tc, tr;
  float scale;

  for(tr=0;tr<15;tr++)
  {
    for(tc=0;tc<8;tc++)
    {
      scale=glyph->data[tr*8+tc]/255.0;

      start[tc].Red=start[tc].Red+((float)color.Red-start[tc].Red)*scale;
      start[tc].Blue=start[tc].Blue+((float)color.Blue-start[tc].Blue)*scale;
      start[tc].Green=start[tc].Green+((float)color.Green-start[tc].Green)*scale;
    }
    start+=buffer_width;
  }
}

/**
 * Draws text to a sprite or output buffer.
 *
 * \param buffer       the output image to write to
 * \param buffer_width the output image's width, in pixels
 * \param glyphs       the glyph list (font) to use
 * \param x            the x coordinate to start writing at
 * \param y            the y coordinate to start writing at
 * \param color        the color to draw the text with
 * \param text         the text to write, as UTF-16
 */
void draw_text(SPRITE buffer, UINTN buffer_width, glyph_list_t *glyphs, UINT32 x, UINT32 y, COLOR color, CHAR16 *text)
{
  unsigned int tc, td;
  unsigned int length=StrLen(text);
  CHAR16 chr;
  glyph_t *glyph;
  unsigned int current_row=0;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *pos, *start;

  start=buffer+y*buffer_width+x;
  pos=start;

  for(tc=0;tc<length;tc++)
  {
    glyph=NULL;
    chr=text[tc];
    if(chr==L'\r') //CRs are skipped, they're useless with additive drawing anyway
      continue;
    if(chr==L'\n')
    {
      pos=start+(++current_row)*15*buffer_width;
      continue;
    }
    for(td=0;td<glyphs->glyph_count;td++)
    {
      if(glyphs->glyphs[td].chr==chr)
      {
        glyph=glyphs->glyphs+td;
        break;
      }
    }
    if(glyph==NULL)
    {
      LOG.warn(L"no glyph for character '%c' (%d)",chr>=0x20?chr:L' ',chr);
      glyph=glyphs->glyphs+glyphs->glyph_count-1;
    }
    draw_glyph(pos,buffer_width,glyph,color);
    pos+=8; /** \TODO warn about exceeding width, then either do a line break or clip current line */
  }
}

/**
 * Frees a previously allocated list of glyphs.
 *
 * \param glyphs the list to free
 */
void free_glyphs(glyph_list_t *glyphs)
{
  if(glyphs==NULL)
  {
    LOG.error(L"asked to free NULL glyph list");
    return;
  }
  free_pages(glyphs,glyphs->memory_pages);
}


/**
 * Waits for the graphics card's vertical synchronisation event.
 * How and if this works depends on your hardware. If you're running the application in a virtual environment it might
 * not work at all.
 */
void wait_vsync()
{
  EFI_STATUS result;
  UINT64 rax=0;

  if(ARG_VSYNC&1)
  {
    while(!(rax&8))
    {
      asm volatile ("mov $0x3da,%%dx \n\
          in %%dx,%%al \n\
          " :"=a" (rax) : :"rdx");
      result=gBS->Stall(100);
      ON_ERROR_RETURN(L"Stall",);
    }
  }
  if(ARG_VSYNC&2)
  {
    while(rax&8)
    {
      asm volatile ("mov $0x3da,%%dx \n\
          in %%dx,%%al \n\
          " :"=a" (rax) : :"rdx");
      result=gBS->Stall(100);
      ON_ERROR_RETURN(L"Stall",);
    }
  }
}

/**
 * Limits graphics animation framerates.
 * This works by keeping track of timestamps: a given number of ticks needs to have elapsed between frames. If vsync is
 * enabled this function additionally waits for the next vsync event.
 *
 * \param previous            the previous timestamp value, will be updated when done waiting
 * \param minimum_frame_ticks the number of ticks that must have elapsed since the previous timestamp
 * \return TRUE on success, FALSE otherwise
 */
BOOLEAN limit_framerate(UINT64 *previous, UINT64 minimum_frame_ticks)
{
  EFI_STATUS result;
  UINT64 current=get_timestamp();

  while(current-*previous<minimum_frame_ticks)
  {
    result=gBS->Stall(500);
    ON_ERROR_RETURN(L"Stall",FALSE);
    current=get_timestamp();
  }
  *previous=current;
  wait_vsync();
  return TRUE;
}

/**
 * Creates a full-screen graphics buffer.
 * Make sure you have set the target graphics mode first.
 *
 * \return the buffer, or NULL on error
 */
GFX_BUFFER create_graphics_fs_buffer()
{
  void *addr=allocate_pages(graphics_fs_pages);
  if(!addr)
    return NULL;
  ZeroMem(addr,graphics_fs_pages*4096);
  return addr;
}

/**
 * Frees a full-screen graphics buffer.
 *
 * \param addr the buffer to free
 */
void free_graphics_fs_buffer(void *addr)
{
  free_pages(addr,graphics_fs_pages);
}


/**
 * Outputs a full-screen graphics buffer to the graphics display.
 *
 * \param buffer the graphics buffer to output
 * \return the resulting status
 */
EFI_STATUS graphics_fs_blt(GFX_BUFFER buffer)
{
  return graphics_protocol->Blt(graphics_protocol,buffer,EfiBltBufferToVideo,0,0,0,0,graphics_fs_width,graphics_fs_height,0);
}

/**
 * Initializes the graphics output.
 * Call this function at the start of graphical applications.
 *
 * \return the resulting status, EFI_SUCCESS on success
 */
EFI_STATUS init_graphics()
{
  EFI_STATUS result;

  graphics_protocol=get_graphics_protocol();
  if(!graphics_protocol)
  {
    LOG.error(L"cannot locate graphics output");
    return EFI_UNSUPPORTED;
  }

//  Print(L"(if you can read this the graphics mode just ran over you: press any key)\n");

  if((result=set_graphics_mode(graphics_protocol,ARG_MODE))!=EFI_SUCCESS)
  {
    print_graphics_modes(graphics_protocol);
    return result;
  }

  result=query_current_mode(graphics_protocol,&graphics_info);
  if(result!=EFI_SUCCESS)
    return result;

  graphics_fs_width=graphics_info->HorizontalResolution;
  graphics_fs_height=graphics_info->VerticalResolution;
  graphics_fs_pixel_count=graphics_fs_width*graphics_fs_height;
  graphics_fs_pages=(graphics_fs_pixel_count*sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL)-1)/4096+1;

  graphics_fs_buffer=create_graphics_fs_buffer();
  if(!graphics_fs_buffer)
    return EFI_UNSUPPORTED;

  return EFI_SUCCESS;
}

/**
 * Shuts down the graphics features.
 * Call this at the end of graphical applications.
 */
void shutdown_graphics()
{
  free_graphics_fs_buffer(graphics_fs_buffer);
}
