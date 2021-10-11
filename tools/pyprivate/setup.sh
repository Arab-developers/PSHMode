#!/usr/bin/bash

if [ "$PREFIX" == "" ]; then
  PREFIX="/usr"
fi

VERSION=$(python3 -c 'import sys;print(sys.version.split(" ")[0].rsplit(".",1)[0])')
FILE_NAME="pyprivate"

mkdir "bin" &>/dev/null
echo "compile pyprivate:"
gcc -Os -I $PREFIX/include/python$VERSION -o bin/$FILE_NAME $FILE_NAME.c -lpython$VERSION -lpthread -lm -lutil -ldl
echo "Done..."
