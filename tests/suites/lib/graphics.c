/** \file
 * Tests for graphics functions.
 *
 * \author Richard Nusser
 * \copyright 2017-2018 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_lib_graphics
 */

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <math.h>
#include <UEFIStarter/core.h>
#include <UEFIStarter/graphics.h>
#include <UEFIStarter/tests/tests.h>


/******************
 * Image resources
 ***/


//netpbm: common

/** data type for netpbm image parser tests */
typedef struct
{
  UINTN data_length;                     /**< the length of input data, in bytes */
  char *data;                            /**< the input data to parse */
  UINTN width;                           /**< the expected image width */
  UINTN height;                          /**< the expected image height */
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *pixels; /**< the expected pixel data */
} parse_image_data_testcase_t;

/** helper data type for easier access to file_contents_t's memory content */
typedef union {
  char _raw[256];           /**< raw access */
  file_contents_t contents; /**< structured access */
} contents_buffer_t;

/** internal image data buffer for netpbm parser tests */
static contents_buffer_t _file_contents_buffer;

/**
 * internal: assembles a file_contents_t structure manually
 *
 * \param size the content length, in bytes
 * \param data the content data
 * \return the file_contents_t structure
 */
static file_contents_t *assemble_file_contents(unsigned int size, char *data)
{
  if(size>sizeof(contents_buffer_t)-sizeof(file_contents_t))
  {
    LOG.error(L"data size too large, can't handle %d bytes",size);
    return NULL;
  }
  ZeroMem(_file_contents_buffer._raw,sizeof(contents_buffer_t));
  _file_contents_buffer.contents.data_length=size;
  CopyMem(_file_contents_buffer.contents.data,data,size);
  return &_file_contents_buffer.contents;
}

/**
 * Runs an individual netpbm test case.
 *
 * \param testcase the test case to run
 * \param parser   the parser function to test
 */
void do_parse_image_test(parse_image_data_testcase_t *testcase, image_t *(*parser)(file_contents_t *))
{
  file_contents_t *contents=assemble_file_contents(testcase->data_length,testcase->data);
  image_t *image=parser(contents);
  INTN count, tc;

  if(!assert_not_null(image,L"could not parse image"))
    return;
  if(!assert_intn_equals(testcase->width,image->width,L"width") || !assert_intn_equals(testcase->height,image->height,L"height"))
    return;

  count=image->width*image->height;
  for(tc=0;tc<count;tc++)
    assert_pixel(testcase->pixels[tc],image->data[tc],L"content");

  free_image(image);
}

/**
 * shortcut macro to quickly run a netpbm test case
 *
 * \param TYPE the image format to test ("ppm", "pgm" or "pbm")
 */
