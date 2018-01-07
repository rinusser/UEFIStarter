/** \file
 * Assertions and utilities for graphics tests
 *
 * \author Richard Nusser
 * \copyright 2017 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_tests_asserts
 */

#include <Uefi.h>
#include "../../include/graphics.h"


/** shortcut macro for pixel's size in bytes */
#define BYTES_PER_PIXEL sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL)

/** default background color for color tests */
#define DIFFTEST_DEFAULT_BACKGROUND_UINT32 0x11223344


/** data type for bounding boxes */
typedef struct
{
  INTN left;    /**< the bounding box's left coordinate */
  INTN top;     /**< the bounding box's top coordinate */
  INTN right;   /**< the bounding box's right coordinate */
  INTN bottom;  /**< the bounding box's bottom coordinate */
} bounding_box_t;

/** data type for image change tests */
typedef struct
{
  bounding_box_t box;  /**< the expected bounding box for changes */
  INTN image_width;    /**< the images' width */
  INTN image_height;   /**< the images' height */
  image_t *before;     /**< the "before" image */
  image_t *after;      /**< the "after" image */
} graphics_difftest_t;



void init_graphics_difftest_ex(graphics_difftest_t *difftest, INTN width, INTN height, UINT32 bgcol);
void init_graphics_difftest(graphics_difftest_t *difftest, INTN width, INTN height);

void find_bounding_box_for_changes(graphics_difftest_t *difftest);
void assert_box_equals(bounding_box_t *box, INTN left, INTN top, INTN right, INTN bottom, CHAR16 *message);
void assert_differences_within_box(graphics_difftest_t *difftest, INTN min_width, INTN max_width, INTN min_height, INTN max_height, CHAR16 *message);

void reset_bounding_box(bounding_box_t *box);
void reset_graphics_difftest(graphics_difftest_t *difftest);

void destroy_graphics_difftest(graphics_difftest_t *difftest);

