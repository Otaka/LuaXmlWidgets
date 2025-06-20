import os
import shutil
import sys

file_chunk_index_generator = 0

help_message = """
The app can serialize/deserialize folder with files to/from a single binary file

Usage:
 python3 fileTreeSerializer.py (--serialize/-s) /path/input/src/folder /path/to/output/serialized/dst/file
 python3 fileTreeSerializer.py (--deserialize/-d) /path/to/output/dst/folder /path/to/input/serialized/src/file
 
 Format of the binary file:
 The binary file consists of two parts:
 1. Files directory - sequential list of file/folder chunks. Folder chunks has children chunks indexes.
 2. Files content - just concatenated content of all files. offsets and sizes are in the files directory.

 The files directory is a structure that consists of sequential list of file chunks.
 Structure:
 1. i32 that represents the number of file chunks
 2. Each file chunk is a structure that consists of:
  Sized string that represents the file name
  i8 that represents a boolean value that indicates if the chunk is file(0) or directory(1)
  If the file chunk is a directory:
    1. i32 that represents the number of children
    2. array of i32 that represents the index of each child
  If the file chunk is a file:
    1. i32 that represents the size of the file
    2. i32 that represents the offset of the file content in the same file relative to the end of the file directory
The root chunk is always at the end of the file directory.

Data types:
Sized string - i32 that represents the length of the string(including null terminator), content in utf-8, 1 byte for the null terminator
i8 - 1 byte
i32 - 4 bytes(little endian)
i32_array
"""


def main():
    if len(sys.argv) < 2:
        print("Invalid number of arguments.")
        print(help_message)
        sys.exit(1)

    mode = sys.argv[1]
    if mode not in ["--serialize", "-s", "--deserialize", "-d", "--help", "-h"]:
        print("Invalid mode. Allowed modes are [--serialize/-s --deserialize/-d]")
        sys.exit(1)

    if mode in ["--help", "-h"]:
        print(help_message)
        sys.exit(0)
    if mode in ["--serialize", "-s"]:
        if len(sys.argv) != 4:
            print("Invalid number of arguments. For serialization.")
            print(help_message)
            sys.exit(1)
        input_folder = sys.argv[2]
        output_file = sys.argv[3]
        serialize_folder(input_folder, output_file)
    if mode in ["--deserialize", "-d"]:
        if len(sys.argv) != 4:
            print("Invalid number of arguments. For deserialization.")
            print(help_message)
            sys.exit(1)
        output_folder = sys.argv[2]
        input_serialized_file = sys.argv[3]
        deserialize_folder(output_folder, input_serialized_file)


def serialize_folder(input_folder, output_file):
    inputFolderAbsPath = os.path.abspath(input_folder)
    # input folder must exist
    if not os.path.exists(inputFolderAbsPath):
        print("The input folder " + inputFolderAbsPath + " does not exist")
        sys.exit(1)
    if not os.path.isdir(inputFolderAbsPath):
        print("The path " + inputFolderAbsPath + " is not a directory")
        sys.exit(1)

    outputFileAbsPath = os.path.abspath(output_file)
    # output file cannot be in the input folder
    if os.path.commonpath([inputFolderAbsPath, outputFileAbsPath]) == inputFolderAbsPath:
        print("The output file " + outputFileAbsPath + " cannot be in the input folder " + inputFolderAbsPath)
        sys.exit(1)

    latest_input_files_modify_time = get_latest_modify_time_recursive(inputFolderAbsPath)
    if os.path.exists(outputFileAbsPath):
        latest_output_file_modify_time = os.path.getmtime(outputFileAbsPath)
        if latest_output_file_modify_time > latest_input_files_modify_time:
            print("The output file " + outputFileAbsPath + " is up to date")
            sys.exit(0)
        # delete the output file
        os.remove(outputFileAbsPath)
    parent_folder = os.path.dirname(outputFileAbsPath)

    tmp_directory_file = os.path.join(parent_folder, "tmp_directory.dir")
    tmp_content_file = os.path.join(parent_folder, "tmp_content.dat")
    if os.path.exists(tmp_directory_file):
        os.remove(tmp_directory_file)
    if os.path.exists(tmp_content_file):
        os.remove(tmp_content_file)

    file_chunks = prepare_directory(inputFolderAbsPath)
    current_content_offset = 0
    with open(tmp_directory_file, "wb") as directory_file:
        with open(tmp_content_file, "wb") as content_file:
            write_int(directory_file, len(file_chunks))
            for chunk in file_chunks:
                write_string(directory_file, chunk["name"])
                write_bool(directory_file, chunk["is_dir"])
                if chunk["is_dir"]:
                    write_int(directory_file, len(chunk["children"]))
                    for child in chunk["children"]:
                        write_int(directory_file, child)
                else:
                    write_int(directory_file, chunk["size"])
                    write_int(directory_file, current_content_offset)
                    with open(chunk["full_path"], "rb") as file:
                        # copy file content to the content file
                        content = file.read()
                        content_file.write(content)
                        current_content_offset += chunk["size"]
    concatenate_files(outputFileAbsPath, [tmp_directory_file, tmp_content_file])
    os.remove(tmp_directory_file)
    os.remove(tmp_content_file)


