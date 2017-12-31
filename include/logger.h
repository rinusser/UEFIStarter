/** \file
 * Logging facility, supports verbosity levels
 *
 * \author Richard Nusser
 * \copyright 2017 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_lib_logger
 */

#ifndef __LOGGER_H
#define __LOGGER_H

#include <Uefi.h>

/** helper macro to log a TRACE message containing the current file and line */
#define TRACE_HERE LOG.trace(L"%a#%d",__FILE__,__LINE__);

/**
 * helper macro to quickly log a WARN message if an EFI result is anything but EFI_SUCCESS
 *
 * \param TEXT the text to log
 */
#define ON_ERROR_WARN(TEXT) if(result!=EFI_SUCCESS) LOG.warn(TEXT);

/**
 * helper macro to quickly log an ERROR message and return if an EFI result is anything but EFI_SUCCESS
 *
 * \param TEXT the text to log
 * \param RV   the value to return
 */
#define ON_ERROR_RETURN(TEXT,RV) if(result!=EFI_SUCCESS) \
  { \
    LOG.error(L"%s() returned status %d (%r)",TEXT,result,result); \
    return RV; \
  }


/**
 * the list of log levels
 */
typedef enum
{
  OFF=0,  /**< disable logging:      la-la-la, I can't hear you */
  ERROR,  /**< error messages:       something definitely broke */
  WARN,   /**< warnings:             something may have broken */
  INFO,   /**< informative messages: just in case something breaks */
  DEBUG,  /**< debug messages:       something breaks and I'm trying to fix it */
  TRACE   /**< trace messages:       something breaks and I don't know what or where */
} LOGLEVEL;

/**
 * function pointer for log output functions
 *
 * \param level the log level to output at
 * \param msg the log message to print, as UTF-16
 */
typedef void logger_print_function_t(CHAR16 *level, CHAR16 *msg);

/**
 * function pointer for logging functions to be invoked directly
 *
 * \param fmt the message's format string, as UTF-16 - takes same format as Print() functions
 * \param ... any additional arguments matching the format string placeholders
 */
typedef void EFIAPI logger_function_t(UINT16 *fmt, ...);

/** logging facility data type, contains logging function pointer for each log level */
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
