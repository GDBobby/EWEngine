import sys
import subprocess
from pathlib import Path

def compile_shaders():
    if len(sys.argv) < 2:
        print("Usage: python compile_shaders.py <input_directory>")
        sys.exit(1)

    input_dir = Path(sys.argv[1]).resolve()
    
    if not input_dir.is_dir():
        print(f"Error: Directory '{input_dir}' does not exist.")
        sys.exit(1)
        
    parent_dir = input_dir.parent

    glslc_binary = sys.argv[2] if len(sys.argv) > 2 else "glslc"


    ALLOWED_EXTENSIONS = {'.vert', '.frag', '.comp', '.mesh'}
    shader_files = [
        f for f in input_dir.iterdir() 
        if f.is_file() and f.suffix.lower() in ALLOWED_EXTENSIONS
    ]

    if not shader_files:
        print(f"No files found in {input_dir}")
        return

    skipped_count = 0

    for file_path in shader_files:
        output_path = parent_dir / f"{file_path.name}.spv"

        if output_path.exists():
            source_mtime = file_path.stat().st_mtime
            output_mtime = output_path.stat().st_mtime
            
            if source_mtime <= output_mtime:
                print(f"Skipped: {file_path.name} (Up to date)")
                skipped_count += 1
                continue
        
        print(f"Compiling {file_path.name} -> {output_path.name}...")
        
        try:
            subprocess.run(
                ["glslc", str(file_path), "-o", str(output_path)], 
                check=True
            )
        except FileNotFoundError:
            print(f"Error: Compiler '{glslc_binary}' could not be found.")
            print("Make sure the path is correct or that the Vulkan SDK is added to your system PATH.")
            sys.exit(1)
        except subprocess.CalledProcessError:
            print(f"Failed to compile {file_path.name}")

    print(f"\nDone! All compiled shaders placed in: {parent_dir}")

if __name__ == "__main__":
    compile_shaders()