/** \file
 * This application shows animated snow in text mode.
 *
 * Press the reft/right arrow keys for wind.
 * Press 'q' to quit.
 *
 * \author Richard Nusser
 * \copyright 2017-2018 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_apps
 */

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <UEFIStarter/core.h>

//if defined, this will output the current crosswind speed
//#define SHOW_CROSS_SPEED

#define FLAKE_SCREEN_WIDTH _screen_width               /**< the snowflake canvas width */
#define FLAKE_SCREEN_HEIGHT (_screen_height-2)         /**< the snowflake canvas height */
#define FLAKE_DEFAULT_DURATION_SECONDS 60              /**< the default time (in seconds) this application should run */
#define FLAKE_DEFAULT_UPDATE_INTERVAL_MILLISECONDS 100 /**< the default update interval (in ms) between frames */
#define FLAKE_DEFAULT_COUNT 100                        /**< the default number of flakes, including off-screen ones */
#define FLAKE_DEFAULT_GROUND_LIFETIME 10               /**< the default lifetime (in seconds) of snowflakes on the ground */
#define FLAKE_CROSS_STEP 0.3               /**< the default wind acceleration step size */
#define FLAKE_MAX_CROSS_SPEED 2.0          /**< the default maximum wind speed */
#define FLAKE_CROSS_SPEED_FALLOFF_MULT 0.8 /**< the default wind speed falloff multiplier */
#define FLAKE_CROSS_SPEED_BASE 0.1         /**< the default base wind speed */

/** command-line arguments */
static cmdline_argument_t _argument_list[] = {
  {{uint64:FLAKE_DEFAULT_DURATION_SECONDS},            ARG_INT,   NULL,L"-duration",           L"Duration (in seconds) snow should fall"},
  {{uint64:FLAKE_DEFAULT_COUNT},                       ARG_INT,   NULL,L"-count",              L"Number of flakes generated (about half of them on screen)"},
  {{uint64:FLAKE_DEFAULT_UPDATE_INTERVAL_MILLISECONDS},ARG_INT,   NULL,L"-interval",           L"Interval (in milliseconds) between frames"},
  {{uint64:FLAKE_DEFAULT_GROUND_LIFETIME},             ARG_INT,   NULL,L"-lifetime",           L"Lifetime (in seconds) of flakes on ground"},
  {{dbl:FLAKE_CROSS_STEP},                             ARG_DOUBLE,NULL,L"-cross-step",         L"Crosswind increment step"},
  {{dbl:FLAKE_MAX_CROSS_SPEED},                        ARG_DOUBLE,NULL,L"-max-cross-speed",    L"Maximum crosswind speed"},
  {{dbl:FLAKE_CROSS_SPEED_FALLOFF_MULT},               ARG_DOUBLE,NULL,L"-cross-falloff-multi",L"Crosswind speed falloff multiplier (keep <=1.0)"},
  {{dbl:FLAKE_CROSS_SPEED_BASE},                       ARG_DOUBLE,NULL,L"-base-cross-speed",   L"Base crosswind speed"}
};

#define ARG_SECONDS         _argument_list[0].value.uint64 /**< helper macro to access application lifetime parameter */
#define ARG_FLAKE_COUNT     _argument_list[1].value.uint64 /**< helper macro to access flake count parameter */
#define ARG_UPDATE_INTERVAL _argument_list[2].value.uint64 /**< helper macro to access update interval parameter */
#define ARG_GROUND_LIFETIME _argument_list[3].value.uint64 /**< helper macro to access ground lifetime parameter */
#define ARG_CROSS_STEP               _argument_list[4].value.dbl  /**< helper macro to access wind acceleration step size parameter */
#define ARG_MAX_CROSS_SPEED          _argument_list[5].value.dbl  /**< helper macro to access max wind speed parameter */
#define ARG_CROSS_SPEED_FALLOFF_MULT _argument_list[6].value.dbl  /**< helper macro to access wind speed falloff parameter */
#define ARG_CROSS_SPEED_BASE         _argument_list[7].value.dbl  /**< helper macro to access base wind speed parameter */


/** command-line argument group */
static ARG_GROUP(_arguments,_argument_list,L"Weather options (in a UEFI boot time executable, mind you)");

static UINTN _screen_width;           /**< internal storage for screen's total width (in characters) */
static UINTN _screen_height;          /**< internal storage for screen's total height (in characters) */
static UINTN _ground_lifetime_frames; /**< internal storage for snowflake's ground lifetime (in number of frames) */


/**
 * This generates a very-pseudo random number.
 * Don't use this function for anything but trivial applications, the numbers are nowhere near random.
 *
 * \return a number random enough for this application
 *
 * \TODO There's a GetRandomNumber32() function in EDK2 but it seems to segfault all the time, see what's up
 */
UINT32 random()
{
  return get_timestamp();
/*  UINT32 value;
  GetRandomNumber32(&value);
  return value;*/
}

