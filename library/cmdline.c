/** \file
 * Command line parameter parser
 *
 * \author Richard Nusser
 * \copyright 2017-2018 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_lib_cmdline
 */

#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include "../include/cmdline.h"
#include "../include/logger.h"
#include "../include/string.h"

/**
 * (internal) performs check whether string is numeric
 *
 * \param string        the string to check, as UTF-16
 * \param allow_decimal whether decimal numbers are OK
 * \return whether the string looks like a number
 *
 * \TODO the wctype* functions should be replaced by a built-in, or moved to a separate lib
 */
static BOOLEAN _wctype_number(CHAR16 *string, BOOLEAN allow_decimal)
{
  unsigned int tc=0, digits=0, decimals_points=0;
  BOOLEAN decimal_point_last=FALSE;
  CHAR16 ch;
  if(!string || string[0]==0)
    return FALSE;
  if(string[0]==L'-')
    tc++;
  while((ch=string[tc++])!=0)
  {
    decimal_point_last=FALSE;
    if(ch=='.')
    {
      if(!allow_decimal || decimals_points++>0 || digits<1)
        return FALSE;
      decimal_point_last=TRUE;
    }
    else if(ch<L'0' || ch>L'9')
      return FALSE;
    digits++;
  }
  if(digits<1)
    return FALSE;
  if(allow_decimal&&decimal_point_last)
    return FALSE;
  return TRUE;
}

/**
 * Checks whether a string looks like an integer.
 *
 * \param string the string to check, as UTF-16
 * \return whether it's an integer
 */
BOOLEAN wctype_int(CHAR16 *string)
{
  return _wctype_number(string,FALSE);
}

/**
 * Checks whether a string looks like a number.
 *
 * \param string the string to check, as UTF-16
 * \return whether it's a decimal number
 */
BOOLEAN wctype_float(CHAR16 *string)
{
  return _wctype_number(string,TRUE);
}

/**
 * Converts a numeric string into a double value.
 * Errors will produce an ERROR level log message.
 *
 * \param str the string to convert
 * \return the parsed number; -1.0 on error
 *
 * \TODO replace _wcstof() with built-in, or move to separate file
 */
double _wcstof(CHAR16 *str)
{
  double rv=0;
  BOOLEAN after_decimal=FALSE;
  BOOLEAN negative=FALSE;
  BOOLEAN is_digit;
  CHAR16 ch;
  unsigned int tc=0, digit_value;
  double fraction=10.0;

  if(str==NULL || str[0]==0)
  {
    LOG.error(L"_wcstof: cannot parse NULL or empty string");
    return -1.0;
  }
  LOG.debug(L"_wcstof: parsing \"%s\"",str);
  if(str[0]==L'-')
  {
    negative=TRUE;
    tc++;
  }

  while((ch=str[tc++])!=0)
  {
    is_digit=ch>=L'0'&&ch<=L'9';
    digit_value=ch-L'0';
    if(after_decimal)
    {
      if(ch==L'.')
      {
        LOG.error(L"_wcstof: encountered more than 1 decimal point");
        return -1.0;
      }
      else if(is_digit)
      {
        rv=rv+digit_value/fraction;
        fraction*=10;
      }
      else
      {
        LOG.error(L"_wcstof: invalid string: %s, failing character: 0x%02X",str,ch);
        return -1.0;
      }
    }
    else
    {
      if(ch==L'.')
        after_decimal=TRUE;
      else if(is_digit)
        rv=rv*10+digit_value;
      else
      {
        LOG.error(L"_wcstof: invalid string: %s, failing character: 0x%02X",str,ch);
        return -1.0;
      }
    }
  }

  return negative?-rv:rv;
}


/**
 * Checks whether a double command line parameter is within boundaries.
 *
 * \param v     the command line parameter
 * \param field the field name to use in error messages, as UTF-16
 * \param min   the lowest allowed value (inclusive)
 * \param max   the highest allowed value (inclusive)
 * \return whether the parameter is within the boundaries
 *
 * \TODO add test
 */
BOOLEAN validate_double_range(double_uint64_t v, CHAR16 *field, double min, double max)
{
  if(v.dbl>=min && v.dbl<=max)
    return TRUE;
  LOG.error(L"%s must be between %s and %s",field,ftowcs(min),ftowcs(max));
  return FALSE;
}

/**
 * Checks whether a uint64 command line parameter is within boundaries.
 *
 * \param v     the command line parameter
 * \param field the field name to use in error messages, as UTF-16
 * \param min   the lowest allowed value (inclusive)
 * \param max   the highest allowed value (inclusive)
 * \return whether the parameter is within the boundaries
 *
 * \TODO add test
 */
