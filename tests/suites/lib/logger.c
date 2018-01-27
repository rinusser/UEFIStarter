/** \file
 * Tests for the logging facility.
 *
 * \author Richard Nusser
 * \copyright 2017-2018 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_lib_logger
 */

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <UEFIStarter/core.h>
#include <UEFIStarter/tests/tests.h>

/**
 * The number of entries in the log counter array.
 * Index is of type LOGLEVEL: "OFF" is 0, "TRACE" starts at 1, so we need to reserve the index 0 entry.
 */
#define LOG_COUNT_ENTRIES 6

/**
 * Internal storage for the _counting_logger() function.
 * Contains the number of entries issued per log level.
 */
static UINTN _log_counts[LOG_COUNT_ENTRIES];

/**
 * Reset's the log message counters to 0.
 */
static void _reset_log_counts()
{
  UINTN tc;
  for(tc=0;tc<LOG_COUNT_ENTRIES;tc++)
    _log_counts[tc]=0;
}

/**
 * A (mostly silent) log printer: instead of showing messages somewhere it just counts how many log entries were
 * generated at which log level.
 *
 * \param level the entry's log level
 * \param msg   the entry's log message
 */
static void _counting_logger(CHAR16 *level, CHAR16 *msg)
{
  /** \XXX there _has_ to be a better way to match log levels.. maybe add numeric level to print function? */
  if(StrCmp(level,L"ERROR")==0)
    _log_counts[ERROR]++;
  else if(StrCmp(level,L"WARN")==0)
    _log_counts[WARN]++;
  else if(StrCmp(level,L"INFO")==0)
    _log_counts[INFO]++;
  else if(StrCmp(level,L"DEBUG")==0)
    _log_counts[DEBUG]++;
  else if(StrCmp(level,L"TRACE")==0)
    _log_counts[TRACE]++;
  else
    ErrorPrint(L"unhandled level \"%s\"\n",level);
}

/**
 * Assertion for log counters.
 * Compares all counters to expected values.
 *
 * \param error the expected number of entries at level ERROR
 * \param warn  the expected number of entries at level WARN
 * \param info  the expected number of entries at level INFO
 * \param debug the expected number of entries at level DEBUG
 * \param trace the expected number of entries at level TRACE
 */
static void _assert_log_counts(UINTN error, UINTN warn, UINTN info, UINTN debug, UINTN trace)
{
  assert_intn_equals(error,_log_counts[ERROR],L"error count");
  assert_intn_equals(warn, _log_counts[WARN], L"warn count");
  assert_intn_equals(info, _log_counts[INFO], L"info count");
  assert_intn_equals(debug,_log_counts[DEBUG],L"debug count");
  assert_intn_equals(trace,_log_counts[TRACE],L"trace count");
}

/**
 * Makes sure the log level threshold works.
 *
 * \test a log event at the current log level should be printed
 * \test a log event above the current log level should be printed
 * \test a log event below the current log level should be ignored
 * \test when the current log level is OFF all log events should be ignored
 */
void test_logger()
{
  _reset_log_counts();

  //log message at level, should have +1 entry at level
  set_log_level(INFO);
  set_logger_function(_counting_logger);
  LOG.info(L"info");
  _assert_log_counts(0,0,1,0,0);

  //repeat with log level 1 higher: should still output
  LOG.warn(L"warn");
  _assert_log_counts(0,1,1,0,0);

  //repeat with log level 1 lower than level: should not output anymore
  LOG.debug(L"debug");
  _assert_log_counts(0,1,1,0,0);

  //setting level to OFF should disable all levels
  set_log_level(OFF);
  LOG.trace(L"trace");
  LOG.debug(L"debug");
  LOG.info(L"info");
  LOG.warn(L"warn");
  LOG.error(L"error");
  _assert_log_counts(0,1,1,0,0);
}


/**
 * Test runner for this group.
 * Gets called via the generated test runner.
 *
 * \return whether the test group was executed
 */
BOOLEAN run_logger_tests()
{
  INIT_TESTGROUP(L"logger");
  RUN_TEST(test_logger,L"logger");
  FINISH_TESTGROUP();
}
