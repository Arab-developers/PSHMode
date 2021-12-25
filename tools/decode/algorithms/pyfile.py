from typing import Union


def open_python_file(filename) -> Union[str, bytes]:
    try:
        with open(filename, "r") as r_file:
            return r_file.read()
    except UnicodeDecodeError:
        with open(filename, "rb") as rb_file:
            return rb_file.read()