BOOLEAN validate_uint64_range(double_uint64_t v, CHAR16 *field, UINT64 min, UINT64 max)
{
  if(v.uint64>=min && v.uint64<=max)
    return TRUE;
  LOG.error(L"%s must be between %ld and %ld",field,min,max);
  return FALSE;
}


/**
 * data type to map a log level to a command line argument
 */
typedef struct
{
  LOGLEVEL level; /**< the log level */
  CHAR16 *str;    /**< the command line argument, as UTF-16 */
} logger_args_mapping_t;

/**
 * log level -> command line argument mappings
 */
static logger_args_mapping_t logger_args[]=
{
  { TRACE, L"-trace" },
  { DEBUG, L"-debug" },
  { INFO,  L"-info" },
  { WARN,  L"-warn" },
  { ERROR, L"-error" },
  { OFF,   L"-no-log" }
};

/**
 * (internal) prints the help text for a command line argument group
 *
 * \param arguments the argument group
 */
static void _print_argument_group_help(cmdline_argument_group_t *arguments)
{
  unsigned int tc;
  CHAR16 *typetext;
  CHAR16 *defaulttext;
  CHAR16 *padding=L"                                ";
  unsigned int max_pad_length=StrLen(padding);
  unsigned int max_arg_length=0;
  unsigned int arg_length, pad_start;

  if(arguments->name)
    Print(L"\n%s:\n",arguments->name);

  /** \TODO could mind required parameters (currently numbers for double/int args) for pad length, then remove typetext padding in Print() below */
  /** \TODO combine pad length for different groups that were combined, e.g. builtin and custom audio options */
  for(tc=0;tc<arguments->count;tc++)
  {
    arg_length=StrLen(arguments->list[tc].name);
    if(arg_length>max_arg_length)
      max_arg_length=arg_length;
  }

  for(tc=0;tc<arguments->count;tc++)
  {
    defaulttext=NULL;
    arg_length=StrLen(arguments->list[tc].name);
    if(arg_length+max_pad_length<max_arg_length)
      pad_start=0;
    else
      pad_start=max_pad_length-1-max_arg_length+arg_length;
    LOG.trace(L"arg_length=%d, max_pad_length=%d, max_arg_length=%d, pad_start=%d",arg_length,max_pad_length,max_arg_length,pad_start);
    switch(arguments->list[tc].type)
    {
      case ARG_BOOL:
        typetext=L"";
        break;
      case ARG_INT:
        typetext=L"<integer>";
        defaulttext=CatSPrint(NULL,L" [default: %d]",arguments->list[tc].value.uint64);
        break;
      case ARG_DOUBLE:
        typetext=L"<decimal>";
        defaulttext=CatSPrint(NULL,L" [default: %s]",ftowcs(arguments->list[tc].value.dbl));
        break;
      case ARG_STRING:
        typetext=L"<string>";
        if(arguments->list[tc].value.wcstr!=NULL)
          defaulttext=CatSPrint(NULL,L" [default: %s]",arguments->list[tc].value.wcstr);
        break;
      default:
        LOG.error(L"unhandled argument type: %d",arguments->list[tc].type);
    }
    Print(L"  %s %9s%s %s%s\n",arguments->list[tc].name,typetext,padding+pad_start,arguments->list[tc].helptext,defaulttext?defaulttext:L"");
    if(defaulttext!=NULL)
      FreePool(defaulttext);
  }
}

/**
 * Outputs the help text for all argument groups, including the built-in ones.
 *
 * \param argument_group_count the number of argument groups passed
 * \param args                 the vararg list of argument groups (cmdline_argument_group_t *)
 */
void print_help_text(UINTN argument_group_count, VA_LIST args)
{
  unsigned int tc;
  cmdline_argument_group_t *arguments;

  Print(L"General options:\n\
  -help    This text\n\
\n\
Logging options:\n\
  -trace   Set log threshold to TRACE\n\
  -debug   Set log threshold to DEBUG\n\
  -info    Set log threshold to INFO\n\
  -warn    Set log threshold to WARN\n\
  -error   Set log threshold to ERROR\n\
  -no-log  Disable logging\n");

  for(tc=0;tc<argument_group_count;tc++)
  {
    arguments=VA_ARG(args,cmdline_argument_group_t *);
    _print_argument_group_help(arguments);
  }
  Print(L"\n");
}

/**
 * Checks if all command line arguments have been parsed successfully.
 * The parser actually shortens the processed strings to 0 length, this checks for that.
 *
 * \param argc the number of command-line arguments
 * \param argv the list of command-line arguments, as UTF-16
 * \return whether all parameters have been handled
 */
BOOLEAN check_no_arguments_remaining(INTN argc, CHAR16 **argv)
{
  unsigned int tc;
  unsigned int errors=0;
  for(tc=1;tc<argc;tc++)
  {
    if(argv[tc][0]!=0)
    {
      LOG.error(L"unhandled parameter \"%s\"",argv[tc]);
      errors++;
    }
  }
  return errors<1;
}

