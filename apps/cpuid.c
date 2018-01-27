/** \file
 * This application will show some information about the CPU.
 *
 * \author Richard Nusser
 * \copyright 2017-2018 Richard Nusser
 * \license GPLv3 (see http://www.gnu.org/licenses/)
 * \sa https://github.com/rinusser/UEFIStarter
 * \ingroup group_apps
 */

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <UEFIStarter/core.h>


/**
 * helper macro to quickly output a CPUID flag
 *
 * \param NAME     the flag's name
 * \param REGISTER the variable to read from
 * \param BIT      the flag's bit offset
 */
#define CI(NAME,REGISTER,BIT) Print(L ## #NAME ": %d\n",(REGISTER&1<<BIT)>0)

/**
 * Prints the CPU's vendor ID and a few of the capability flags.
 */
void cpuid()
{
  UINT16 buf[13];
  UINT64 rax,rbx,rcx,rdx;
  UINT64 id1,id2,id3;

  asm("xor %%rax,%%rax\n"
      "cpuid\n"
      :"=b" (id1), "=d" (id2), "=c" (id3)
      :
      :"rax");
  LOG.trace(L"id1=%l08X id2=%l08X id3=%l08X\n",id1,id2,id3);

  buf[0]=(id1&0xff);
  buf[1]=(id1&0xff00)>>8;
  buf[2]=(id1&0xff0000)>>16;
  buf[3]=(id1&0xff000000)>>24;
  buf[4]=(id2&0xff);
  buf[5]=(id2&0xff00)>>8;
  buf[6]=(id2&0xff0000)>>16;
  buf[7]=(id2&0xff000000)>>24;
  buf[8]=(id3&0xff);
  buf[9]=(id3&0xff00)>>8;
  buf[10]=(id3&0xff0000)>>16;
  buf[11]=(id3&0xff000000)>>24;
  buf[12]=0;
  Print(L"vendor id: %s\n",buf);

  asm("mov $1,%%rax\n"
      "cpuid\n"
      :"=a" (rax), "=b" (rbx), "=c" (rcx), "=d" (rdx));
  LOG.trace(L"rax=%l08X rbx=%l08X rcx=%l08X rdx=%l08X\n",rax,rbx,rcx,rdx);

  Print(L"stepping: %d\n",rax&0xf);
  Print(L"model: %d\n",(rax&0xf0)>>4);
  Print(L"family: %d\n",(rax&0xf00)>>8);
  Print(L"processor type: %d\n",(rax&0x3000)>>12);
  Print(L"extended model: %d\n",(rax&0xf0000)>>16);
  Print(L"extended family: %d\n",(rax&0xff00000)>>20);

  CI(fpu,rdx,0);
  CI(msr,rdx,5);
  CI(apic,rdx,9);
  CI(mmx,rdx,23);
  CI(sse,rdx,25);
  CI(sse2,rdx,26);
  CI(htt,rdx,28);

  CI(sse3,rcx,0);
  CI(ssse3,rcx,9);
  CI(sse4.1,rcx,19);
  CI(sse4.2,rcx,20);
  CI(aes,rcx,25);
  CI(avx,rcx,28);
  CI(hypervisor,rcx,31);
}

/**
 * Reads a CPU's model-specific register.
 *
 * \param rcx the register selector for the RDMSR instruction
 * \return the register's value
 */
UINT64 rdmsr(UINT64 rcx)
{
  UINT64 rdx, rax;

  asm("rdmsr"
      :"=d" (rdx), "=a" (rax)
      :"c" (rcx));
  return ((rdx&0xffffffff)<<32)|(rax&0xffffffff);
}

/**
 * Prints a selection of model-specific CPU registers.
 * Currently the selection is very narrow: the function just prints the APIC base address.
 */
void read_msrs()
{
  UINT32 func=0x1b;
  Print(L"MSRs:\n");
  Print(L"  %02X: %016lX (%s)\n",func,rdmsr(func),L"APIC base address");
}

/** data type for interrupt descriptor table metadata */
typedef struct
{
  unsigned short limit;  /**< the IDT's size, in bytes-1 */
  void *address;         /**< the IDT's starting address */
} __attribute__((packed)) idt_reg_t;

/**
 * Interrupt handler, this will be registered by test_idt() and should get called when signalling the interrupt there.
 * This will set rax (the return register) to the current instruction pointer.
 */
void handler()
{
  asm volatile ("lea (%rip),%rax\n\
                 iretq");
}

/**
 * Writes an interrupt handler address into the IDT.
 *
 * \param entry_base the start of the IDT entry to write to
 * \param func       the handler's address
 */
void write_idt_entry_address(void *entry_base, void *func)
{
  UINT64 addr=(UINT64)func;
  *((UINT16 *)(entry_base+0))=addr&0xffff;
  *((UINT16 *)(entry_base+6))=(addr>>16)&0xffff;
  *((UINT32 *)(entry_base+8))=(addr>>32)&0xffffffff;
}

/**
 * Tests whether interrupt handlers can be registered.
 *
 * This will register handler() as the handling function for INT 3, then signal INT 3. The handler should get called
 * and should return its instruction pointer (which should be very close to handler()'s address). This function checks
 * the returned instruction pointer.
 */
void test_idt()
{
  UINT64 delta;
  UINT64 handler_address;
  UINT64 rax;
  idt_reg_t idt_reg;
  idt_reg.address=NULL;
  idt_reg.limit=0;
  asm volatile ("sidt %0" :"=m" (idt_reg));
  Print(L"IDT address=%016lX, limit=%d\n",idt_reg.address,idt_reg.limit);

  Print(L"writing INT 3 handler...\n");
  write_idt_entry_address(idt_reg.address+3*16,handler);

  Print(L"calling INT 3...\n");
  rax=0;
  asm volatile ("int $3" :"=a" (rax));

  handler_address=(UINT64)&handler;
  Print(L"int 3 handler address: %016lX\n",handler_address);
  Print(L"int 3 handler returned %016lX ",rax);
  if(rax>handler_address)
    delta=rax-handler_address;
  else
    rax=handler_address-rax;
  if(delta<100)
    Print(L"(OK: close enough)\n");
  else
    Print(L"(ERROR: difference too high)\n");
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

  cpuid();
  Print(L"\n");
  read_msrs();
  test_idt();

  shutdown();
  return EFI_SUCCESS;
}
