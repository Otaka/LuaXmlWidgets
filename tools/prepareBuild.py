import subprocess


# This script is used to generate the lua standard library file tree
def main():
    execute(["python3", "./tools/fileTreeSerializer.py", "-s", "./lua_std_lib/src", "./lua_std_lib/generated/lua_std_lib.dat"])
    execute(["python3", "./tools/binToC.py", "-i", "./lua_std_lib/generated/lua_std_lib.dat", "-o", "./lua_std_lib/generated/lua_std_lib.hpp", "-v", "LUA_STD_LIB"])

def execute(args):
    try:
        # Start the external application
        process = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = process.communicate()
    
        if process.returncode != 0:
            print(f"Subprocess failed with return code: {process.returncode}")
            print(f"Error output: {stderr.decode('utf-8')}")
            raise SystemExit(process.returncode)
    except Exception as e:
        print(f"An error occurred: {e}")
        raise SystemExit(1)
    
main()
