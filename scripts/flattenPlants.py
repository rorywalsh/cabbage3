import os
import re
import argparse

def extract_cabbage_section(file_content):
    """Extract the text between <Cabbage> and </Cabbage> tags."""
    match = re.search(r'<Cabbage>(.*?)</Cabbage>', file_content, re.DOTALL)
    return match.group(1) if match else ""

def update_bounds(bounds_str, x_offset, y_offset):
    """Update the bounds of a widget by adding offsets."""
    bounds = re.findall(r'\d+', bounds_str)
    print(f"Original bounds: {bounds}")
    if len(bounds) == 4:
        new_x = int(bounds[0]) + x_offset
        new_y = int(bounds[1]) + y_offset
        new_bounds = f"bounds({new_x}, {new_y}, {bounds[2]}, {bounds[3]})"
        print(f"Updated bounds: {new_bounds}")
        return new_bounds
    return bounds_str

def process_plant_section(section):
    """Process a plant section and update the bounds of its widgets."""
    processed_lines = []
    inside_plant = False
    plant_buffer = []
    x_offset = y_offset = 0

    for line in section.splitlines():
        stripped_line = line.strip()
        print(f"Processing line: {stripped_line}")
        if inside_plant:
            if '}' in stripped_line:
                inside_plant = False
                for plant_line in plant_buffer:
                    print(f"Updating plant line: {plant_line}")
                    updated_line = re.sub(r'bounds\(([^)]+)\)', lambda m: update_bounds(m.group(0), x_offset, y_offset), plant_line)
                    processed_lines.append(updated_line)
                processed_lines.append("")  # Add a line break after each plant
                plant_buffer = []
            else:
                plant_buffer.append(stripped_line)
        elif '{' in stripped_line and 'bounds' in stripped_line:
            inside_plant = True
            bounds_match = re.search(r'bounds\(([^)]+)\)', stripped_line)
            if bounds_match:
                bounds = [int(i) for i in re.findall(r'\d+', bounds_match.group(1))]
                if len(bounds) == 4:
                    x_offset, y_offset = bounds[0], bounds[1]
                    print(f"Found plant with bounds: {bounds} and offsets: ({x_offset}, {y_offset})")
            processed_lines.append(re.sub(r'\s*\{', '', stripped_line))
        else:
            processed_lines.append(stripped_line)

    return '\n'.join(processed_lines)

def process_file(file_path):
    """Process a single file to update plant sections."""
    with open(file_path, 'r') as file:
        content = file.read()
        cabbage_section = extract_cabbage_section(content)
        updated_section = process_plant_section(cabbage_section)
        updated_content = re.sub(r'<Cabbage>.*?</Cabbage>', f'<Cabbage>\n{updated_section}\n</Cabbage>', content, flags=re.DOTALL)

    output_path = os.path.join(os.path.dirname(__file__), 'updated_' + os.path.basename(file_path))
    with open(output_path, 'w') as file:
        file.write(updated_content)

    print(f"Processed file saved to: {output_path}")

def main():
    parser = argparse.ArgumentParser(description="Update plant sections in a .csd file.")
    parser.add_argument('file_path', type=str, help="Path to the .csd file to be processed")
    args = parser.parse_args()

    file_path = args.file_path
    if not os.path.isfile(file_path):
        print("The provided path is not a valid file.")
    else:
        process_file(file_path)

if __name__ == "__main__":
    main()
