import os
import sys
import json
import shutil

sys.path.append(__file__.rsplit("/", 1)[0])
from lib.config import Config
from lib.variables import Variables, TOOL_NAME

RED = '\033[1;31m'
GREEN = '\033[1;32m'
YELLOW = '\033[1;33m'
NORMAL = '\033[0m'
UNDERLINE = '\033[4m'
BOLD = '\033[1m'

with open(os.path.join(Variables.REAL_TOOL_PATH, 'packages.json')) as fp:
    INSTALL_DATA = json.load(fp)


class HackerModeInstaller:
    def python_system_modules(self) -> list:
        """this
        function return all modules that installed in system."""
        return os.popen("pip3 freeze").read().split("\n")

    def is_installed(self, module, python_modules):
        for python_module in python_modules:
            if module in python_module:
                return [module, python_module]
        return False

    def installed_message(self, package, show=True):
        if show:
            default_message = f'{package.split("=")[0]} installed successfully.'
            print(f'{NORMAL}[{GREEN}✔{NORMAL}] {GREEN}{default_message}{NORMAL}')

    def failed_message(self, package, show=True, is_base=False):
        if show:
            default_message = f'not able to install "{package}".'
            color = RED if is_base else YELLOW
            print(f'{NORMAL}[{color}{"✗" if is_base else "!"}{NORMAL}] {color}{default_message}{NORMAL}')

    def check(self, show_output=True, install_message=False) -> dict:
        """this
        function check packages and modules
        and return all packages that not installed.
        """
        modules: list = []
        packages: list = []

        python_modules = self.python_system_modules()
        if show_output:
            print("\nCHECKING:")
            print("python modules:")
        for module in INSTALL_DATA["PYTHON3_MODULES"]:
            if self.is_installed(module, python_modules) or os.path.exists(
                    os.popen(f"realpath $(command -v {module}) 2> /dev/null").read().strip()):
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

        if show_output and install_message:
            if os.path.isdir(Variables.TOOL_INSTALL_PATH):
                if all([modules, packages]):
                    print(f"# {YELLOW}PSHMode installed successfully but there is some issues.{NORMAL}")
                else:
                    print(f"# {GREEN}PSHMode installed successfully.{NORMAL}")
            else:
                self.delete(show_message=False)
                print(f"# {RED}Installed failed!.{NORMAL}")

        return {"packages": packages, "modules": modules}

    def install(self):
        # install packages
        logout = "~/.PSHMode-install.log"
        need_to_install = self.check(show_output=False)
        for package in need_to_install["packages"]:
            for command in INSTALL_DATA["PACKAGES"][package][Variables.PLATFORME]:
                print(f"Installing {package} ...")
                os.system(f"{command} 2>> {logout}")

        # install modules
        for module in need_to_install["modules"]:
            print(f"Installing {module} ...")
            os.system(f"pip3 install {module} 2>> {logout}")

        # setup PSHMode tools
        self.install_tools_packages()

        # to check
        self.check(install_message=True)

    def update(self):
        if not Config.get('actions', 'DEBUG', cast=bool, default=False):
            hackermode_command_line_path = os.environ.get("_").split("bin/")[0] + "bin/PSHMode"
            if os.path.exists(hackermode_command_line_path):
                os.remove(hackermode_command_line_path)
            os.system(
                f'curl https://raw.githubusercontent.com/Arab-developers/PSHMode/main/install.sh > PSHMode.install 2> .PSHMode-install.log && {Variables.SHELL_COMMAND} PSHMode.install')
        else:
            print("# can't update in the DEUBG mode!")

    def add_shortcut(self):
        # add PSHMode shortcut...
        try:
            with open(Variables.BASHRIC_FILE_PATH, "r") as f:
                data = f.read()
            if data.find(Variables.TOOL_SHORTCUT.strip()) == -1:
                with open(Variables.BASHRIC_FILE_PATH, "w") as f:
                    f.write(data + Variables.TOOL_SHORTCUT)
        except PermissionError:
            print(f"# {RED}can't add the tool shortcut!{NORMAL}")

    def delete(self, show_message=True):
        if show_message:
            status = input("# Do you really want to delete the tool?\n [n/y]: ").lower()
        else:
            status = "y"
        if status in ("y", "yes", "ok", "yep"):
            tool_path = os.path.join(os.environ["HOME"], ".PSHMode")
            errors = 0
            if os.path.exists(tool_path):
                try:
                    with open(Variables.BASHRIC_FILE_PATH, "r") as f:
                        data = f.read()
                    if data.find(Variables.TOOL_SHORTCUT.strip()) != -1:
                        with open(Variables.BASHRIC_FILE_PATH, "w") as f:
                            f.write(data.replace(Variables.TOOL_SHORTCUT, ""))
                except PermissionError:
                    if show_message:
                        errors += 1
                        print("# cannot remove PSHMode shortcut!")
                try:
                    shutil.rmtree(tool_path)
                except Exception as e:
                    errors += 1
                    print(e)

            if errors:
                exit(1)
            else:
                if show_message:
                    print("# The deletion was successful...")

    def install_tools_packages(self):
        # compile shell file
        old_path = os.getcwd()
        os.chdir(os.path.join(Variables.TOOL_INSTALL_PATH, "lib"))
        os.system(f"{Variables.SHELL_COMMAND} setup.sh")
        os.chdir(old_path)

        # install tools packages
        def run_setup(root, dir):
            old_path = os.getcwd()
            os.chdir(os.path.join(root, dir))
            os.system(f"{Variables.SHELL_COMMAND} setup.sh")
            os.chdir(old_path)

        for root, dirs, files in os.walk(Variables.TOOLS_PATH):
            for dir in dirs:
                if os.path.exists(os.path.join(root, dir, "setup.sh")):
                    print(f"Installing {dir} packages...")
                    run_setup(root, dir)


if __name__ == "__main__":
    x = HackerModeInstaller()
    x.check()
    x.install()
