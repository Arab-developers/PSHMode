import marshal
import os
import sys

sys.path.insert(0, os.environ.get("VIRTUAL_ENV") + "/tools/decode")

import base64
import builtins
import webbrowser
from types import CodeType
from typing import Union, Optional
from algorithms.filters import eval_filter
from subprocess import Popen, PIPE
from rich.console import Console

console = Console()


class FakeFunction:
    def __init__(self, source: str, filename: str):
        global __file__
        self.pyc_source = None

        # to save the real functions.
        self.old_webbrowser_open = webbrowser.open
        self.old_os_system = os.system
        self.old__file__ = __file__
        self.old_exec = builtins.exec
        self.old_loads = marshal.loads
        self.old_compile = builtins.compile

        # change real functions to fake function.
        __file__ = filename
        exec = self._fake_exec
        marshal.loads = self._fake_loads
        builtins.compile = self._fake_compile

        # ignore spamm function
        webbrowser.open = lambda *args, **kwargs: None
        os.system = lambda *agrs, **kwargs: None

        # execute the source code.
        try:
            if "eval" in source:
                source = eval_filter(source)
            self.old_exec(source)
        except ModuleNotFoundError as err:
            print("#", err)
            print("# install the Module first then try again.")
        except SystemError:
            print("# unknown opcode! try to use another python3 version to decode this file.")
        except NameError as err:
            if self.pyc_source is not None:
                pass
            else:
                print("#", err)
                print("# there is a NameError in the file fix it first and try again.")
        except KeyboardInterrupt:
            pass

        # to replace all fake functions with the
        # real function.
        webbrowser.open = self.old_webbrowser_open
        os.system = self.old_os_system
        __file__ = self.old__file__
        builtins.exec = self.old_exec
        marshal.loads = self.old_loads
        builtins.compile = self.old_compile

    def get_source(self) -> Union[str, None, CodeType]:
        if self.pyc_source:
            if type(self.pyc_source) == bytes:
                try:
                    return self.pyc_source.decode()
                except UnicodeDecodeError:
                    return marshal.loads(self.pyc_source)
            else:
                return str(self.pyc_source)
        return None

    def _fake_exec(self, *args, **kwargs):
        if type(args[0]) in (bytes, str):
            self.pyc_source = args[0]

    def _fake_loads(self, *args, **kwargs):
        if type(args[0]) in (bytes, str):
            self.pyc_source = args[0]
        return self.old_loads(*args, **kwargs)

    def _fake_compile(self, *args, **kwargs):
        if type(args[0]) in (bytes, str):
            self.pyc_source = args[0]
        return self.old_compile(*args, **kwargs)


class DecompilePyc:
    def __init__(self, filename: str):
        self.filename = filename
        self.std = Popen([os.environ.get("VIRTUAL_ENV") + "/tools/decode/pycdc/pycdc", filename], stdout=PIPE,
                         stderr=PIPE)

    def get_source(self) -> Optional[str]:
        out = self.std.stdout.read().decode()
        err = self.std.stderr.read().decode()
        if out and err:
            return out + '\n' + err
        elif out:
            return out
        else:
            print(err)
            return None


class DecompileMarshal:
    def __init__(self, bytecode: CodeType):
        self._data: bytes = marshal.dumps(bytecode)
        self._magic_number: bytes = b'a\r\r\n\x00\x00\x00\x00\xe2\xb6\xcea\r\x00\x00\x00'

    def get_source(self) -> bytes:
        return self._magic_number + self._data


def check(filename) -> bool:
    std = Popen(["check-hash", filename], stdout=PIPE, stderr=PIPE)
    out = std.stdout.read().decode()
    if out:
        console.print(f'# encode type: [red]{out.split(": ")[-1][:-1]}[/red]')
        console.print("# This file cannot be decrypted because it uses our encryption.")
        return True
    return False
