/**
 * Command line parameter parser
 *
 * \author Richard Nusser
 * \copyright 2017 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \link https://github.com/rinusser/UEFIStarter
 */
#ifndef __CMDLINE_H
#define __CMDLINE_H

#include <Uefi.h>


#define RV_HELP 0x10000000

typedef enum
{
  ARG_BOOL=1,
  ARG_INT,
  ARG_DOUBLE,
  ARG_STRING,
} argument_type;

typedef union {
  double dbl;
  UINT64 uint64;
  CHAR16 *wcstr;
} double_uint64_t; //TODO: rename

typedef BOOLEAN validate_argument_f(double_uint64_t);

typedef struct {
  double_uint64_t value;
  argument_type type;
  validate_argument_f *validator_func;
  CHAR16 *name;
  CHAR16 *helptext;
} cmdline_argument_t;

typedef struct {
  CHAR16 *name;
  UINTN count;
  cmdline_argument_t *list;
} cmdline_argument_group_t;

#define ARG_GROUP(VARIABLE,LIST,DESCRIPTION) cmdline_argument_group_t VARIABLE={DESCRIPTION,sizeof(LIST)/sizeof(cmdline_argument_t),LIST};


//TODO: move these
BOOLEAN wctype_int(CHAR16 *string);
BOOLEAN wctype_float(CHAR16 *string);
double _wcstof(CHAR16 *str);

#define DOUBLE_RANGE_VALIDATOR(FUNC,FIELD,MIN,MAX) static BOOLEAN FUNC(double_uint64_t v) { return validate_double_range(v,FIELD,MIN,MAX); }
BOOLEAN validate_double_range(double_uint64_t v, CHAR16 *field, double min, double max);

#define INT_RANGE_VALIDATOR(FUNC,FIELD,MIN,MAX) static BOOLEAN FUNC(double_uint64_t v) { return validate_uint64_range(v,FIELD,MIN,MAX); }
BOOLEAN validate_uint64_range(double_uint64_t v, CHAR16 *field, UINT64 min, UINT64 max);

EFI_STATUS parse_parameters(INTN argc, CHAR16 **argv, UINTN group_count, VA_LIST groups);
void print_help_text(UINTN group_count, VA_LIST groups);


#endif
