/** \file
 * Timing functions
 *
 * \author Richard Nusser
 * \copyright 2017-2018 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_lib_timestamp
 */

#include <Library/UefiBootServicesTableLib.h>
#include "../include/timestamp.h"
#include "../include/logger.h"
#include "../include/string.h"


//when defined the RDTSC assembly call details will be checked and logged.
//x86 specs shouldn't require this, so to increase timestamp accuracy this isn't compiled in by default.
//#define DEBUG_TIMESTAMPS

/** internal storage for the number of timestamp ticks per second */
UINT64 _rdtsc_ticks_per_second=0;


/**
 * Initializes the timestamp features.
 * This will take ~2 seconds at current settings. This function takes the naive approach and simply measures how many
 * ticks pass in 2 seconds. This tick frequency will be used later when converting ticks to seconds.
 *
 * Make sure to call this function before you attempt to determine elapsed wallclock time.
 *
 * \return 0 on success, an error code otherwise.
 */
int init_timestamps()
{
  EFI_STATUS result;
  EFI_EVENT event;
  UINT64 start, end, diff;
  UINTN index;

  result=gST->BootServices->CreateEvent(EVT_TIMER,TPL_CALLBACK,NULL,NULL,&event);
  ON_ERROR_RETURN(L"CreateEvent",-1);
  result=gST->BootServices->SetTimer(event,TimerPeriodic,1000*1000*10);
  ON_ERROR_RETURN(L"CreateEvent",-2);
  result=gST->BootServices->WaitForEvent(1,&event,&index);
  ON_ERROR_RETURN(L"CreateEvent",-3);

  start=get_timestamp();

  result=gST->BootServices->WaitForEvent(1,&event,&index);
  ON_ERROR_RETURN(L"CreateEvent",-4);

  end=get_timestamp();
  diff=end-start;
  _rdtsc_ticks_per_second=diff;

  LOG.trace(L"start timestamp: %lX (%ld)",start,start);
  LOG.trace(L"end timestamp: %lX (%ld)",end,end);
  LOG.trace(L"timestamp ticks per second: %lX (%s GHz)",diff,ftowcs(((double)diff)/1000000000));

  return 0;
}

/**
 * Fetches and returns the current timestamp.
 *
 * \return the current timestamp
 *
 * \XXX this could be made inline for improved timing accuracy
 */
UINT64 get_timestamp()
{
  UINT64 rdx,rax;
  asm volatile ("rdtsc"
    :"=d" (rdx), "=a" (rax));
#ifdef DEBUG_TIMESTAMPS
  LOG.trace(L"rdx=%lX, rax=%lX",rdx,rax);
  if((rax>0xFFFFFFFF) || (rdx>0xFFFFFFFF))
  {
    LOG.error(L"expected 32bit values in rax and rdx, got a value exceeding that");
    return 0;
  }
#endif
  return (rdx<<32)+rax;
}

/**
 * Calculates the number of seconds between two timestamps.
 * This will only work if init_timestamps() has been called first.
 *
 * \param start the start of the timestamp interval
 * \param end   the end of the timestamp interval
 * \return the difference in seconds
 *
 * \TODO this doesn't check for division by 0 errors on purpose, maybe check performance hit and add it if insignificant
 */
double timestamp_diff_seconds(UINT64 start, UINT64 end)
{
#ifdef DEBUG_TIMESTAMPS
  if(!_rdtsc_ticks_per_second)
  {
    LOG.error(L"timestamp ticks per second unknown, most likely init_timestamps() wasn't called");
    return -1.0;
  }
#endif
  return ((double)end-start)/_rdtsc_ticks_per_second;
}

/**
 * Returns the number of timestamp ticks per second
 *
 * \return timestamp ticks per second
 */
UINT64 get_timestamp_ticks_per_second()
{
  return _rdtsc_ticks_per_second;
}
