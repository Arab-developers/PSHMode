#!/usr/bin/bash

if [[ "$PREFIX" == "" ]]; then
  PREFIX="/usr"
fi
PLATFORM=$(python3 -c "import sys, os, platform;print('win' if sys.platform in ('win32', 'cygwin') else 'macosx' if sys.platform == 'darwin' else 'termux' if os.environ.get('PREFIX') != None else 'ish shell' if platform.release().endswith('ish') else 'linux' if sys.platform.startswith('linux') or sys.platform.startswith('freebsd') else 'unknown')")
VERSION=$(python3 -c 'import sys;print(sys.version.split(" ")[0].rsplit(".",1)[0])')
if [[ "$PLATFORM" == "ish shell" ]]; then
  gcc -Os -I $PREFIX/include/python$VERSION -o shell shell.c /usr/lib/libpython3.8.so.1.0 -lpthread -lm -lutil -ldl
else
  gcc -Os -I $PREFIX/include/python$VERSION -o shell shell.c -lpython$VERSION -lpthread -lm -lutil -ldl
fi
rm shell.c