#define RUN_PARSE_IMAGE_TESTS(TYPE) \
  INTN tc, count; \
  count=sizeof(TYPE ## _testcases)/sizeof(parse_image_data_testcase_t); \
  for(tc=0;tc<count;tc++) \
    do_parse_image_test(TYPE ## _testcases+tc,parse_ ## TYPE ## _image_data);


//netpbm: PPM format

/** the PPM image data to parse for tests */
char ppm_data[]={0x50,0x36,0x0a,0x23,0x20,0x78,0x0a,0x32,0x20,0x33,0x0a,0x32,0x35,0x35,0x0a,0xed,0x1c,0x24,0xff,0xf2,0x00,0x00,0xa2,0xe8,0xb5,0xe6,0x1d,0x00,0x00,0x00,0xff,0xff,0xff};

/** the PPM image's expected pixels */
EFI_GRAPHICS_OUTPUT_BLT_PIXEL expected_ppm_pixels[]=
{
  {36,28,237,0},
  {0,242,255,0},
  {232,162,0,0},
  {29,230,181,0},
  {0,0,0,0},
  {255,255,255,0},
};

/** the test case for the PPM parser test */
parse_image_data_testcase_t ppm_testcases[]=
{
  {sizeof(ppm_data),ppm_data,2,3,expected_ppm_pixels},
};

/**
 * Makes sure parse_ppm_image_data() works.
 *
 * \test parse_ppm_image_data() reads correct dimensions and pixel values
 */
void test_parse_ppm_image_data()
{
  RUN_PARSE_IMAGE_TESTS(ppm);
}


//netpbm: PGM format

/** the PGM image data to parse for tests */
char pgm_data[]={0x50,0x35,0x0a,0x23,0x0a,0x32,0x20,0x32,0x0a,0x32,0x35,0x35,0x0a,0x00,0x7f,0xff,0xc3};

/** the PGM image's expected pixels */
EFI_GRAPHICS_OUTPUT_BLT_PIXEL expected_pgm_pixels[]={
  {0,0,0,0},
  {127,127,127,0},
  {255,255,255,0},
  {195,195,195,0},
};

/** the test case for the PGM parser test */
parse_image_data_testcase_t pgm_testcases[]=
{
  {sizeof(pgm_data),pgm_data,2,2,expected_pgm_pixels},
};

/**
 * Makes sure parse_pgm_image_data() works.
 *
 * \test parse_pgm_image_data() reads correct dimensions and pixel values
 */
void test_parse_pgm_image_data()
{
  RUN_PARSE_IMAGE_TESTS(pgm);
}


//netpbm: PBM format

/** the PBM image data to parse for tests */
char pbm_data[]={0x50,0x34,0x0a,0x23,0x23,0x0a,0x31,0x37,0x20,0x32,0x0a,0xf2,0x1b,0xff,0x90,0xde,0x7f};

#define B {0,0,0,0}       /**< helper macro to quickly create black pixel data */
#define W {255,255,255,0} /**< helper macro to quickly create white pixel data */

/** the PBM image's expected pixels */
EFI_GRAPHICS_OUTPUT_BLT_PIXEL expected_pbm_pixels[]={B,B,B,B,W,W,B,W,W,W,W,B,B,W,B,B,B,B,W,W,B,W,W,W,W,B,B,W,B,B,B,B,W,W};

/** the test case for the PBM parse test */
parse_image_data_testcase_t pbm_testcases[]=
{
  {sizeof(pbm_data),pbm_data,17,2,expected_pbm_pixels},
};

/**
 * Makes sure parse_pbm_image_data() works.
 *
 * \test parse_pbm_image_data() reads correct dimensions and pixel values
 */
void test_parse_pbm_image_data()
{
  RUN_PARSE_IMAGE_TESTS(pbm);
}


/*********************
 * Image manipulation
 ***/

/** data type for image rotation testcases */
typedef struct
{
  float theta; /**< the rotation angle to test, in radians */
  INTN dx;     /**< the horizontal direction to expect changes in  */
  INTN dy;     /**< the vertical direction to expect changes in */
} rotation_testcase_t;

/** test cases for test_rotate_image() */
rotation_testcase_t rotation_testcases[]=
{
  { 0.0000, 1, 0},
  { 0.7854, 1, 1},
  { 1.5708, 0, 1},
  { 2.3562,-1, 1},
  { 3.1416,-1, 0},
  { 3.9270,-1,-1},
  { 4.7124, 0,-1},
  { 5.4978, 1,-1},
  { 6.2832, 1, 0},
  {-2.3562,-1,-1},
};

EFI_GRAPHICS_OUTPUT_BLT_PIXEL rotation_color_yarp={200,150,30,0}; /**< "on" color for rotation tests */
EFI_GRAPHICS_OUTPUT_BLT_PIXEL rotation_color_narp={80,240,110,0}; /**< "off" color for rotation tests */

/**
 * internal: checks a rotated image at 9 specific pixels to see if it was rotated correctly.
 * Here's an example image being rotated. The original image is on the left, the result after rotating by 90° clockwise
 * on the right:
 *
 *     ...........         ...........
 *     .         .         .         .
 *     .         .         .         .
 *     .    *****.   ==>   .    *    .
 *     .         .         .    *    .
 *     ...........         .....*.....
 *      original             90° CW
 *
 * The rotated images are tested at 9 points. These points are in a 3x3 grid, with the center pixel in the middle of
 * the image. The center point is the center of a new coordinate system, with x going right and y going down. Test
 * points are marked with `°`:
 *
 *     ...........
 *     .         .
 *     .  ° ° °  .
 *     .  ° ° °  . --> x
 *     .  ° ° °  .
 *     ...........
 *          |
 *          V
 *          y
 *
 * Thus, the top left point is at coordinates (-1,-1), the center left point at (-1,0) and so on.
 *
 * Because of the contents of the original and rotated image the tests work by asserting that:
 *
 * 1. the center point always has the foreground color
 * 2. a specific test point has the foreground color (+- a small margin)
 * 3. the other 7 test points have the background color.
 *
 * The coordinates to expect a foreground color pixel at are passed in parameters "dx" and "dy".
 *
 * The scale of the 3x3 grid can be adjusted with the "r" parameter.
 *
 * \param image      the rotated image to check
 * \param r          the distance scale to use for checks, in pixels
 * \param dx         the horizontal direction to expect change in
 * \param dy         the vertical direction to expect change in
 * \param msg_prefix a description of what's being tested, to include in error messages
 */
static void _do_rotation_checks(image_t *image, INTN r, INTN dx, INTN dy, CHAR16 *msg_prefix)
{
  INTN x, y;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL actual;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL expected;

  for(y=-1;y<=1;y++)
  {
    for(x=-1;x<=1;x++)
    {
      actual=image->data[(15+r*y)*31+15+r*x];
      expected=((y==dy&&x==dx) || (y==0&&x==0))?rotation_color_yarp:rotation_color_narp;
      assert_pixel_near(expected,3,actual,memsprintf(L"theta=%s: r=%d, dx=%d, dy=%d at x=%d, y=%d",msg_prefix,r,dx,dy,x,y));
    }
  }
}

/**
 * Makes sure rotate_image() works.
 * The _do_rotation_checks() function documentation illustrates how this is asserted.
 *
 * \test rotate_image() rotates images clockwise for positive angles and counter-clockwise for negative angles
 */
void test_rotate_image()
{
  image_t *original=create_image(31,31);
  image_t *rotated=create_image(31,31);
  INTN tc, x, y, count;
  UINT32 yarp=*((INT32 *)&rotation_color_yarp);
  UINT32 narp=*((INT32 *)&rotation_color_narp);
  UINT32 *data=(UINT32 *)original->data;

  set_graphics_sin_func(sin);
  set_graphics_cos_func(cos);

  for(y=0;y<31;y++)
    for(x=0;x<31;x++)
      data[y*31+x]=(y>=14&&y<=16&&x>=14)?yarp:narp;

  _do_rotation_checks(original,4,1,0,L"original");

  count=sizeof(rotation_testcases)/sizeof(rotation_testcase_t);
  for(tc=0;tc<count;tc++)
  {
    LOG.trace(L"tc=%d",tc);
    rotate_image(original->data,rotated->data,15,rotation_testcases[tc].theta);
    LOG.trace(L"  rotated");
    _do_rotation_checks(rotated,4,rotation_testcases[tc].dx,rotation_testcases[tc].dy,ftowcs(rotation_testcases[tc].theta));
    LOG.trace(L"  checked");
  }

  free_image(original);
  free_image(rotated);
}


/*********************
 * Color manipulation
 ***/

/** test data for pixel interpolation */
static EFI_GRAPHICS_OUTPUT_BLT_PIXEL _interpolation_data[]={{0,0,0,0},{0,0,255,0},{123,5,0,0},{0,255,0,0},{255,0,0,0},{50,200,164,0}};

/**
 * Makes sure interpolate_4px works.
 *
 * \test interpolate_4px() handles all 4 corners correctly
 * \test interpolate_4px() handles all 5 mid points correctly
 * \test interpolate_4px() handles arbitrary points correctly
 */
void test_interpolate_4px()
{
  SPRITE data=_interpolation_data;

  assert_pixel(data[0],interpolate_4px(data,3,0,0),L"left top pixel");
  assert_pixel(data[1],interpolate_4px(data,3,1,0),L"right top pixel");
  assert_pixel(data[3],interpolate_4px(data,3,0,1),L"left bottom pixel");
  assert_pixel(data[4],interpolate_4px(data,3,1,1),L"right bottom pixel");

  assert_pixel_values(127,0,  0  ,0,interpolate_4px(data,3,0.5,0.0),L"center top");
  assert_pixel_values(127,0,  127,0,interpolate_4px(data,3,1.0,0.5),L"right middle");
  assert_pixel_values(0,  127,127,0,interpolate_4px(data,3,0.5,1.0),L"center bottom");
  assert_pixel_values(0,  127,0,  0,interpolate_4px(data,3,0.0,0.5),L"left middle");
  assert_pixel_values(63, 63, 63, 0,interpolate_4px(data,3,0.5,0.5),L"center middle");

  assert_pixel_values(47, 47, 15, 0,interpolate_4px(data,3,0.25,0.25),L"x=0.25, y=0.25");
}

/**
 * Makes sure interpolate_2px() works.
 *
 * \test interpolate_2px() uses second pixel for ratio 1.0
 * \test interpolate_2px() handles arbitrary points correctly
 */
void test_interpolate_2px()
{
  SPRITE data=_interpolation_data+1;

  assert_pixel(data[1],interpolate_2px(data,1),L"right pixel");
  assert_pixel_values(50,4,98,0,interpolate_2px(data,0.8),L"x=0.8");
}


/********
 * Fonts
 ***/

/**
 * internal: creates a glyph list for use in tests.
 *
 * \return a list of glyphs
 */
static glyph_list_t *_get_parse_glyphs_font()
{
  glyph_list_t *glyphs;
  image_t *image;
  CHAR16 *text=L"AB\n c";
  INTN width=2*8;
  INTN height=2*15;
  COLOR white={255,255,255,0};
  COLOR gray75={191,191,191,0};
  COLOR gray50={127,127,127,0};

  image=create_image(width,height);
  ZeroMem(image->data,width*height*sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
  image->data[0]=white;
  image->data[7]=gray50;
  image->data[16*width+8]=gray75;
  glyphs=parse_glyphs(image,text);
  free_image(image);

  return glyphs;
}

/**
 * Makes sure parse_glyphs() works.
 *
 * \test parse_glyphs() initializes the glyph list correctly
 * \test parse_glyphs() splits an input text of 4 characters into 4 glyphs
 * \test parse_glyphs() reads glyphs correctly when given a multiline input string
 */
void test_parse_glyphs()
{
  UINT8 expected_glyph_data[8*15];
  glyph_list_t *glyphs=_get_parse_glyphs_font();

  assert_intn_equals(1,glyphs->memory_pages,L"memory pages");
  assert_intn_equals(4,glyphs->glyph_count,L"glyph count");

  assert_intn_equals(L'A',glyphs->glyphs[0].chr,L"glyph 1 char");
  ZeroMem(expected_glyph_data,8*15);
  expected_glyph_data[0]=255;
  expected_glyph_data[7]=127;
  assert_uint8_array(8*15,expected_glyph_data,glyphs->glyphs[0].data,L"glyph 1 data");

  assert_intn_equals(L'B',glyphs->glyphs[1].chr,L"glyph 2 char");
  ZeroMem(expected_glyph_data,8*15);
  assert_uint8_array(8*15,expected_glyph_data,glyphs->glyphs[1].data,L"glyph 2 data");

  assert_intn_equals(L' ',glyphs->glyphs[2].chr,L"glyph 3 char");
  assert_uint8_array(8*15,expected_glyph_data,glyphs->glyphs[2].data,L"glyph 3 data");

  assert_intn_equals(L'c',glyphs->glyphs[3].chr,L"glyph 4 char");
  ZeroMem(expected_glyph_data,8*15);
  expected_glyph_data[8]=191;
  assert_uint8_array(8*15,expected_glyph_data,glyphs->glyphs[3].data,L"glyph 4 data");

  free_glyphs(glyphs);
}


/**
 * Makes sure draw_text() works.
 *
 * \test draw_text() writes text at the correct position
 * \test draw_text() blends semi-transparent glyphs onto background
 */
void test_draw_text()
{
  glyph_list_t *glyphs=_get_parse_glyphs_font();
  image_t *target=create_image(40,40);
  COLOR col={40,127,255,0};
  INTN x, y;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *px;

  for(y=0;y<40;y++)
  {
    for(x=0;x<40;x++)
    {
      px=target->data+y*40+x;
      px->Red=0;
      px->Green=64+(x%2)*128;
      px->Blue=64+(y%2)*128;
      px->Reserved=0;
    }
  }

  assert_pixel_values(0,192,192,0,target->data[3*40+3],L"original value");
  draw_text(target->data,40,glyphs,3,2,col,L"cA\nA ");
  assert_pixel_values(191,143,78, 0,target->data[3*40+3],L"letter c opaque");
  assert_pixel_values(0,  64, 192,0,target->data[3*40+4],L"letter c transparent");
  assert_pixel_values(0,  192,64, 0,target->data[4*40+3],L"letter c transparent");
  assert_pixel_values(0,  64, 64, 0,target->data[4*40+4],L"letter c transparent");

  assert_pixel(col,target->data[2*40+11],L"letter A top right");

  assert_pixel(col,target->data[17*40+3],L"letter A bottom left");

  free_image(target);
  free_glyphs(glyphs);
}


/*********
 * Runner
 ***/

/**
 * Test runner for this group.
 * Gets called via the generated test runner.
 *
 * \return whether the test group was executed
 */
BOOLEAN run_graphics_tests()
{
  INIT_TESTGROUP(L"graphics");

  RUN_TEST(test_parse_ppm_image_data,L"PPM image parser");
  RUN_TEST(test_parse_pgm_image_data,L"PGM image parser");
  RUN_TEST(test_parse_pbm_image_data,L"PBM image parser");

  RUN_TEST(test_rotate_image,L"arbitrary image rotation");

  RUN_TEST(test_interpolate_2px,L"linear interpolation");
  RUN_TEST(test_interpolate_4px,L"bilinear interpolation");

  RUN_TEST(test_parse_glyphs,L"font parser");
  RUN_TEST(test_draw_text,L"font blending");

  FINISH_TESTGROUP();
}