def deserialize_folder(output_folder_path, input_file_path):
    outputFolderAbsPath = os.path.abspath(output_folder_path)
    # output folder must exist
    if not os.path.exists(outputFolderAbsPath):
        print("The output folder " + outputFolderAbsPath + " does not exist")
        sys.exit(1)
    if not os.path.isdir(outputFolderAbsPath):
        print("The path " + outputFolderAbsPath + " is not a directory")
        sys.exit(1)
    # output folder should be emtpy
    if len(os.listdir(outputFolderAbsPath)) > 0:
        print("The output folder " + outputFolderAbsPath + " is not empty")
        sys.exit(1)
    inputFileAbsPath = os.path.abspath(input_file_path)
    if not os.path.exists(inputFileAbsPath):
        print("The input file " + inputFileAbsPath + " does not exist")
        sys.exit(1)
    if not os.path.isfile(inputFileAbsPath):
        print("The path " + inputFileAbsPath + " is not a file")
        sys.exit(1)
    file_structure = read_file_chunks(inputFileAbsPath)
    recreate_file_chunk(outputFolderAbsPath, outputFolderAbsPath, file_structure["file_chunks"][-1],
                        file_structure["file_chunks"],
                        file_structure["content_offset"], inputFileAbsPath)


def recreate_file_chunk(root_output_folder, parent_folder, chunk, all_chunks, content_file_global_offset,
                        content_file_path):
    if chunk["is_dir"]:
        chunk_name = chunk["name"]
        if chunk["name"] == "/":
            chunk_name = ""
        dir_path = os.path.join(parent_folder, chunk_name)
        validate_file_in_folder(dir_path, root_output_folder)
        if chunk["name"] != "/":
            os.makedirs(dir_path)
        for child_index in chunk["children"]:
            child_chunk = all_chunks[child_index]
            recreate_file_chunk(root_output_folder, dir_path, child_chunk, all_chunks, content_file_global_offset,
                                content_file_path)
    else:
        file_path = os.path.join(parent_folder, chunk["name"])
        validate_file_in_folder(file_path, root_output_folder)
        with open(content_file_path, "rb") as content_file:
            content_file.seek(content_file_global_offset + chunk["offset"])
            with open(file_path, "wb") as file:
                file.write(content_file.read(chunk["size"]))


def validate_file_in_folder(file_path, folder_path):
    absolute_file_path = os.path.abspath(file_path)
    absolute_folder_path = os.path.abspath(folder_path)
    # is file somewhere in the folder
    if os.path.commonpath([absolute_file_path, absolute_folder_path]) != absolute_folder_path:
        raise ValueError(
            "Something wrong. The file '" + absolute_file_path + "' is not in the output folder '" + absolute_folder_path + "'")


