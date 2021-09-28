#!/usr/bin/python3 -B
import os
import sys
import time
from threading import Thread

BIT: int = 1024
SIZES: tuple = (
    "KB",
    "MB",
    "GB",
    "TB",
    "PB",
    "EB",
    "ZB",
    "YB"
)

class Size:

    def __init__(self, path: str, anim: bool=True) -> None:
        self._isdone = False
        if anim: self.anim
        self.size = self.model_size(self.num_size(path))

    @property
    def anim(self) -> None:
        text =  "Calculating the size..."
        def _anim() -> None:
            while not self._isdone:
                for x in range(len(text)):
                    if self._isdone:
                        break
                    print(f'\r{text[:x]}{text[x].upper()}{text[x+1:]}', end="")
                    time.sleep(0.2)
        Thread(target=_anim).start()


    def num_size(self, path: str) -> int:
        return os.path.getsize(path) if os.path.isfile(path) else sum(
            os.path.getsize(os.path.join(p, file))
            for p, d, f in os.walk(path)
            for file in f
        )

    def model_size(self, size: int, color: bool=True) -> str:
        for x in range(len(SIZES)):
            _x = x+1
            if (_size := eval("("*_x + str(size) + "".join([f"/{BIT})"]*_x))) < BIT:
                _size = str(_size).split(".")
                _size = f"\x1b[0;36m{_size[0]}.{_size[1][0:2]} \x1b[0;32m{SIZES[x]}\x1b[0m" if color else f"{_size[0]}.{_size[1][0:2]} {SIZES[x]}"
                break
        self._isdone = True # close animation
        print("\r                       ", end='\r')
        return _size