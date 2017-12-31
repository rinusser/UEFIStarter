/** \file
 * Logging facility, supports verbosity levels
 *
 * \author Richard Nusser
 * \copyright 2017 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_lib_logger
 */

#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include "../include/logger.h" //TODO: this shouldn't need to be relative, the package/module config must be incomplete


/** list of log level's printable names */
static CHAR16 *logger_level_names[]={L"",L"ERROR",L"WARN",L"INFO",L"DEBUG",L"TRACE"};

/** current log level; anything higher that won't be logged */
static int logging_threshold=TRACE;

/** numbers of logged messages for each log level */
static UINTN logger_entry_counts[TRACE+1]; //technically entry 0 (OFF) will be unused, but it's probably wiser to keep indexing simple to avoid bugs


/**
 * Fetches the current log level.
 *
 * \return the current log level
 */
LOGLEVEL get_log_level()
{
  return logging_threshold;
}

/**
 * Sets a new log level.
 * You can disable logging by calling this with log level "OFF".
 *
 * \param level the new log level to use
 * \return the previous log level
 */
LOGLEVEL set_log_level(LOGLEVEL level)
{
  LOGLEVEL prev=logging_threshold;
  logging_threshold=level;
  return prev;
}

/**
 * Default log writer: prints to text console.
 *
 * \param level the log level to log at
 * \param msg   the log message, as UTF-16
 */
void log_print(CHAR16 *level, CHAR16 *msg)
{
  Print(L"%s: %s\n",level,msg);
}

/** pointer to the current log writer */
logger_print_function_t *logger_print_func=log_print;

/**
 * internal: formats log message and calls log writer with final string to log.
 * This function uses the same format strings as the Print() functions.
 *
 * \param level   the log level to log at
 * \param message the message's format string
 * \param args    the vararg list of parameters matching matching the format string's placeholder
 */
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

/**
 * helper macro for quickly defining a callable logging function
 *
 * \param NAME  the logging function's name
 * \param LEVEL the log level to log at
 */
#define LOGGER_FORWARD_FUNC(NAME,LEVEL) static void EFIAPI NAME(CHAR16 *message, ...) \
{ \
  VA_LIST args; \
  VA_START(args,message); \
  _logger_function_va(LEVEL,message,args); \
  VA_END(args); \
}

LOGGER_FORWARD_FUNC(trace,TRACE); /**< defines "trace", the user-callable logging function for TRACE level */
LOGGER_FORWARD_FUNC(debug,DEBUG); /**< defines "debug", the user-callable logging function for DEBUG level */
LOGGER_FORWARD_FUNC(info, INFO);  /**< defines "info", the user-callable logging function for INFO level */
LOGGER_FORWARD_FUNC(warn, WARN);  /**< defines "warn", the user-callable logging function for WARN level */
LOGGER_FORWARD_FUNC(error,ERROR); /**< defines "error", the user-callable logging function for ERROR level */

/**
 * The global logging facility.
 *
 * This contains function pointers for each log level, so users can call e.g. LOG.debug() to log a debug message.
 */
const loggers_t LOG={
  .trace=trace,
  .debug=debug,
  .info=info,
  .warn=warn,
  .error=error
};

/**
 * Resets all log levels' message counts.
 * Used e.g. by the test framework to detect which tests generated errors.
 */
void reset_logger_entry_counts()
{
  unsigned int tc=0;
  for(tc=OFF;tc<=TRACE;tc++)
    logger_entry_counts[tc]=0;
}

/**
 * Gets a log level's message count.
 *
 * \param level the log level to get the count for
 * \return the number of logged messages at the given level since the last reset
 */
UINTN get_logger_entry_count(LOGLEVEL level)
{
  return logger_entry_counts[level];
}

/**
 * Immediately halts the UEFI environment.
 *
 * Use this to e.g. debug issues in graphical animations where it's vital to stop everything before the target gets
 * painted over.
 *
 * Technically the emitted interrupt 3 is usually used for debuggers but the UEFI specification requires INT 3 to be
 * unassigned by default, thus halting everything. If you use this function with a debugger attached this will probably
 * trigger the debugger instead.
 */
void kill()
{
  asm volatile ("int $3");
}

/**
 * Sets a new log writer.
 * Use this to replace the built-in text mode writer, e.g. to output log messages in a graphical environment
 *
 * \param func the new log writer function
 * \return the previous log writer function
 */
logger_print_function_t *set_logger_function(logger_print_function_t *func)
{
  logger_print_function_t *previous=logger_print_func;
  logger_print_func=func;
  return previous;
}