def read_file_chunks(input_file):
    result = {"content_offset": 0, "file_chunks": []}
    with open(input_file, "rb") as inputfile:
        file_chunks_count = read_int(inputfile)
        for _ in range(file_chunks_count):
            chunk = {}
            chunk["name"] = read_string(inputfile)
            chunk["is_dir"] = read_bool(inputfile)
            if chunk["is_dir"]:
                children_count = read_int(inputfile)
                chunk["children"] = []
                for _ in range(children_count):
                    child_index = read_int(inputfile)
                    chunk["children"].append(child_index)
            else:
                chunk["size"] = read_int(inputfile)
                chunk["offset"] = read_int(inputfile)
            result["file_chunks"].append(chunk)
        result["content_offset"] = inputfile.tell()
    return result


def concatenate_files(output_file, input_files_list):
    with open(output_file, "wb") as fext:
        for input_file in input_files_list:
            with open(input_file, "rb") as fo:
                shutil.copyfileobj(fo, fext)


def prepare_directory(folder):
    global file_chunk_index_generator
    chunks_list = []
    nested_indexes = _prepare_directory(folder, chunks_list)
    index = file_chunk_index_generator
    file_chunk_index_generator += 1
    dir_info = {"index": index, "size": 0, "name": "/", "is_dir": True,
                "children": nested_indexes}
    chunks_list.append(dir_info)
    return chunks_list


def _prepare_directory(folder, chunks_list):
    global file_chunk_index_generator
    children_chunks_ids = []
    for file in os.listdir(folder):
        file_path = os.path.join(folder, file)
        if os.path.islink(file_path):
            continue
        if os.path.isdir(file_path):
            dir_path = file_path
            nested_child_chunks_ids = _prepare_directory(dir_path, chunks_list)
            index = file_chunk_index_generator
            file_chunk_index_generator += 1
            dir_name = os.path.basename(dir_path)
            file_info = {"index": index, "size": 0, "name": dir_name, "is_dir": True,
                         "children": nested_child_chunks_ids}
            chunks_list.append(file_info)
            children_chunks_ids.append(index)
        else:
            size = os.path.getsize(file_path)
            index = file_chunk_index_generator
            file_chunk_index_generator += 1
            file_name = os.path.basename(file_path)
            file_info = {"index": index, "size": size, "name": file_name, "is_dir": False, "children": [],
                         "full_path": file_path}
            chunks_list.append(file_info)
            children_chunks_ids.append(index)
    return children_chunks_ids


def reverse_file_chunks(file_chunks):
    reversed_file_chunks = file_chunks.reverse()
    return reversed_file_chunks


def read_int(file):
    return int.from_bytes(file.read(4), byteorder='little')


def read_bool(file):
    return file.read(1) == b'\1'


def read_string(file):
    length = read_int(file)
    string = file.read(length - 1).decode('utf-8')
    file.read(1)  # skip null terminator
    return string


def write_int(file, number):
    file.write(number.to_bytes(4, byteorder='little'))


def write_bool(file, value):
    file.write(b'\1' if value else b'\0')


def write_string(file, string):
    bytes = string.encode('utf-8')
    write_int(file, len(bytes) + 1)
    file.write(bytes)
    file.write(b'\0')


def get_latest_modify_time_recursive(folder):
    latest_modify_time = os.path.getmtime(folder)
    for root, dirs, files in os.walk(folder, topdown=False, followlinks=False):
        for file in files:
            file_path = os.path.join(root, file)
            if os.path.islink(file_path):
                continue
            modify_time = os.path.getmtime(file_path)
            if modify_time > latest_modify_time:
                latest_modify_time = modify_time
        for dir in dirs:
            dir_path = os.path.join(root, dir)
            if os.path.islink(dir_path):
                continue
            modify_time = os.path.getmtime(dir_path)
            if modify_time > latest_modify_time:
                latest_modify_time = modify_time
    return latest_modify_time


main()
