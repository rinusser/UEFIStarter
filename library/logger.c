/**
 * Logging facility, supports verbosity levels
 *
 * \author Richard Nusser
 * \copyright 2017 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \link https://github.com/rinusser/UEFIStarter
 */
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include "../include/logger.h" //TODO: this shouldn't need to be relative, the package/module config must be incomplete


static CHAR16 *logger_level_names[]={L"",L"ERROR",L"WARN",L"INFO",L"DEBUG",L"TRACE"};
static int logging_threshold=TRACE;
static UINTN logger_entry_counts[TRACE+1]; //technically entry 0 (OFF) will be unused, but it's probably wiser to keep indexing simple to avoid bugs


LOGLEVEL get_log_level()
{
  return logging_threshold;
}

LOGLEVEL set_log_level(LOGLEVEL level)
{
  LOGLEVEL prev=logging_threshold;
  logging_threshold=level;
  return prev;
}

void log_print(CHAR16 *level, CHAR16 *msg)
{
  Print(L"%s: %s\n",level,msg);
}

logger_print_function_t *logger_print_func=log_print;

static void _logger_function_va(LOGLEVEL level, UINT16 *message, VA_LIST args)
{
  CHAR16 *msg;

  logger_entry_counts[level]++;
  if(level>logging_threshold)
    return;

  msg=CatVSPrint(NULL,message,args);
  logger_print_func(logger_level_names[level],msg);
  FreePool(msg);
}


#define LOGGER_FORWARD_FUNC(NAME,LEVEL) static void EFIAPI NAME(CHAR16 *message, ...) \
{ \
  VA_LIST args; \
  VA_START(args,message); \
  _logger_function_va(LEVEL,message,args); \
  VA_END(args); \
}

LOGGER_FORWARD_FUNC(trace,TRACE);
LOGGER_FORWARD_FUNC(debug,DEBUG);
LOGGER_FORWARD_FUNC(info, INFO);
LOGGER_FORWARD_FUNC(warn, WARN);
LOGGER_FORWARD_FUNC(error,ERROR);

const loggers_t LOG={
  .trace=trace,
  .debug=debug,
  .info=info,
  .warn=warn,
  .error=error
};

void reset_logger_entry_counts()
{
  unsigned int tc=0;
  for(tc=OFF;tc<=TRACE;tc++)
    logger_entry_counts[tc]=0;
}

UINTN get_logger_entry_count(LOGLEVEL level)
{
  return logger_entry_counts[level];
}

void kill()
{
  asm volatile ("int $3");
}

logger_print_function_t *set_logger_function(logger_print_function_t *func)
{
  logger_print_function_t *previous=logger_print_func;
  logger_print_func=func;
  return previous;
}
