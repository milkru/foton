#!/usr/bin/python3

import os
import sys
import platform
import subprocess

Python = "python" if platform.system() == "Windows" else "python3"

def executeCommandSilent(command):
	out = open(os.devnull, 'w')
	err = subprocess.STDOUT
	return subprocess.call(command, shell = True, stdout=out, stderr=err);

def runPythonScript(Script, Params):
	if os.system(Python + " " + Script + " " + Params) != 0:
		print("Unable to run " + Script)
		exit(255)

if executeCommandSilent(Python + " --version") != 0:
	print("Make sure you can run Python scripts from the command line.")
	exit(255)

if executeCommandSilent("cmake --version") != 0:
	print("CMake not installed.")
	exit(255)

if executeCommandSilent("git --version") != 0:
	print("Git not installed.")
	exit(255)

runPythonScript(os.path.join("ThirdParty", "bootstrap.py"), "-b ThirdParty")
