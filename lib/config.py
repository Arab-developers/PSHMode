# coding: utf-8
import os
import json
import shutil
import sys

try:
    from lib.variables import Variables
except ImportError:
    from variables import Variables


class config(object):
    default_file = os.path.join(Variables.CONFIG_PATH, 'PSHMode-config.json')

    def __init__(self, file=None):
        if file:
            self.file = file
        else:
            file = self.default_file
            if not os.path.isfile(file):
                # default settings
                shutil.copy(os.path.join(__file__.split("/lib/config.py")[0], 'settings.json'), file)
            self.file = file

    def set_file(self, file_path):
        if not os.path.isfile(file_path):
            with open(file_path, 'w') as f:
                f.write(json.dumps({}, indent=4))
        self.file = file_path

    def set(self, section, option, value):
        section = section.lower()
        option = option.upper()
        with open(self.file, 'r') as f:
            data = json.loads(f.read())

        try:
            data[section][option] = value
        except KeyError:
            data[section] = {}
            data[section][option] = value

        with open(self.file, 'w') as f:
            f.write(json.dumps(data, indent=4))

    def get(self, section, option, cast=None, default=None):
        section: str = section.lower()
        option: str = option.upper()
        objects = (str, bool, dict, list, int, set)
        with open(self.file, 'r') as f:
            data = json.loads(f.read())
        if default != None:
            try:
                return self.get(section, option, cast=cast)
            except KeyError:
                default = cast(default) if cast in objects else default
                self.set(section, option, default)
                return default
        else:
            if cast in objects:
                return cast(data[section][option])
            return data[section][option]


Config = config()

if __name__ == '__main__':
    try:
        print(Config.get(sys.argv[1], sys.argv[2]))
    except KeyError:
        exit(1)
