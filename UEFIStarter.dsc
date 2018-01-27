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

  UEFIStarterCore|UEFIStarter/library/core.inf
  logger|UEFIStarter/library/core/logger.inf
  cmdline|UEFIStarter/library/core/cmdline.inf
  string|UEFIStarter/library/core/string.inf
  console|UEFIStarter/library/core/console.inf
  memory|UEFIStarter/library/core/memory.inf
  timestamp|UEFIStarter/library/core/timestamp.inf
  files|UEFIStarter/library/core/files.inf

  UEFIStarterPCI|UEFIStarter/library/pci.inf
  UEFIStarterGraphics|UEFIStarter/library/graphics.inf
  UEFIStarterAC97|UEFIStarter/library/ac97.inf

  UEFIStarterTests|UEFIStarter/library/tests/tests.inf


[Components]
  UEFIStarter/apps/quit.inf
  UEFIStarter/apps/cpuid.inf
  UEFIStarter/apps/helloworld.inf
  UEFIStarter/apps/input.inf
  UEFIStarter/apps/snow.inf
  UEFIStarter/apps/timer.inf
  UEFIStarter/apps/lspci.inf
  UEFIStarter/apps/gop.inf
  UEFIStarter/apps/rotation.inf
  UEFIStarter/apps/ac97.inf

  UEFIStarter/library/core/logger.inf
  UEFIStarter/library/core/cmdline.inf
  UEFIStarter/library/core/string.inf
  UEFIStarter/library/core/memory.inf
  UEFIStarter/library/core/console.inf
  UEFIStarter/library/core/timestamp.inf
  UEFIStarter/library/core/files.inf

  UEFIStarter/library/pci.inf
  UEFIStarter/library/graphics.inf
  UEFIStarter/library/ac97.inf

  UEFIStarter/library/tests/tests.inf

  UEFIStarter/tests/suites/selftest/testself.inf
  UEFIStarter/tests/suites/lib/testlib.inf

!include StdLib/StdLib.inc
