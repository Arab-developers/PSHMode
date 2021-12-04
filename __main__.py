import os
import runpy

runpy.run_path(os.path.join(os.path.abspath(__file__).rsplit("/", 1)[0], "bin/PSHMode"))
