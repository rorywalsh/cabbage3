import re
import json
import os
import argparse

def parse_values(values_str):
    # Check if values are enclosed in quotes, indicating they are strings
    if '"' in values_str:
        # Split by comma, keeping quotes intact, to get individual values
        values = re.findall(r'\".*?\"|[^,]+', values_str)
        parsed_values = [value.strip().strip('"') for value in values]
    else:
        # Split by comma and convert to float if possible
        values = values_str.split(',')
        parsed_values = [float(value.strip()) if value.strip().replace('.', '', 1).isdigit() else value.strip() for value in values]

    # Return single value if there's only one, else return list of values
    return parsed_values if len(parsed_values) > 1 else parsed_values[0]

def parse_cabbage_code(cabbage_code):
    widget_regex = re.compile(r'(\w+)\s+(\S.+)')
    param_regex = re.compile(r'(\w+)\((.*?)\)')
    
    widgets = []
    for line in cabbage_code.splitlines():
        match = widget_regex.match(line.strip())
        if match:
            widget_type = match.group(1)
            params = match.group(2)
            
            widget_dict = {'type': widget_type}
            for param in param_regex.findall(params):
                key, value = param
                widget_dict[key] = parse_values(value)
                
            widgets.append(widget_dict)
    
    return widgets

def convert_to_json_objects(cabbage_code_section, single_line=False):
    cabbage_code = cabbage_code_section.group(1)
    widgets = parse_cabbage_code(cabbage_code)
    if single_line:
        return '[\n' + ',\n'.join(json.dumps(widget) for widget in widgets) + '\n]'
    else:
        return json.dumps(widgets, indent=4)

def process_csd_file(file_path, single_line=False):
    with open(file_path, 'r') as file:
        content = file.read()
        
    cabbage_section_regex = re.compile(r'<Cabbage>(.*?)</Cabbage>', re.DOTALL)
    modified_content = cabbage_section_regex.sub(lambda m: '<Cabbage>\n' + convert_to_json_objects(m, single_line) + '\n</Cabbage>', content)
    
    script_dir = os.path.dirname(os.path.abspath(__file__))
    output_file_path = os.path.join(script_dir, 'modified_' + os.path.basename(file_path))
    with open(output_file_path, 'w') as file:
        file.write(modified_content)
    
    print(f"Modified file saved as {output_file_path}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Convert Cabbage code to JSON objects.')
    parser.add_argument('file_path', type=str, help='Path to the .csd file')
    parser.add_argument('--single-line', action='store_true', help='Output each JSON object on a single line')
    args = parser.parse_args()
    
    process_csd_file(args.file_path, args.single_line)
