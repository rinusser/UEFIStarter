/** \file
 * Functions for creating and showing graphics
 *
 * \author Richard Nusser
 * \copyright 2017 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_lib_graphics
 */

#ifndef __GRAPHICS_H
#define __GRAPHICS_H

#include <Uefi.h>
#include <Protocol/GraphicsOutput.h>
#include "cmdline.h"
#include "files.h"
#include "logger.h"

#define COLOR      EFI_GRAPHICS_OUTPUT_BLT_PIXEL    /**< shortcut macro: indicates pixel data is intended to be used as paint color */
#define SPRITE     EFI_GRAPHICS_OUTPUT_BLT_PIXEL *  /**< shortcut macro: indicates pixel data is intended to be as a drawable image */
#define GFX_BUFFER EFI_GRAPHICS_OUTPUT_BLT_PIXEL *  /**< shortcut macro: indicates pixel data is intended to be used as a screen buffer */


extern EFI_GRAPHICS_OUTPUT_PROTOCOL *graphics_protocol;
extern EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *graphics_info;
extern UINTN graphics_fs_width, graphics_fs_height;
extern GFX_BUFFER graphics_fs_buffer;
extern UINTN graphics_fs_pages;
extern UINTN graphics_fs_pixel_count;

#define ARG_MODE  graphics_argument_list[0].value.uint64 /**< helper macro to access the "graphics mode" command-line argument's value */
#define ARG_VSYNC graphics_argument_list[1].value.uint64 /**< helper macro to access the "vsync mode" command-line argument's value */
#define ARG_FPS   graphics_argument_list[2].value.uint64 /**< helper macro to access the "framerate limit" command-line argument's value */
extern cmdline_argument_t graphics_argument_list[];
extern cmdline_argument_group_t graphics_arguments;


/**********
 * General
 */

EFI_STATUS init_graphics();
void shutdown_graphics();

GFX_BUFFER create_graphics_fs_buffer();
void free_graphics_fs_buffer(void *addr);
EFI_STATUS graphics_fs_blt(GFX_BUFFER buffer);

EFI_GRAPHICS_OUTPUT_PROTOCOL *get_graphics_protocol(); //move to init and make field static?
EFI_STATUS draw_filled_rect(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, UINTN x, UINTN y, UINTN width, UINTN height, COLOR *color);
void print_graphics_modes(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop);
EFI_STATUS set_graphics_mode(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, int mode);
EFI_STATUS query_current_mode(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, EFI_GRAPHICS_OUTPUT_MODE_INFORMATION **info);
void wait_vsync();
BOOLEAN limit_framerate(UINT64 *previous, UINT64 minimum_frame_ticks);

typedef double trig_func(double); /**< data type for trigonometry function pointers */
void set_graphics_sin_func(trig_func *f);
void set_graphics_cos_func(trig_func *f);


/*********
 * Images
 */

/** data type for image data, size is dynamic */
typedef struct
{
  UINTN memory_pages;                   /**< the number of memory pages allocated */
  UINT32 width;                         /**< the image's width, in pixels */
  UINT32 height;                        /**< the image's height, in pixels */
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL data[]; /**< the image's pixel data */
} image_t;

/** data type for image assets */
typedef struct
{
  image_t **image;  /**< the asset's content */
  CHAR16 *filename; /**< the asset's filename */
} image_asset_t;


image_t *parse_ppm_image_data(file_contents_t *contents);
image_t *parse_pgm_image_data(file_contents_t *contents);
image_t *parse_pbm_image_data(file_contents_t *contents);

image_t *load_ppm_file(CHAR16 *filename);
image_t *load_pgm_file(CHAR16 *filename);
image_t *load_pbm_file(CHAR16 *filename);
image_t *load_netpbm_file(CHAR16 *filename);
image_t *create_image(INTN width, INTN height);
void free_image(image_t *image);
void load_image_assets(UINTN count, image_asset_t *assets);
void free_image_assets(UINTN count, image_asset_t *assets);

COLOR interpolate_2px(COLOR *colors, float ratio);
COLOR interpolate_4px(COLOR *corners, UINTN row_width, float x, float y);
void rotate_image(SPRITE original, SPRITE rotated, INTN radius, float theta);


/********
 * Fonts
 */

/** data type for individual 8x15 font glyphs */
typedef struct
{
  CHAR16 chr;       /**< the character the glyph is for */
  UINT8 data[8*15]; /**< the glyph's pixel data */
} glyph_t;

/** data type for list of glyphs, size is dynamic */
typedef struct
{
  UINT32 memory_pages; /**< the number of allocated memory pages */
  UINT16 glyph_count;  /**< the number of glyphs in this list */
  glyph_t glyphs[];    /**< the list of glyphs */
} glyph_list_t;

glyph_list_t *parse_glyphs(image_t *image, CHAR16 *text);
glyph_list_t *load_font();
void draw_text(SPRITE buffer, UINTN buffer_width, glyph_list_t *glyphs, UINT32 x, UINT32 y, COLOR color, CHAR16 *text);
void draw_glyph(SPRITE start, UINTN buffer_width, glyph_t *glyph, COLOR color);
void free_glyphs(glyph_list_t *glyphs);


#endif
