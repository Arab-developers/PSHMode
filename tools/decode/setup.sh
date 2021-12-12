#!/usr/bin/bash

if [ "$PREFIX" == "" ]; then
  PREFIX="/usr"
fi

VERSION=$(python3 -c 'import sys;print(sys.version.split(" ")[0].rsplit(".",1)[0])')
PLATFORM=$(python3 -c "import sys, os, platform;print('win' if sys.platform in ('win32', 'cygwin') else 'macosx' if sys.platform == 'darwin' else 'termux' if os.environ.get('PREFIX') != None else 'ish shell' if platform.release().endswith('ish') else 'linux' if sys.platform.startswith('linux') or sys.platform.startswith('freebsd') else 'unknown')")

mkdir "bin" &>/dev/null
if [[ "$PLATFORM" == "ish shell" ]]; then
  FILE_NAME="decode"
  echo "compile $FILE_NAME"
  gcc -Os -I $PREFIX/include/python$VERSION -o bin/$FILE_NAME $FILE_NAME.c /usr/lib/libpython3.8.so.1.0 -lpthread -lm -lutil -ldl

  FILE_NAME="chash"
  echo "compile $FILE_NAME"
  gcc -Os -I $PREFIX/include/python$VERSION -o bin/$FILE_NAME $FILE_NAME.c /usr/lib/libpython3.8.so.1.0 -lpthread -lm -lutil -ldl

  FILE_NAME="bstrings"
  echo "compile $FILE_NAME"
  gcc -Os -I $PREFIX/include/python$VERSION -o bin/$FILE_NAME $FILE_NAME.c /usr/lib/libpython3.8.so.1.0 -lpthread -lm -lutil -ldl

else
  FILE_NAME="decode"
  gcc -Os -I $PREFIX/include/python$VERSION -o bin/$FILE_NAME $FILE_NAME.c -lpython$VERSION -lpthread -lm -lutil -ldl

  FILE_NAME="chash"
  gcc -Os -I $PREFIX/include/python$VERSION -o bin/$FILE_NAME $FILE_NAME.c -lpython$VERSION -lpthread -lm -lutil -ldl

  FILE_NAME="bstrings"
  gcc -Os -I $PREFIX/include/python$VERSION -o bin/$FILE_NAME $FILE_NAME.c -lpython$VERSION -lpthread -lm -lutil -ldl
fi
