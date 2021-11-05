#!/usr/bin/bash

if [ "$PREFIX" == "" ]; then
  PREFIX="/usr"
fi

VERSION=$(python3 -c 'import sys;print(sys.version.split(" ")[0].rsplit(".",1)[0])')

mkdir "bin" &>/dev/null
echo "setup decode..."
FILE_NAME="decode"
gcc -Os -I $PREFIX/include/python$VERSION -o bin/$FILE_NAME $FILE_NAME.c -lpython$VERSION -lpthread -lm -lutil -ldl
echo "setup chash..."
FILE_NAME="chash"
gcc -Os -I $PREFIX/include/python$VERSION -o bin/$FILE_NAME $FILE_NAME.c -lpython$VERSION -lpthread -lm -lutil -ldl
echo "setup bstrings..."
FILE_NAME="bstrings"
gcc -Os -I $PREFIX/include/python$VERSION -o bin/$FILE_NAME $FILE_NAME.c -lpython$VERSION -lpthread -lm -lutil -ldl
