import marshal
from types import CodeType


def get_bytecode(source: str) -> CodeType:
    return compile(source, "strings", "exec")


def get_bytecode_from_file(filename: str) -> CodeType:
    try:
        with open(filename, "r") as f:
            data = f.read()
        return get_bytecode(data)
    except UnicodeDecodeError:
        with open(filename, "rb") as f:
            data = f.read()
        return marshal.loads(data[16:])


def get_marshal_bytes_from_bytecode(bytecode: CodeType) -> bytes:
    for const in bytecode.co_consts:
        if type(const) == bytes:
            try:
                marshal.loads(const)
                return const
            except Exception:
                pass
    return b''
