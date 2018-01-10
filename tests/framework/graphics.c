/** \file
 * Assertions and utilities for graphics tests
 *
 * Some of the graphics tests work by looking for changes between two images, then asserting whether those changes are
 * within a given rectangular region, the bounding box.
 *
 * \author Richard Nusser
 * \copyright 2017-2018 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_tests_asserts
 */

#include <Library/BaseMemoryLib.h>
#include "../../include/string.h"
#include "graphics.h"
#include "asserts.h"


/**
 * Initializes a bounding box.
 *
 * \param box the bounding box to reset
 */
void reset_bounding_box(bounding_box_t *box)
{
  box->left=-1;
  box->right=-1;
  box->top=-1;
  box->bottom=-1;
}

/**
 * Resets a graphics difference test.
 * Initializes the expected bounding box and resets the "after" image.
 *
 * \param difftest the difference test to reset
 */
void reset_graphics_difftest(graphics_difftest_t *difftest)
{
  reset_bounding_box(&difftest->box);
  CopyMem(difftest->after->data,difftest->before->data,difftest->image_width*difftest->image_height*BYTES_PER_PIXEL);
}

/**
 * Initializes a graphics difference test.
 *
 * \param difftest the difference test to reset
 * \param width    the compared images' width
 * \param height   the compared images' height
 * \param bgcol    the background color to paint "before" and "after" images with
 */
void init_graphics_difftest_ex(graphics_difftest_t *difftest, INTN width, INTN height, UINT32 bgcol)
{
  difftest->image_width=width;
  difftest->image_height=height;
  difftest->before=create_image(width,height);
  difftest->after=create_image(width,height);
  SetMem32(difftest->before->data,width*height*BYTES_PER_PIXEL,bgcol);
  reset_graphics_difftest(difftest);
}

/**
 * Initializes a default graphics difference test.
 *
 * \param difftest the difference test to reset
 * \param width    the compared images' width
 * \param height   the compared images' height
 */
void init_graphics_difftest(graphics_difftest_t *difftest, INTN width, INTN height)
{
  init_graphics_difftest_ex(difftest,width,height,DIFFTEST_DEFAULT_BACKGROUND_UINT32);
}

/**
 * Destroys a graphics difference test structure.
 *
 * \param difftest the difference test to destroy
 */
void destroy_graphics_difftest(graphics_difftest_t *difftest)
{
  free_image(difftest->before);
  free_image(difftest->after);
}

/**
 * Compares "before" and "after" images of a difference test and determines the bounding box changes happened in.
 *
 * \param difftest the difference test to update
 */
void find_bounding_box_for_changes(graphics_difftest_t *difftest)
{
  INTN x, y;
  INTN y_offset;
  UINT32 *s1=(UINT32 *)difftest->before->data;
  UINT32 *s2=(UINT32 *)difftest->after->data;
  reset_bounding_box(&difftest->box);

  for(y=0;y<difftest->image_height;y++)
  {
    y_offset=y*difftest->image_width;
    for(x=0;x<difftest->image_width;x++)
    {
      if(s1[y_offset+x]!=s2[y_offset+x])
      {
        if(difftest->box.left==-1 || difftest->box.left>x)
          difftest->box.left=x;
        if(difftest->box.right==-1 || difftest->box.right<x)
          difftest->box.right=x;
        if(difftest->box.top==-1)
          difftest->box.top=y;
        if(difftest->box.bottom==-1 || difftest->box.bottom<y)
          difftest->box.bottom=y;
      }
    }
  }
}

/**
 * Asserts a bounding box matches expected values.
 *
 * \param box     the actual bounding box
 * \param left    the expected left coordinate
 * \param top     the expected top coordinate
 * \param right   the expected right coordinate
 * \param bottom  the expected bottom coordinate
 * \param message a descriptive message of what's being tested
 */
void assert_box_equals(bounding_box_t *box, INTN left, INTN top, INTN right, INTN bottom, CHAR16 *message)
{
  assert_intn_equals(left,  box->left,  memsprintf(L"%s, left",  message));
  assert_intn_equals(top,   box->top,   memsprintf(L"%s, top",   message));
  assert_intn_equals(right, box->right, memsprintf(L"%s, right", message));
  assert_intn_equals(bottom,box->bottom,memsprintf(L"%s, bottom",message));
}

/**
 * Asserts the bounding box of changes is within given ranges of width and height.
 *
 * \param difftest   the difference test to check
 * \param min_width  the smallest allowed bounding box width
 * \param max_width  the largest allowed bounding box width
 * \param min_height the smallest allowed bounding box height
 * \param max_height the largest allowed bounding box height
 * \param message    a descriptive message of what's being tested
 */
void assert_differences_within_box(graphics_difftest_t *difftest, INTN min_width, INTN max_width, INTN min_height, INTN max_height, CHAR16 *message)
{
  find_bounding_box_for_changes(difftest);
  assert_intn_in_closed_interval(min_width, max_width, difftest->box.right-difftest->box.left+1,memsprintf(L"%s width",message));
  assert_intn_in_closed_interval(min_height,max_height,difftest->box.bottom-difftest->box.top+1,memsprintf(L"%s height",message));
}
