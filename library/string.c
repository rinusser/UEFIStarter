/**
 * String functions missing from EDK
 *
 * \author Richard Nusser
 * \copyright 2017 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \link https://github.com/rinusser/UEFIStarter
 */
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include "../include/memory.h"
#include "../include/string.h"
#include "../include/logger.h"

//this will generate a lot of TRACE output
//#define DEBUG_STRSTR

#define FTOWCS_MIN_VALUE -1000000000.0
#define FTOWCS_MAX_VALUE  1000000000.0


CHAR16 *ftowcs(double value/*, unsigned int decimals*/)
{
  BOOLEAN negative=FALSE;
  INT64 left;
  UINT64 right;

  if(value<FTOWCS_MIN_VALUE)
  {
    LOG.error(L"double value too low to convert");
    return NULL;
  }
  if(value>FTOWCS_MAX_VALUE)
  {
    LOG.error(L"double value too high to convert");
    return NULL;
  }

  LOG.trace(L"  value<0: %s",value<0?L"yes":L"no");
  if(value<0)
  {
    negative=TRUE;
    value=-value;
  }
  left=(INT64)value;
  right=(UINT64)((value-left)*1000);
  LOG.trace(L"  left=%ld, right=%ld",left,right);
  if(right%10>=5)
  {
    right+=5;
    LOG.trace(L"  left=%ld, right=%ld (right value needs rounding up, increased it by 5)",left,right);
    if(right>=1000)
    {
      right-=1000;
      left+=1;
      LOG.trace(L"  left=%ld, right=%ld (right value wrapped around, decreased it and increased left value by 1)",left,right);
    }
  }
  right/=10;
  return memsprintf(L"%s%ld.%02ld",negative?L"-":L"",left,right);
}

UINT64 atoui64(char *str)
{
  UINT64 rv=0;
  UINTN tc;

  if(str==NULL || str[0]==0)
  {
    LOG.error(L"atoui64: cannot parse NULL or empty string");
    return -1;
  }
  LOG.debug(L"atoui64: parsing \"%a\"",str);
  for(tc=0;tc<20;tc++)
  {
    LOG.trace(L"atoui64: tc=%d, current=%ld",tc,rv);
    if(str[tc]==0)
      break;
    if(str[tc]<'0' || str[tc]>'9')
    {
      LOG.error(L"invalid string to convert: %a, failing char:0x%02X",str,str[tc]);
      return -1;
    }
    rv=rv*10+str[tc]-'0';
  }
  return rv;
}

BOOLEAN ctype_whitespace(char ch)
{
  return ch=='\t' || ch=='\n' || ch=='\r' || ch==' ';
}

CHAR16 *sprint_status(CHAR16 *function, EFI_STATUS code)
{
  return memsprintf(L"%s() returned status %d (%r)",function,code,code);
}

void print_status(CHAR16 *function, EFI_STATUS code) //XXX see if this is still required
{
  Print(L"%s\n",sprint_status(function,code));
}


CHAR16* EFIAPI memsprintf(const CHAR16 *fmt, ...)
{
  CHAR16 *str;
  VA_LIST args;

  if(fmt==NULL)
    LOG.warn(L"memsprintf(): format string should not be NULL");

  VA_START(args,fmt);
  str=CatVSPrint(NULL,fmt,args);
  VA_END(args);

  track_pool_memory(str);

  return str;
}


UINTN split_string(CHAR16 ***list, CHAR16 *input, CHAR16 separator)
{
  UINTN count=1;
  UINTN length, tc;
  CHAR16 **ptrs;

  if(list==NULL)
  {
    LOG.error(L"list pointer can't be NULL");
    return 0;
  }

  if(input==NULL)
  {
    *list=NULL;
    return 0;
  }

  length=StrLen(input);
  for(tc=0;tc<length;tc++)
    if(input[tc]==separator)
      count++;
  ptrs=AllocateZeroPool(count*sizeof(CHAR16 *));
  *list=ptrs;

  ptrs[0]=input;
  count=1;
  for(tc=0;tc<length;tc++)
  {
    if(input[tc]==separator)
    {
      ptrs[count++]=input+tc+1;
      input[tc]=0;
    }
  }

  return count;
}