/**
 * (internal) parses logger/help parameters from command line
 *
 * \param argc the number of command-line arguments
 * \param argv the list of command-line arguments, as UTF-16
 * \return whether the help screen was requested
 */
static BOOLEAN _parse_logger_args(INTN argc, CHAR16 **argv)
{
  unsigned int logger_args_count=sizeof(logger_args)/sizeof(logger_args_mapping_t);
  LOGLEVEL log_level=INFO;
  unsigned int tc, td;
  BOOLEAN help=FALSE;

  reset_logger_entry_counts();

  for(tc=0;tc<argc;tc++)
  {
    if(StrCmp(argv[tc],L"-help")==0)
    {
      help=TRUE;
      continue;
    }
    for(td=0;td<logger_args_count;td++)
    {
      if(StrCmp(argv[tc],logger_args[td].str)==0)
      {
        log_level=logger_args[td].level;
        argv[tc][0]=0;
      }
    }
  }
  set_log_level(log_level);

  return !help;
}

/**
 * (internal) fills argument group structure with all matching command line arguments
 *
 * \param argc      the number of command-line arguments
 * \param argv      the list of command-line arguments, as UTF-16
 * \param arguments the argument group to parse values into
 * \return whether all matching parameters have been parsed successfully
 */
static BOOLEAN _parse_parameter_group(INTN argc, CHAR16 **argv, cmdline_argument_group_t *arguments)
{
  unsigned int tc, td;
  INTN bytes;
  CHAR16 *ptr;

  if(arguments->count>1000)
  {
    LOG.error(L"argument group count is %u, can't be right",arguments->count);
    return FALSE;
  }

  for(tc=0;tc<argc;tc++)
  {
    for(td=0;td<arguments->count;td++)
    {
      if(StrCmp(argv[tc],arguments->list[td].name)==0)
      {
        switch(arguments->list[td].type)
        {
          case ARG_BOOL:
            arguments->list[td].value.uint64=1;
            break;
          case ARG_INT:
            if(tc>(argc-2) || !wctype_int(argv[tc+1]))
            {
              LOG.error(L"argument %s must be followed by a number",arguments->list[td].name);
              return FALSE;
            }
            arguments->list[td].value.uint64=StrDecimalToUint64(argv[tc+1]);
            argv[tc++][0]=0;
            break;
          case ARG_DOUBLE:
            if(tc>(argc-2) || !wctype_float(argv[tc+1]))
            {
              LOG.error(L"argument %s must be followed by a decimal number",arguments->list[td].name);
              return FALSE;
            }
            arguments->list[td].value.dbl=_wcstof(argv[tc+1]);
            argv[tc++][0]=0;
            break;
          case ARG_STRING:
            if(tc>(argc-2))
            {
              LOG.error(L"argument %s must be followed by a string",arguments->list[td].name);
              return FALSE;
            }
            bytes=StrSize(argv[tc+1]);
            ptr=AllocatePool(bytes);
            if(ptr==NULL)
            {
              LOG.error(L"could not allocate memory for string parameter");
              return FALSE;
            }
            CopyMem(ptr,argv[tc+1],bytes);
            arguments->list[td].value.wcstr=ptr;
            argv[tc++][0]=0;
            break;
          default:
            LOG.error(L"unhandled argument type: %d",arguments->list[td].type);
            return FALSE;
        }
        argv[tc][0]=0;
        if(arguments->list[td].validator_func && !arguments->list[td].validator_func(arguments->list[td].value))
          return FALSE;
        break;
      }
    }
  }
  return TRUE;
}

/**
 * Parses command-line parameters
 *
 * \param argc        the number of command-line arguments
 * \param argv        the list of command-line arguments, as UTF-16
 * \param group_count the number of command line argument groups passed
 * \param args        the vararg list of command line groups (as cmdline_argument_group_t *)
 * \return an EFI status code
 */
EFI_STATUS parse_parameters(INTN argc, CHAR16 **argv, UINTN group_count, VA_LIST args)
{
  unsigned int tc;
  cmdline_argument_group_t *group;

  if(!_parse_logger_args(argc,argv))
  {
    print_help_text(group_count,args);
    return RV_HELP;
  }

  for(tc=0;tc<group_count;tc++)
  {
    group=VA_ARG(args,cmdline_argument_group_t *);
    if(!group || !_parse_parameter_group(argc,argv,group))
      return EFI_INVALID_PARAMETER;
  }

  if(!check_no_arguments_remaining(argc,argv))
    return EFI_INVALID_PARAMETER;

  return EFI_SUCCESS;
}
