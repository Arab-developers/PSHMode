import os
import json
import shutil

from lib.config import Config

from lib.variables import Variables, HACKERMODE_FOLDER_NAME

RED = '\033[1;31m'
GREEN = '\033[1;32m'
YELLOW = '\033[1;33m'
NORMAL = '\033[0m'
UNDERLINE = '\033[4m'
BOLD = '\033[1m'

with open(os.path.join(Variables.HACKERMODE_PATH, 'packages.json')) as fp:
    INSTALL_DATA = json.load(fp)


class HackerModeInstaller:
    def python_system_modules(self) -> list:
        """this
        function return all modules that installed in system."""
        modules = map(
            lambda lib: lib.split("==")[0],
            os.popen("pip3 freeze").read().split("\n")
        )
        return list(modules)

    def installed_message(self, package, show=True):
        if show:
            default_message = f'{package.split("=")[0]} installed successfully.'
            print(f'{NORMAL}[{GREEN}✔{NORMAL}] {GREEN}{default_message}{NORMAL}')

    def failed_message(self, package, show=True, is_base=False):
        if show:
            default_message = f'not able to install "{package}".'
            color = RED if is_base else YELLOW
            print(f'{NORMAL}[{color}{"✗" if is_base else "!"}{NORMAL}] {color}{default_message}{NORMAL}')

    def check(self, show_output=True) -> dict:
        """this
        function check packages and modules
        and return all packages that not installed.
        """
        modules: list = []
        packages: list = []

        python_modules = self.python_system_modules()
        if show_output:
            print("CHECKING:")
            print("python modules:")
        for module in INSTALL_DATA["PYTHON3_MODULES"]:
            if (module in python_modules) or os.path.exists(
                    os.popen(f"realpath $(command -v {module})").read().strip()):
                self.installed_message(module, show=show_output)
            else:
                modules.append(module)
                self.failed_message(module, show=show_output)

        if show_output:
            print("packages:")
        for package in INSTALL_DATA["PACKAGES"].keys():
            if not INSTALL_DATA["PACKAGES"][package][Variables.PLATFORME]:
                continue
            if os.path.exists(os.popen(f"realpath $(command -v {package.strip()})").read().strip()):
                self.installed_message(package, show=show_output)
            else:
                packages.append(package)
                self.failed_message(package, show=show_output)

        return {"packages": packages, "modules": modules}

    def install(self):
        # check platforme
        if not Variables.PLATFORME in ('termux', 'linux'):
            if Variables.PLATFORME == 'unknown':
                print("# The tool could not recognize the system!")
                print("# Do You want to continue anyway?")
                while True:
                    if input('# [Y/N]: ').lower() == 'y':
                        break
                    else:
                        print('# good bye :D')
                        return
            else:
                print(f"# The tool does not support {Variables.PLATFORME}")
                print('# good bye :D')
                return

        # install packages
        need_to_install = self.check(show_output=False)
        for package in need_to_install["packages"]:
            for command in INSTALL_DATA["PACKAGES"][package][Variables.PLATFORME]:
                os.system(command)

        # install modules
        for module in need_to_install["modules"]:
            os.system(f"pip3 install {module}")

        # move HackerMode to install path
        if Config.get('actions', 'DEBUG', False):
            print("# can't move the HackerMode folder ")
            print("# to install path in debug mode!")
            return None
        if os.path.isdir(HACKERMODE_FOLDER_NAME):
            try:
                shutil.move(HACKERMODE_FOLDER_NAME, Variables.HACKERMODE_INSTALL_PATH)
                self.install_tools_packages()
                Config.set('actions', 'IS_INSTALLED', True)
                self.check()
                print(f'# {GREEN}HackerMode installed successfully...{NORMAL}')
            except shutil.Error as e:
                self.delete(show_message=False)
                print(e)
                print('# installed failed!')
        else:
            self.delete(show_message=False)
            print(f'{RED}# Error: the tool path not found!')
            print(f'# try to run tool using\n# {GREEN}"python3 HackerMode install"{NORMAL}')
            print('# installed failed!')

    def update(self):
        if not Config.get('actions', 'DEBUG', cast=bool, default=False):
            hackermode_command_line_path = os.environ.get("_").split("bin/")[0] + "bin/HackerMode"
            if os.path.exists(hackermode_command_line_path):
                os.remove(hackermode_command_line_path)
            os.system(
                f'curl https://raw.githubusercontent.com/Arab-developers/HackerMode/future/install.sh > HackerModeInstall && bash HackerModeInstall')
            print(f'# {GREEN}HackerMode updated successfully...{NORMAL}')
        else:
            print("# can't update in the DEUBG mode!")

    def add_shortcut(self):
        # add HackerMode shortcut...
        try:
            with open(Variables.BASHRIC_FILE_PATH, "r") as f:
                data = f.read()
            if data.find(Variables.HACKERMODE_SHORTCUT.strip()) == -1:
                with open(Variables.BASHRIC_FILE_PATH, "w") as f:
                    f.write(data + Variables.HACKERMODE_SHORTCUT)
        except PermissionError:
            print(NORMAL + "# add HackerMode shortcut:")
            print(f"# '{YELLOW}{Variables.HACKERMODE_SHORTCUT}{NORMAL}'")
            print("# to this path:")
            print("# " + Variables.HACKERMODE_BIN_PATH)

    def delete(self, show_message=True):
        if show_message:
            status = input("# Do you really want to delete the tool?\n [n/y]: ").lower()
        else:
            status = "y"
        if status in ("y", "yes", "ok", "yep"):
            bin_path = os.path.join(os.environ["SHELL"].split("/bin/")[0], "/bin/HackerMode")
            tool_path = os.path.join(os.environ["HOME"], ".HackerMode")
            if os.path.exists(bin_path):
                os.remove(bin_path)
            if os.path.exists(tool_path):
                shutil.rmtree(tool_path)
                try:
                    with open(Variables.BASHRIC_FILE_PATH, "r") as f:
                        data = f.read()
                    if data.find(Variables.HACKERMODE_SHORTCUT.strip()) != -1:
                        with open(Variables.BASHRIC_FILE_PATH, "w") as f:
                            f.write(data.replace(Variables.HACKERMODE_SHORTCUT, ""))
                except PermissionError:
                    if show_message:
                        print("# cannot remove HackerMode shortcut!")
            if show_message:
                print("# The deletion was successful...")

    def install_tools_packages(self):
        # compile shell file
        old_path = os.getcwd()
        os.chdir(os.path.join(os.environ.get("HOME"), ".HackerMode/HackerMode/lib"))
        os.system("bash setup.sh")
        os.chdir(old_path)

        # install tools packages
        tools_path = os.path.join(os.environ.get("HOME"), ".HackerMode/HackerMode/tools")
        for root, dirs, files in os.walk(tools_path):
            for dir in dirs:
                if os.path.exists(os.path.join(root, dir, "setup.sh")):
                    print(f"installing {dir} packages:")
                    old_path = os.getcwd()
                    os.chdir(os.path.join(root, dir))
                    os.system("bash setup.sh")
                    os.chdir(old_path)


if __name__ == "__main__":
    x = HackerModeInstaller()
    x.check()
    x.install()
