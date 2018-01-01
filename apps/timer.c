/** \file
 * This application shows how to wait for events and register a timer.
 *
 * \author Richard Nusser
 * \copyright 2017 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_apps
 */

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include "../include/console.h"
#include "../include/string.h"
#include "../include/timestamp.h"
#include "../include/logger.h"


/**
 * Callback function for the timer.
 * Contains a Print() call that may crash a Virtualbox VM. In current builds it does not, but it used to for some
 * reason. Calling Print() in a timer callback is just for demonstration anyway.
 *
 * \param event   (unused) the triggered timer event
 * \param context (unused) the triggered timer event's context data
 */
void callback(EFI_EVENT event, void *context)
{
  Print(L" 2c\n");
}

/**
 * Prints the current date/time info and timing capabilities of the UEFI runtime service's GetTime().
 * This shows resolutions of 1Hz on all tested systems, the nanosecond field seems to be always empty. This makes
 * GetTime() useless for timing e.g. animation frames.
 */
void do_gettime_tests()
{
  EFI_STATUS result;
  EFI_TIME time;
  EFI_TIME_CAPABILITIES caps;

  result=gST->RuntimeServices->GetTime(&time,&caps);
  ON_ERROR_RETURN(L"GetTime",);

  Print(L"time: %04d-%02d-%02d %02d:%02d:%02d.%09d (tz=%d, dst=%d)\n",time.Year,time.Month,time.Day,time.Hour,time.Minute,time.Second,time.Nanosecond,time.TimeZone,time.Daylight);
  Print(L"date: resolution=%dHz, accuracy=%dppm, tozero=%d\n",caps.Resolution,caps.Accuracy/1000000,caps.SetsToZero);
}


/**
 * Compares the internal get_timestamp() against the UEFI service's timer functions.
 * The timer interval is 1s, so the internal timestamp should increase by 1 for each Hz of CPU frequency.
 */
void do_timestamp_tests()
{
  UINT64 start, end, diff;
  EFI_STATUS result;
  EFI_EVENT event;
  UINTN index;
  unsigned int tc;
  unsigned int loop_count=10;
  UINT64 min, max, middle;
  float accuracy;

  result=gST->BootServices->CreateEvent(EVT_TIMER,TPL_CALLBACK,NULL,NULL,&event);
  ON_ERROR_RETURN(L"CreateEvent",);

  result=gST->BootServices->SetTimer(event,TimerPeriodic,1000*1000*10);
  ON_ERROR_RETURN(L"SetTimer",);

  LOG.info(L"prepared event, waiting for 1+%d intervals...",loop_count);

  result=gST->BootServices->WaitForEvent(1,&event,&index);
  ON_ERROR_RETURN(L"WaitForEvent",);

  start=get_timestamp();
  Print(L"start timestamp: %016lX (%ld)\n",start,start);

  for(tc=0;tc<loop_count;tc++)
  {
    result=gST->BootServices->WaitForEvent(1,&event,&index);
    ON_ERROR_RETURN(L"WaitForEvent",);
    end=get_timestamp();

    diff=end-start;
    Print(L"interval:        %016lX (%ld)\n",diff,diff);
    start=end;
    if(tc>0)
    {
      if(diff<min)
        min=diff;
      if(diff>max)
        max=diff;
    }
    else
    {
      min=diff;
      max=diff;
    }
  }
  middle=(max+min)/2; //not an actual average!
  accuracy=100.0*(max-min)/middle;
  Print(L"ticks per second: min=%lu, max=%lu, diff=%lu => timer accuracy: +-%s%%\n",min,max,max-min,ftowcs(accuracy));
}

/**
 * Registers events to wait for.
 *
 * One of the events is a callback function, it will get called asynchronously whenever the event is fired. The called
 * handler currently prints to the console - this used to hang when executed inside a VirtualBox VM.
 */
void do_event_tests()
{
  EFI_STATUS result;
  EFI_EVENT events[2];
  UINTN tc;
  UINTN index;
  UINT64 start_ts, end_ts;

  init_timestamps();

  result=gST->BootServices->CreateEvent(EVT_TIMER,TPL_CALLBACK,NULL,NULL,&events[0]);
  ON_ERROR_RETURN(L"CreateEvent",);

  result=gST->BootServices->CreateEvent(EVT_TIMER|EVT_NOTIFY_SIGNAL,TPL_CALLBACK,(EFI_EVENT_NOTIFY)callback,NULL,&events[1]);
  ON_ERROR_RETURN(L"CreateEvent",);
//  uefi_call_or_return(,ST->BootServices->CreateEvent,5,EVT_TIMER|EVT_NOTIFY_WAIT,TPL_CALLBACK,callback,NULL,&events[1]);

  result=gST->BootServices->SetTimer(events[0],TimerPeriodic,100*1000*10);
  ON_ERROR_RETURN(L"SetTimer",);
  result=gST->BootServices->SetTimer(events[1],TimerPeriodic,500*1000*10);
  ON_ERROR_RETURN(L"SetTimer",);

  start_ts=get_timestamp();
  Print(L"waiting for events (c..callback, w..wait)...\n");
  for(tc=0;tc<20;tc++)
  {
    LOG.trace(L"waiting...");
    result=gST->BootServices->WaitForEvent(1,&events[0],&index);
    ON_ERROR_RETURN(L"WaitForEvent",);
    Print(L" %dw%s",index+1,index>0?L"\n":L"");
  }
  Print(L"\n");
  end_ts=get_timestamp();
  Print(L"waited for %ss\n",ftowcs(timestamp_diff_seconds(start_ts,end_ts)));

  result=gST->BootServices->CloseEvent(events[0]);
  ON_ERROR_RETURN(L"CloseEvent",);
  result=gST->BootServices->CloseEvent(events[1]);
  ON_ERROR_RETURN(L"CloseEvent",);
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
  if((rv=init(argc,argv,0))!=EFI_SUCCESS)
    return rv;

  do_gettime_tests();
  do_timestamp_tests();
  do_event_tests();

  shutdown();
  return EFI_UNSUPPORTED;
}
