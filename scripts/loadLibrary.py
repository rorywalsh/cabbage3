import sys
import ctypes
import os

# Default library path
DEFAULT_LIBRARY_PATH = "../cmake-build-debug/CabbageVST3Effect/out/CabbageVST3Effect.vst3/Contents/x86_64-linux/CabbageVST3Effect.so"

def load_library_and_find_symbol(symbol_name, library_path=DEFAULT_LIBRARY_PATH):
    if not os.path.exists(library_path):
        print(f"Error: Library file '{library_path}' does not exist.")
        sys.exit(1)

    try:
        # Load the shared object (.so) file
        lib = ctypes.CDLL(library_path)
        print(f"Successfully loaded: {library_path}")

        # Try to retrieve the symbol
        try:
            symbol = getattr(lib, symbol_name)
            print(f"Symbol '{symbol_name}' found in {library_path}")
        except AttributeError:
            print(f"Error: Symbol '{symbol_name}' not found in {library_path}")
            sys.exit(1)

    except OSError as e:
        print(f"Failed to load library: {e}")
        sys.exit(1)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python loadLibrary.py <symbol_name> [library_path]")
        sys.exit(1)

    symbol_name = sys.argv[1]
    library_path = sys.argv[2] if len(sys.argv) > 2 else DEFAULT_LIBRARY_PATH

    load_library_and_find_symbol(symbol_name, library_path)
