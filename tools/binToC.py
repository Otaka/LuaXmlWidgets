# Converts binary file to C string
import argparse
import os


def main():
    parser = argparse.ArgumentParser(description='Convert a binary file to a C multiline string.')
    parser.add_argument('-i', '--input', help='The input binary file to convert.', required=True, dest='input_file')
    parser.add_argument('-o', '--output', help='The output C++ file to write the multiline string to.', required=True,
                        dest='output_file', )
    parser.add_argument('-v', '--var', help='The name of the variable to store the multiline string in.', required=True,
                        dest='variable_name')
    parser.add_argument('-c', '--chunk_size', type=int, default=512, help='The number of bytes to read at a time.')
    parser.add_argument('-a', '--append', action='store_true',
                        help='Append to the output file instead of overwriting it.')

    args = parser.parse_args()
    convert_file_to_multiline_string(args.input_file, args.output_file, args.variable_name, args.chunk_size,
                                     args.append)


def convert_file_to_multiline_string(input_file_path, output_file_path, variable_name, chunk_size=512, append=False):
    with open(input_file_path, 'rb') as input_file:
        converted_lines = []
        previous_is_hex = False
        while True:
            chunk = input_file.read(chunk_size)
            if not chunk:
                break
            converted_chunk = ''
            for byte in chunk:
                converted_byte = convert_byte(byte, previous_is_hex)
                if converted_byte.startswith('\\x'):
                    previous_is_hex = True
                else:
                    previous_is_hex = False
                converted_chunk += converted_byte

            converted_lines.append(converted_chunk)

    # Join the lines with line continuation character
    multiline_string = '\\\n'.join(converted_lines)
    file_size = os.path.getsize(input_file_path)

    output_string = f'const char*{variable_name}="{multiline_string}";\nconst unsigned long {variable_name}_size={file_size};\n'

    if append:
        with open(output_file_path, 'a') as output_file:
            output_file.write(output_string)
    else:
        with open(output_file_path, 'w') as output_file:
            output_file.write(output_string)

    print(f'Conversion complete. Output written to {output_file_path}')


def is_printable_ascii(byte):
    """Check if a byte has a printable ASCII representation."""
    return 32 <= byte <= 126  # Printable ASCII range


# we should do smart conversion, because if we have the following bytes \x92, 'a' and after conversion it will become \x92a - the compiler will not understand it as a single hex value
def convert_byte(byte, previous_is_hex):
    """Convert a byte to its string representation."""
    # if it is new line - return '\\n'
    if byte == 10:
        return '\\n'
    # if it is carriage return - return '\\r'
    if byte == 13:
        return '\\r'
    # if it is tab - return '\\t'
    if byte == 9:
        return '\\t'
    # if it is backslash - return '\\\\'
    if byte == 92:
        return '\\\\'
    # if it is double quote - return '\\"'
    if byte == 34:
        return '\\"'
    if previous_is_hex:
        if is_printable_ascii(byte):
            # check if byte is in 'a'-'f' or 'A'-'F' and '0'-'9' range - convert as \xHH, in other case print as it is
            if (48 <= byte <= 57) or (65 <= byte <= 70) or (97 <= byte <= 102):
                return '\\x{:02x}'.format(byte)
            else:
                return chr(byte)
    if is_printable_ascii(byte):
        return chr(byte)
    else:
        return '\\x{:02x}'.format(byte)


main()
