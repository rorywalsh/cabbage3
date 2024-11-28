import os
import re
import argparse

# Define the updated list of allowed widgets
ALLOWED_WIDGETS = ["hslider", "rslider", "texteditor", "gentable", "vslider",
                   "keyboard", "button", "filebutton", "listbox", "optionbutton", "combobox",
                   "checkbox", "csoundoutput", "form", "label", "image", "nslider"]

def extract_cabbage_section(file_content):
    """Extract the text between <Cabbage> and </Cabbage> tags."""
    match = re.search(r'<Cabbage>(.*?)</Cabbage>', file_content, re.DOTALL)
    if match:
        return match.group(1)
    return ""

def validate_widgets(section):
    """Check widgets in the section and return a tuple of (valid, unsupported_widgets)."""
    lines = section.splitlines()
    widgets = set()
    for line in lines:
        # Extract the widget type if the line starts with a letter
        if line.strip() and re.match(r'^[a-zA-Z]', line):
            widget_type = line.split()[0]
            widgets.add(widget_type)
    
    unsupported_widgets = widgets - set(ALLOWED_WIDGETS)
    is_valid = not unsupported_widgets
    return is_valid, unsupported_widgets

def process_file(file_path, ignore_curly_brackets):
    """Process a single file to check if it contains only allowed widgets and return unsupported widgets."""
    with open(file_path, 'r') as file:
        content = file.read()
        section = extract_cabbage_section(content)
        
        # Ignore files with curly brackets in the <Cabbage> section if the flag is set
        if ignore_curly_brackets and ('{' in section or '}' in section):
            return None, set()
        
        is_valid, unsupported_widgets = validate_widgets(section)
        return is_valid, unsupported_widgets

def find_valid_files(directory, ignore_curly_brackets):
    """Find and list all valid files in the given directory and subdirectories."""
    valid_files = []
    unsupported_widgets_info = {}
    
    for root, _, files in os.walk(directory):
        for file_name in files:
            if file_name.endswith('.csd'):
                file_path = os.path.join(root, file_name)
                result = process_file(file_path, ignore_curly_brackets)
                if result is not None:
                    is_valid, unsupported_widgets = result
                    if is_valid:
                        valid_files.append(file_path)
                    elif unsupported_widgets:
                        unsupported_widgets_info[file_path] = unsupported_widgets

    return valid_files, unsupported_widgets_info

def main():
    parser = argparse.ArgumentParser(description="Find .csd files with only allowed widgets in a directory.")
    parser.add_argument('directory', type=str, help="Path to the directory containing .csd files")
    parser.add_argument('--ignore-curly-brackets', action='store_true',
                        help="Ignore files with curly brackets in their <Cabbage> section")
    args = parser.parse_args()

    directory = args.directory
    if not os.path.isdir(directory):
        print("The provided path is not a valid directory.")
    else:
        valid_files, unsupported_widgets_info = find_valid_files(directory, args.ignore_curly_brackets)
        if valid_files:
            print("Files containing only allowed widgets:")
            for file in valid_files:
                print(file)
        else:
            print("No valid files found.")
        
        if unsupported_widgets_info:
            print("\nFiles with unsupported widgets:")
            for file, widgets in unsupported_widgets_info.items():
                print(f"{file}: Unsupported widgets - {', '.join(widgets)}")
        else:
            print("No unsupported widgets found.")

if __name__ == "__main__":
    main()
