[Defines]
  PLATFORM_NAME                  = UEFIStarter
  PLATFORM_GUID                  = 871898a8-41d5-4fa5-a813-f6bea9f00001
  PLATFORM_VERSION               = 0.01
  DSC_SPECIFICATION              = 0x00010006
  OUTPUT_DIRECTORY               = Build/UEFIStarter
  SUPPORTED_ARCHITECTURES        = X64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT

[PcdsFeatureFlag]

[PcdsFixedAtBuild]

[PcdsFixedAtBuild.IPF]

[LibraryClasses]
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  ShellCEntryLib|ShellPkg/Library/UefiShellCEntryLib/UefiShellCEntryLib.inf

  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  PciLib|MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
  PciCf8Lib|MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf

  DebugLib|MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf

  #StdLib requirements
  UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  UefiHiiServicesLib|MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf
  StdLib|StdLib/LibC/StdLib/StdLib.inf

  logger|UEFIStarter/library/logger.inf
  cmdline|UEFIStarter/library/cmdline.inf
  string|UEFIStarter/library/string.inf
  console|UEFIStarter/library/console.inf
  memory|UEFIStarter/library/memory.inf
  timestamp|UEFIStarter/library/timestamp.inf
  files|UEFIStarter/library/files.inf
  pci|UEFIStarter/library/pci.inf


[Components]
  UEFIStarter/apps/quit.inf
  UEFIStarter/apps/cpuid.inf
  UEFIStarter/apps/helloworld.inf
  UEFIStarter/apps/input.inf
  UEFIStarter/apps/snow.inf
  UEFIStarter/apps/timer.inf
  UEFIStarter/apps/lspci.inf

  UEFIStarter/library/logger.inf
  UEFIStarter/library/cmdline.inf
  UEFIStarter/library/string.inf
  UEFIStarter/library/memory.inf
  UEFIStarter/library/console.inf
  UEFIStarter/library/timestamp.inf
  UEFIStarter/library/files.inf
  UEFIStarter/library/pci.inf

  UEFIStarter/tests/suites/selftest/testself.inf
  UEFIStarter/tests/suites/lib/testlib.inf

!include StdLib/StdLib.inc
