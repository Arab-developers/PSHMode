import marshal
from types import CodeType
from typing import Union


def get_bytecode(source: str) -> CodeType:
    return compile(source, "<strings>", "exec")


def get_bytecode_from_file(filename: str) -> CodeType:
    try:
        with open(filename, "r") as f:
            data = f.read()
        return get_bytecode(data)
    except UnicodeDecodeError:
        with open(filename, "rb") as f:
            data = f.read()
        return marshal.loads(data[16:])


def clean_source(source: Union[str, bytes]) -> Union[str, bytes, CodeType]:
    if type(source) == str:
        try:
            get_bytecode(source)
            return source
        except SyntaxError:
            print("# This is not a python file or maybe there is a syntax error!")
            exit(1)
        except ValueError:
            return source.encode()
    try:
        return source.decode("utf-8")
    except UnicodeDecodeError:
        return get_bytecode(source)


def get_marshal_bytes_from_bytecode(bytecode: CodeType) -> bytes:
    for const in bytecode.co_consts:
        if type(const) == bytes:
            try:
                marshal.loads(const)
                return const
            except Exception:
                pass
    return b''
