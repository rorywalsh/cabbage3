import sys
import os
import subprocess

def increment_ver(version):
    version = version.split('.')
    version[2] = str(int(version[2]) + 1)
    return '.'.join(version)

def get_changed_files():
    result = subprocess.run(['git', 'status', '--porcelain=1'], stdout=subprocess.PIPE)
    changed_files = result.stdout.decode('utf-8').strip().split('\n')
    return [line for line in changed_files if line]

def categorize_changes(changed_files):
    vscabbage_changes = []
    icabbage_changes = []
    tests_changes = []
    example_changes = []

    for line in changed_files:
        status = line[:2].strip()
        file_path = line[3:].strip()

        if file_path.startswith('vscabbage'):
            vscabbage_changes.append((status, os.path.basename(file_path)))
        elif file_path.startswith('icabbage/cabbage'):
            icabbage_changes.append((status, os.path.basename(file_path)))
        elif file_path.startswith('tests'):
            tests_changes.append((status, os.path.basename(file_path)))
        elif file_path.startswith('example'):
            example_changes.append((status, os.path.basename(file_path)))
    
    return vscabbage_changes, icabbage_changes, tests_changes, example_changes

def propose_commit_message(vscabbage_changes, icabbage_changes, tests_changes, example_changes):
    commit_message = ""
    
    def format_changes(changes):
        formatted = ""
        for status, file in changes:
            action = ""
            if status == "M":
                action = "Modified"
            elif status == "A":
                action = "Added"
            elif status == "D":
                action = "Deleted"
            elif status == "R":
                action = "Renamed"
            else:
                action = "Updated"
            formatted += f" {file} "
        return formatted
    
    if vscabbage_changes:
        commit_message += " VSCabbage:" + format_changes(vscabbage_changes)
    
    if icabbage_changes:
        commit_message += " Cabbage:" + format_changes(icabbage_changes)
    
    if tests_changes:
        commit_message += " Tests:" + format_changes(tests_changes)
    
    if example_changes:
        commit_message += " Example:" + format_changes(example_changes)
    
    return commit_message.strip()

newVersionNum = ""

if len(sys.argv) < 2:
    print("Please provide a base commit message enclosed in quotation marks to override default message and to include a build number")

if len(sys.argv) == 3:
    newVersionNum = sys.argv[2]

# Bump version numbers
newFileText = ""
with open("icabbage/cabbage/CMakeLists.txt", "rt") as inputFile:
    for line in inputFile:
        if "set" in line and "BUILD_VERSION" in line:
            if len(newVersionNum) > 0:
                line = "\tset(BUILD_VERSION " + newVersionNum + ")\n"
            else:
                number = line.replace("set", "").replace("(", "").replace(")", "").replace("BUILD_VERSION", "")
                newVersionNum = increment_ver(number.strip())
                line = "\tset(BUILD_VERSION " + newVersionNum + ")\n"
                
        newFileText = newFileText + line

with open("icabbage/cabbage/CMakeLists.txt", "w") as f:
    f.write(newFileText)

# Get changed files and categorize them
changed_files = get_changed_files()
vscabbage_changes, icabbage_changes, tests_changes, example_changes = categorize_changes(changed_files)

# Propose commit message
base_commit_message = sys.argv[1] if len(sys.argv) > 1 else None
proposed_commit_message = propose_commit_message(vscabbage_changes, icabbage_changes, tests_changes, example_changes)

if base_commit_message:
    final_commit_message = base_commit_message + " - Build number: " + newVersionNum
else:
    final_commit_message = "Build number: " + newVersionNum + "\n" + proposed_commit_message

# Print proposed commit message for debugging
print(final_commit_message)

# Execute git commands
os.system('git add icabbage/cabbage')
os.system('git add icabbage/iplug2.patch')
os.system('git add tests')
os.system('git add example')
os.system('git add commit.py')
os.system('git add readme.md')
os.system(f'git commit -m "{final_commit_message}"')
