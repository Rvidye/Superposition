import os
import subprocess

def compile_spv(file, include_path):
    os.makedirs("spv", exist_ok=True)
    output_file = os.path.join("spv", f"{file}.spv")
    if not file.startswith("vk_"):
        return
    target_env = "vulkan1.3"
    print(f"Running: glslc {file} -o {output_file} -I {include_path} --target-env={target_env}")
    result = subprocess.run(
        f"glslc {file} -o {output_file} -I {include_path} --target-env={target_env}",
        shell=True,
        capture_output=True,
        text=True
    )
    if result.returncode != 0:
        print(f"Error compiling {file}:\n{result.stderr}")
    else:
        print(f"Successfully compiled {file} to {output_file}\n")

def recursive_compile(path, include_path):
    original_dir = os.getcwd()
    os.chdir(path)
    
    for file in os.listdir():
        if os.path.isdir(file) and file != "spv":
            recursive_compile(file, include_path)
        elif os.path.isfile(file) and os.path.splitext(file)[1][1:] in ["vert", "frag", "tesc", "tese", "geom", "comp"]:
            compile_spv(file, include_path)
    os.chdir(original_dir)

if __name__ == "__main__":
    current_script_dir = os.path.abspath(os.getcwd())
    include_dir = os.path.join(current_script_dir, "include")
    if not os.path.isdir(include_dir):
        print(f"Include directory not found at: {include_dir}")
        exit(1)
    recursive_compile(".", include_dir)
