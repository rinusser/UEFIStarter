/**
 * Logging facility, supports verbosity levels
 *
 * \author Richard Nusser
 * \copyright 2017 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \link https://github.com/rinusser/UEFIStarter
 */
#ifndef __LOGGER_H
#define __LOGGER_H

#include <Uefi.h>

#define TRACE_HERE LOG.trace(L"%a#%d",__FILE__,__LINE__);

#define ON_ERROR_WARN(TEXT) if(result!=EFI_SUCCESS) LOG.warn(TEXT);

#define ON_ERROR_RETURN(TEXT,RV) if(result!=EFI_SUCCESS) \
  { \
    LOG.error(L"%s() returned status %d (%r)",TEXT,result,result); \
    return RV; \
  }


typedef enum
{
  OFF=0,
  ERROR,
  WARN,
  INFO,
  DEBUG,
  TRACE
} LOGLEVEL;

typedef void logger_print_function_t(CHAR16 *level, CHAR16 *msg);
typedef void EFIAPI logger_function_t(UINT16 *,...);

typedef struct
{
  logger_function_t *trace;
  logger_function_t *debug;
  logger_function_t *info;
  logger_function_t *warn;
  logger_function_t *error;
} loggers_t;

extern const loggers_t LOG;


LOGLEVEL get_log_level();
LOGLEVEL set_log_level(LOGLEVEL level);

void reset_logger_entry_counts();
UINTN get_logger_entry_count(LOGLEVEL level);

void kill();

logger_print_function_t *set_logger_function(logger_print_function_t *func);

#endif
