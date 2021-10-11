#!/usr/bin/bash

if [ "$PREFIX" == "" ]; then
  PREFIX="/usr"
fi
VERSION=$(python3 -c 'import sys;print(sys.version.split(" ")[0].rsplit(".",1)[0])')
gcc -Os -I $PREFIX/include/python$VERSION -o shell shell.c -lpython$VERSION -lpthread -lm -lutil -ldl
rm shell.c
