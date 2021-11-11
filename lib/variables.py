# coding: utf-8
import os
import sys

HACKERMODE_FOLDER_NAME = "psh-mode"


class Variables:
    @property
    def BASHRIC_FILE_PATH(self) -> str:
        if (shell := os.environ.get('SHELL')):
            if shell.endswith("bash"):
                path = os.path.join(shell.split("/bin/")[0], "etc/bash.bashrc")
                if not os.path.exists(path):
                    path = "/etc/bash.bashrc"
            elif shell.endswith("zsh"):
                path = os.path.join(shell.split("/bin/")[0], "etc/zsh/zshrc")
                if not os.path.exists(path):
                    path = "/etc/zsh/zshrc"
                    if not os.path.exists(path):
                        path = os.path.join(shell.split("/bin/")[0], "etc/zshrc")

        return path

    @property
    def HACKERMODE_SHORTCUT(self) -> str:
        """psh-mode shortcut"""
        return """
function psh-mode() {
  if [ $1 ]; then
    if [ $1 == "check" ]; then
      $HOME/.psh-mode/psh-mode/bin/psh-mode check
    elif [ $1 == "update" ]; then
      old_path_update=$(pwd)
      cd
      $HOME/.psh-mode/psh-mode/bin/psh-mode update
      cd $old_path_update
      unset old_path_update
    elif [ $1 == "delete" ]; then
      $HOME/.psh-mode/psh-mode/bin/psh-mode delete
    else
      $HOME/.psh-mode/psh-mode/bin/psh-mode --help
    fi
  else
    if [ $VIRTUAL_ENV ]; then
      echo "psh-mode is running..."
    else
      source $HOME/.psh-mode/psh-mode/bin/activate
    fi
  fi
}
"""

    @property
    def HACKERMODE_ACTIVATE_FILE_PATH(self) -> str:
        """To get psh-mode activate file"""
        return os.path.join(self.HACKERMODE_INSTALL_PATH, "psh-mode/bin/activate")

    @property
    def HACKERMODE_PATH(self) -> str:
        """To get real psh-mode path"""
        return '/'.join(os.path.abspath(__file__).split('/')[:-2])

    @property
    def HACKERMODE_BIN_PATH(self) -> str:
        """To get psh-mode [psh-mode/bin/] directory"""
        return os.path.join(self.HACKERMODE_PATH, "bin")

    @property
    def HACKERMODE_TOOLS_PATH(self) -> str:
        """To get the psh-mode [psh-mode/tools/] path"""
        return os.path.join(self.HACKERMODE_PATH, "tools")

    @property
    def HACKERMODE_INSTALL_PATH(self) -> str:
        """To get the install path [~/.psh-mode/]"""
        ToolPath = os.path.join(os.environ['HOME'], '.psh-mode')
        if not os.path.isdir(ToolPath):
            os.mkdir(ToolPath)
        return ToolPath

    @property
    def HACKERMODE_CONFIG_PATH(self) -> str:
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

        elif os.environ.get('PREFIX') != None:
            return 'termux'

        elif sys.platform.startswith('linux') or sys.platform.startswith('freebsd'):
            return 'linux'
        return 'unknown'


Variables = Variables()

if __name__ == "__main__":
    print(eval(f"Variables.{sys.argv[1]}"))