/** data type describing a snowflake */
typedef struct {
  double column;   /**< the text column this flake currently is in (as double, so movement across screen is smoother) */
  double speed;    /**< the flake's speed */
  int previous_x;  /**< the flake's previous text column */
  int previous_y;  /**< the flake*s previous height */
  int y_offset;    /**< the flake*s current height */
  int time_offset; /**< the flake's time of creation (as frame number) */
} flake_t;

/**
 * Initializes a snowflake.
 *
 * \param flake       the flake to initialize/reset
 * \param time_offset the time of the flake's creation
 */
void init_flake(flake_t *flake, int time_offset)
{
  flake->column=(double)(random()%(FLAKE_SCREEN_WIDTH*2))-(double)FLAKE_SCREEN_WIDTH/2; /** \TODO use screen height and max cross speed to accurately calculate this */
  flake->speed=((random()%60)+40)/100.0;
  flake->previous_y=-10000;
  flake->previous_x=-10000;
  flake->y_offset=random()%20;
  flake->time_offset=time_offset;
  LOG.debug(L"initialized flake: col=%5s, speed=%s, y_offset=%d",ftowcs(flake->column),ftowcs(flake->speed),flake->y_offset);
}

/**
 * Handles snow flake movement for a single frame of animation.
 *
 * This function doesn't handle flakes on the ground: those are actually just visual artifacts. The snowflakes fall
 * "through" the ground, get reset to above the screen and start from there. The flakes seen on the ground were just
 * never removed during animation. Instead the flake's landing time is written to land_times, update_ground() will
 * draw over those flakes that landed more than the configured ground lifetime ago.
 *
 * \param flakes      the snowflakes' data
 * \param iteration   the animation loop iteration number
 * \param cross_speed the current wind speed
 * \param land_times  the list of flakes' last landing times, one entry per text column
 * \return the number of currently moving snowflakes; on error the (1-based) snowflake number the error happened at
 */
int update_flakes(flake_t flakes[], int iteration, float cross_speed, int land_times[])
{
  EFI_STATUS result;
  int x, y;
  int td, active_count=0;
  for(td=0;td<ARG_FLAKE_COUNT;td++)
  {
    flakes[td].column+=cross_speed*flakes[td].speed*flakes[td].speed;
    y=flakes[td].speed*(iteration-flakes[td].time_offset);
    LOG.trace(L"flake %02d: y=%d, previous_y=%d",td,y,flakes[td].previous_y);
    if(y>FLAKE_SCREEN_HEIGHT+flakes[td].y_offset)
    {
      init_flake(&flakes[td],iteration);
      continue;
    }
    active_count++;
    if(y<flakes[td].y_offset)
    {
      flakes[td].previous_y=y;
      continue;
    }
    y-=flakes[td].y_offset;
    if(y==flakes[td].previous_y && x==flakes[td].previous_x)
      continue;
    x=flakes[td].column;
    if(flakes[td].previous_y>=flakes[td].y_offset && flakes[td].previous_y<FLAKE_SCREEN_HEIGHT+flakes[td].y_offset && flakes[td].previous_x>=0 && flakes[td].previous_x<FLAKE_SCREEN_WIDTH)
    {
      result=gST->ConOut->SetCursorPosition(gST->ConOut,flakes[td].previous_x,flakes[td].previous_y-flakes[td].y_offset);
      ON_ERROR_RETURN(L"SetCursorPosition",active_count);
      Print(L" ");
    }
    if(x>=0 && x<FLAKE_SCREEN_WIDTH)
    {
      result=gST->ConOut->SetCursorPosition(gST->ConOut,x,y);
      ON_ERROR_RETURN(L"SetCursorPosition",active_count);

      if(flakes[td].speed>0.9)
        color_print(15,L"*");
//      else if(flakes[td].speed<0.5) //this was intended to add another layer of depth:
//        color_print(8,L"*");        //very slow flakes should be far away, but the gray on black looks fugly
      else
        color_print(7,L"*");

      if(y==FLAKE_SCREEN_HEIGHT)
      {
        land_times[x]=iteration;
        LOG.debug(L"flake %d landed",td);
      }
    }
    flakes[td].previous_y=y+flakes[td].y_offset;
    flakes[td].previous_x=x;
  }

  //the color_print() calls above allocate pool memory entries, there's currently an upper limit for these, so let's
  //free them before the memory tracker runs out of entries.
  free_pool_memory_entries();

  return active_count;
}

/**
 * Reads keyboard input and returns the pressed key.
 * Some terminals and terminal multiplexers generate multiple input events for just one keystroke (e.g. escape
 * sequences), this will return the last of them.
 *
 * \return the pressed key
 */
EFI_INPUT_KEY read_key()
{
  EFI_STATUS result;
  EFI_INPUT_KEY keys[10];
  int tc;

  for(tc=0;tc<10;tc++)
  {
    result=gST->ConIn->ReadKeyStroke(gST->ConIn,&keys[tc]);
    if(result!=EFI_SUCCESS)
      break;
  }
  return keys[0];
}

