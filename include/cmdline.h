/** \file
 * Command line parameter parser
 *
 * \author Richard Nusser
 * \copyright 2017 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_lib_cmdline
 */

#ifndef __CMDLINE_H
#define __CMDLINE_H

#include <Uefi.h>

/** EFI_STATUS compatible return value, indicating user wanted the help screen */
#define RV_HELP 0x10000000

/** the supported command line argument types */
typedef enum
{
  ARG_BOOL=1, /**< boolean */
  ARG_INT,    /**< integer */
  ARG_DOUBLE, /**< double */
  ARG_STRING, /**< UTF-16 string */
} argument_type;

/**
 * one parsed command line value
 * \TODO rename this to something generic, strings have been added since the first version
 */
typedef union {
  double dbl;    /**< double variant */
  UINT64 uint64; /**< UINT64 variant */
  CHAR16 *wcstr; /**< UTF-16 string variant */
} double_uint64_t;

/** function pointer type for command line argument validators */
typedef BOOLEAN validate_argument_f(double_uint64_t);

/** model for one command line argument */
typedef struct {
  double_uint64_t value;                /**< the parsed value */
  argument_type type;                   /**< the argument's type */
  validate_argument_f *validator_func;  /**< the validator function, may be NULL */
  CHAR16 *name;                         /**< the argument's name */
  CHAR16 *helptext;                     /**< a user-friendly help text */
} cmdline_argument_t;

/** model for group of command line arguments */
typedef struct {
  CHAR16 *name;              /**< the group's name or short description, may be NULL */
  UINTN count;               /**< the number of entries in the group */
  cmdline_argument_t *list;  /**< the list of arguments in this group */
} cmdline_argument_group_t;

/**
 * shortcut macro for quickly defining an argument group
 *
 * \param VARIABLE    the group variable to define
 * \param LIST        the list of arguments, as cmdline_argument_t[]
 * \param DESCRIPTION the group's name or description, may be NULL
 */
#define ARG_GROUP(VARIABLE,LIST,DESCRIPTION) cmdline_argument_group_t VARIABLE={DESCRIPTION,sizeof(LIST)/sizeof(cmdline_argument_t),LIST};


BOOLEAN wctype_int(CHAR16 *string);
BOOLEAN wctype_float(CHAR16 *string);
double _wcstof(CHAR16 *str);

/**
 * shortcut macro for quickly defining a validator for "double" type arguments
 *
 * \param FUNC  the function name to define
 * \param FIELD the field name to use in error messages
 * \param MIN   the minimum value (inclusive)
 * \param MAX   the maximum value (inclusive)
 */
#define DOUBLE_RANGE_VALIDATOR(FUNC,FIELD,MIN,MAX) static BOOLEAN FUNC(double_uint64_t v) { return validate_double_range(v,FIELD,MIN,MAX); }

BOOLEAN validate_double_range(double_uint64_t v, CHAR16 *field, double min, double max);

/**
 * shortcut macro for quickly defining a validator for "integer" type arguments
 *
 * \param FUNC  the function name to define
 * \param FIELD the field name to use in error messages
 * \param MIN   the minimum value (inclusive)
 * \param MAX   the maximum value (inclusive)
 */
#define INT_RANGE_VALIDATOR(FUNC,FIELD,MIN,MAX) static BOOLEAN FUNC(double_uint64_t v) { return validate_uint64_range(v,FIELD,MIN,MAX); }

BOOLEAN validate_uint64_range(double_uint64_t v, CHAR16 *field, UINT64 min, UINT64 max);

EFI_STATUS parse_parameters(INTN argc, CHAR16 **argv, UINTN group_count, VA_LIST groups);
void print_help_text(UINTN group_count, VA_LIST groups);


#endif
