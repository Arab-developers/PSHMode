import marshal
import os
import sys
sys.path.insert(0, os.environ.get("VIRTUAL_ENV") + "/tools/decode")

import base64
import builtins
import webbrowser
from types import CodeType
from typing import Union
from algorithms.filters import eval_filter
from subprocess import Popen, PIPE


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
            exit(1)
        except SystemError:
            print("# unknown opcode! try to use another python3 version to decode this file.")
            exit(1)
        except NameError as err:
            if self.pyc_source != None:
                pass
            else:
                print("#", err)
                print("# there is a NameError in the file fix it first and try again.")
                exit(1)
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
        self.std = Popen([os.environ.get("VIRTUAL_ENV") + "/tools/decode/pycdc/pycdc", filename], stdout=PIPE, stderr=PIPE)

    def get_source(self) -> str:
        err = self.std.stderr.read()
        if err:
            print(err.decode())
            exit(1)
        return self.std.stdout.read().decode()