/**
 * Debugging function: prints the current wind speed if SHOW_CROSS_SPEED is defined
 *
 * \param speed the wind speed to print
 */
void print_cross_speed(double speed)
{
  EFI_STATUS result;
  result=gST->ConOut->SetCursorPosition(gST->ConOut,40,FLAKE_SCREEN_HEIGHT+1);
  ON_ERROR_RETURN(L"SetCursorPosition",);
  Print(L"%5s",ftowcs(speed));
}

/**
 * Removes any expired snowflakes on ground.
 *
 * \param iteration  the current animation iteration number
 * \param land_times the last flake landing times per text column
 */
void update_ground(int iteration, int land_times[])
{
  EFI_STATUS result;
  int tc;

  for(tc=0;tc<FLAKE_SCREEN_WIDTH;tc++)
  {
    if(iteration-land_times[tc]!=_ground_lifetime_frames)
      continue;
    result=gST->ConOut->SetCursorPosition(gST->ConOut,tc,FLAKE_SCREEN_HEIGHT);
    ON_ERROR_RETURN(L"SetCursorPosition",);
    Print(L" ");
  }
}

/**
 * Main animation loop.
 */
void do_print_snow()
{
  EFI_STATUS result;
  flake_t flakes[ARG_FLAKE_COUNT];
  int tc;
  EFI_EVENT events[2];
  UINTN index;
  UINTN duration=ARG_SECONDS*1000/ARG_UPDATE_INTERVAL;
  EFI_INPUT_KEY key;
  double cross_speed=ARG_CROSS_SPEED_BASE;
  int land_times[FLAKE_SCREEN_WIDTH];

  for(tc=0;tc<FLAKE_SCREEN_WIDTH;tc++)
    land_times[tc]=-100000;
  _ground_lifetime_frames=ARG_GROUND_LIFETIME*1000/ARG_UPDATE_INTERVAL;

  for(tc=0;tc<ARG_FLAKE_COUNT;tc++)
    init_flake(&flakes[tc],0);

  result=gST->BootServices->CreateEvent(EVT_TIMER,TPL_CALLBACK,NULL,NULL,&events[1]);
  ON_ERROR_RETURN(L"CreateEvent",);
  result=gST->BootServices->SetTimer(events[1],TimerPeriodic,ARG_UPDATE_INTERVAL*1000*10);
  ON_ERROR_RETURN(L"SetTimer",);
  gST->ConOut->EnableCursor(gST->ConOut,FALSE);
  events[0]=gST->ConIn->WaitForKey;

  result=gST->ConOut->SetCursorPosition(gST->ConOut,0,FLAKE_SCREEN_HEIGHT+1);
  ON_ERROR_RETURN(L"SetCursorPosition",);
  Print(L"[Q]uit, [L/Rarr] wind");
  for(tc=0;tc<duration;tc++)
  {
    result=gST->BootServices->WaitForEvent(2,events,&index);
    ON_ERROR_RETURN(L"WaitForEvent",);
    if(index==0)
    {
      tc--;
      key=read_key();
      if(key.UnicodeChar==L'q')
        break;
      switch(key.ScanCode)
      {
        case SCAN_LEFT:
          cross_speed-=ARG_CROSS_STEP;
          if(cross_speed<-ARG_MAX_CROSS_SPEED)
            cross_speed=-ARG_MAX_CROSS_SPEED;
          break;
        case SCAN_RIGHT:
          cross_speed+=ARG_CROSS_STEP;
          if(cross_speed>ARG_MAX_CROSS_SPEED)
            cross_speed=ARG_MAX_CROSS_SPEED;
          break;
      }
#ifdef SHOW_CROSS_SPEED
      print_cross_speed(cross_speed);
#endif
      continue;
    }
    if(update_flakes(flakes,tc,cross_speed,land_times)<1)
      break;
    update_ground(tc,land_times);
    cross_speed=(cross_speed-ARG_CROSS_SPEED_BASE)*ARG_CROSS_SPEED_FALLOFF_MULT+ARG_CROSS_SPEED_BASE;
#ifdef SHOW_CROSS_SPEED
    print_cross_speed(cross_speed);
#endif
  }
  LOG.debug(L"finished after %d iterations",tc+1);
  result=gST->ConOut->SetCursorPosition(gST->ConOut,0,FLAKE_SCREEN_HEIGHT+1);
  ON_ERROR_RETURN(L"SetCursorPosition",);
  gST->ConOut->EnableCursor(gST->ConOut,TRUE);
}

/**
 * Clears the screen.
 */
void clear()
{
  gST->ConOut->ClearScreen(gST->ConOut);
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
  EFI_STATUS result;
  if((result=init(argc,argv,1,&_arguments))!=EFI_SUCCESS)
    return result;

  result=gST->ConOut->QueryMode(gST->ConOut,gST->ConOut->Mode->Mode,&_screen_width,&_screen_height);
  ON_ERROR_RETURN(L"ConOut->QueryMode",result);

  clear();
  do_print_snow();

  shutdown();
  return EFI_SUCCESS;
}
