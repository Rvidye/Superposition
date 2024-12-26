import os
import subprocess

def compile_spv(file: str):
    os.makedirs("spv", exist_ok=True)
    target_env = "opengl"
    if file.startswith("vk_"):
        target_env = "vulkan1.3"
    print(f"Running: glslc {file} -o spv/{file}.spv --target-env={target_env}")
    subprocess.run(f"glslc {file} -o spv/{file}.spv --target-env={target_env}")
    print()

def recursive_compile(path):
    os.chdir(path)
    for file in os.listdir():
        if os.path.isdir(file) and file != "spv":
            recursive_compile(file)

        if os.path.isfile(file) and os.path.splitext(file)[1][1:] in ["vert", "frag", "tesc", "tese", "geom", "comp"]:
            compile_spv(file)
    os.chdir("..")

recursive_compile(".")