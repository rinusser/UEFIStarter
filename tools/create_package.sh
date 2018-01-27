#!/bin/bash
PACKAGE_DIR=$1
SOURCE_NAME=UEFIStarter

_error() {
  echo "Syntax: $(basename $0) </path/to/package>"
  echo -e "\nERROR: $2"
  exit $1
}

_info() {
  echo $*
}


if [ ! -f "Conf/target.txt" ] || [ ! -e "$SOURCE_NAME" ]; then
  _error -1 "You need to execute this script in the edk2 sources root with UEFIStarter installed."
fi

# check directory parameter
[ -z "$PACKAGE_DIR" ] && _error -2 "You need to specify a package directory to create."
[ -e "$PACKAGE_DIR" ] && _error -3 "The package directory must not exist, it will be created automatically."

# extract project name
PROJECT_NAME=`basename "$PACKAGE_DIR"`

[ -e Makefile ] && [ ! -L Makefile ] && _error -4 "'Makefile' must either be a symlink or not exist."


_info "creating project directory and link if necessary..."
mkdir -p "$PACKAGE_DIR"
[ -e "$PROJECT_NAME" ] || ln -s "$PACKAGE_DIR" "$PROJECT_NAME"


_info "adding package basics to project..."
cp $SOURCE_NAME/*.dec $PROJECT_NAME/$PROJECT_NAME.dec
cp $SOURCE_NAME/*.dsc $PROJECT_NAME/$PROJECT_NAME.dsc
cp $SOURCE_NAME/Makefile.edk $PROJECT_NAME/

mkdir "$PROJECT_NAME/static"
echo -e "FS0:\napp" > "$PROJECT_NAME/static/startup.nsh"


_info "editing package basics..."

# change package name/guid in .dec file
sed -i 's/^\(\s*PACKAGE_NAME\s*=\s*\).*/\1'"$PROJECT_NAME/" $PROJECT_NAME/*.dec
sed -i 's/^\(\s*PACKAGE_GUID\s*=\s*\).*/\1'"$(uuid)/" $PROJECT_NAME/*.dec

# change platform name/guid in .dsc file
sed -i 's/^\(\s*PLATFORM_NAME\s*=\s*\).*/\1'"$PROJECT_NAME/" $PROJECT_NAME/*.dsc
sed -i 's/^\(\s*PLATFORM_GUID\s*=\s*\).*/\1'"$(uuid)/" $PROJECT_NAME/*.dsc
sed -i 's/^\(\s*OUTPUT_DIRECTORY\s*=\s*\).*/\1Build\/'"$PROJECT_NAME/" $PROJECT_NAME/*.dsc

# remove useless libraries
sed -i 's/^.*library\/core\/.*//' $PROJECT_NAME/*.dsc

# replace components and remove extra newlines
cat $PROJECT_NAME/*.dsc | tr '\n' '\001' | sed 's/\(\[Components\]\).*\(!include\)/\1\n  '"$PROJECT_NAME"'\/apps\/app.inf\n\n\2/' | sed 's/\x01\x01\x01*/\x01\x01/g' | tr '\001' '\n' > $PROJECT_NAME/*.dsc


_info "writing demo application..."

mkdir $PROJECT_NAME/apps

echo "#include <Uefi.h>
#include <Library/UefiLib.h>
#include <UEFIStarter/core.h>


INTN EFIAPI ShellAppMain(UINTN argc, CHAR16 **argv)
{
  EFI_STATUS rv;
  if((rv=init(argc,argv,0))!=EFI_SUCCESS)
    return rv;

  Print(L\"Hello, world!\\n\");
  //insert or call your code here

  shutdown();
  return EFI_SUCCESS;
}" >> $PROJECT_NAME/apps/app.c

echo "[Defines]
  INF_VERSION = 1.25
  BASE_NAME = app
  FILE_GUID = "`uuid`"
  MODULE_TYPE = UEFI_APPLICATION
  VERSION_STRING = 1.0
  ENTRY_POINT = ShellCEntryLib

[Sources]
  app.c

[Packages]
  MdePkg/MdePkg.dec
  ShellPkg/ShellPkg.dec
  UEFIStarter/UEFIStarter.dec

[LibraryClasses]
  UefiLib
  ShellCEntryLib
  UEFIStarterCore" >> $PROJECT_NAME/apps/app.inf


_info "updating edk2 build target..."
sed -i 's/^\(\s*ACTIVE_PLATFORM\s*=\s*\).*/\1'"$PROJECT_NAME\/$PROJECT_NAME.dsc/" Conf/target.txt


_info "editing package's Makefile..."
sed -i "s/$SOURCE_NAME/$PROJECT_NAME/g" $PROJECT_NAME/Makefile.edk
sed -i 's/\(.*\/\(tools\|tests\)\/.*\)/#\1/' $PROJECT_NAME/Makefile.edk


_info "updating Makefile symlink..."
[ -e Makefile ] && rm Makefile
ln -s "$PROJECT_NAME/Makefile.edk" Makefile


_info "all done."
echo ""
echo "You can build and run your new package with:"
echo "  make && make run"
