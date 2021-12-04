# coding: utf-8
import os
import sys

TOOL_NAME = "PSHMode"


class Variables:
    @property
    def BASHRIC_FILE_PATH(self) -> str:
        if self.PLATFORME == "termux":
            return os.environ.get("PREFIX") + "/etc/zshrc"

        shell = os.environ.get('SHELL')
        if shell.endswith("bash"):
            path = "/etc/bash.bashrc"
        elif shell.endswith("zsh"):
            path = "/etc/zsh/zshrc"
            if not os.path.exists(path):
                path = "/etc/zshrc"
        return path

    @property
    def TOOL_SHORTCUT(self) -> str:
        """PSHMode shortcut"""
        with open(os.path.join(self.REAL_TOOL_PATH, "PSHMode.shortcut"), "r") as file:
            data = file.read()
        return data

    @property
    def ACTIVATE_FILE_PATH(self) -> str:
        """To get PSHMode activate file"""
        return os.path.join(self.TOOL_INSTALL_PATH, "PSHMode/bin/activate")

    @property
    def REAL_TOOL_PATH(self) -> str:
        """To get real PSHMode path"""
        return '/'.join(os.path.abspath(__file__).split('/')[:-2])

    @property
    def TOOLS_PATH(self) -> str:
        """To get the PSHMode [PSHMode/tools/] path"""
        return os.path.join(self.REAL_TOOL_PATH, "tools")

    @property
    def TOOL_INSTALL_PATH(self) -> str:
        """To get the installation path [~/.PSHMode/]"""
        tool_path = os.path.join(os.environ['HOME'], '.PSHMode')
        if not os.path.isdir(tool_path):
            os.mkdir(tool_path)
        return tool_path

    @property
    def CONFIG_PATH(self) -> str:
        """To get the config path [~/.config/]"""
        path = os.path.join(os.environ['HOME'], '.config')
        if not os.path.isdir(path):
            os.mkdir(path)
        return path

    @property
    def PLATFORME(self) -> str:
        """To get the platform name"""
        if sys.platform in ('win32', 'cygwin'):
            return 'win'

        elif sys.platform == 'darwin':
            return 'macosx'

        elif os.environ.get('PREFIX') is not None:
            return 'termux'

        elif sys.platform.startswith('linux') or sys.platform.startswith('freebsd'):
            return 'linux'
        return 'unknown'


Variables = Variables()

if __name__ == "__main__":
    print(eval(f"Variables.{sys.argv[1]}"))
