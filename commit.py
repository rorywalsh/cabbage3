from xml.dom import minidom
import xml.etree.ElementTree as ET
import sys
import os

def increment_ver(version):
    version = version.split('.')
    version[2] = str(int(version[2]) + 1)
    return '.'.join(version)

newVersionNum = ""

if len(sys.argv) < 2:
    print("Please provide a commit message enclosed in quotation marks")
    exit()

if len(sys.argv) == 3:
    newVersionNum = sys.argv[2]

#bump jucer version numbers
newFileText = ""
with open("icabbage/cabbage/CMakeLists.txt", "rt") as inputFile:
    for line in inputFile:
        if "set" in line and "BUILD_VERSION" in line:
            if len(newVersionNum) > 0:
                line = "\tset(BUILD_VERSION "+newVersionNum+ ")\n"
            else:
                number = line.replace("set", "").replace("(", "").replace(")", ""). replace("BUILD_VERSION", "")
                newVersionNum = increment_ver(number.strip());
                
                line = "\tset(BUILD_VERSION "+ newVersionNum+")\n"
                

        newFileText = newFileText+line

with open("icabbage/cabbage/CMakeLists.txt", "w") as f:
    f.write(newFileText)

os.system('git add icabbage/cabbage')
os.system('git add icabbage/iplug.patch')
os.system('git add vscabbage')
os.system('git add commit.py')
os.system('git add readme.md')
os.system('git commit -m "'+sys.argv[1]+' - Version number:'+newVersionNum+'"')
