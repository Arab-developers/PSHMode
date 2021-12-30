import os
import sys
sys.path.insert(0, os.environ.get("VIRTUAL_ENV") + "/tools/decode")

from types import CodeType
from typing import Union
from zipfile import ZipFile
from utils.bytecode import clean_source

def open_file(filename) -> Union[str, bytes, CodeType]:
    try:
        with open(filename, "r") as r_file:
            return clean_source(r_file.read())
    except UnicodeDecodeError:
        with open(filename, "rb") as rb_file:
            return rb_file.read()


def open_python_file(filename) -> Union[str, bytes, CodeType]:
    source = open_file(filename)
    if get_source_type(source) == "zip":
        archive = ZipFile(filename)
        py_filename = archive.filelist[0].filename
        source = archive.read(py_filename)
        if get_source_type(source) == "py":
            return clean_source(source)
        return source
    return source


def get_source_type(source) -> str:
    try:
        compile(source, "<string>", "exec")
        return "py"
    except Exception:
        if type(source) == str:
            source = source.encode("utf-8")
        if b'PK\x03\x04' in source:
            return "zip"
        else:
            return "pyc"
    except SyntaxError:
        return "unknow"


def get_file_type(filename) -> str:
    source = open_file(filename)
    return get_source_type(source)


