#!/usr/bin/env python3

import os
import sys
import shutil
import subprocess
import argparse

def RepoRelative(path):
    return os.path.abspath(os.path.join(os.path.dirname(__file__), '..', path))

def main():
    parser = argparse.ArgumentParser(description='Generate spirv code')
    parser.add_argument('--glslang', action='store', type=str, help='Path to glslangValidator to use')
    args = parser.parse_args()

    glslang = 'glslang'
    if args.glslang:
        glslang = args.glslang
    if not shutil.which(glslang):
        sys.exit("Cannot find glslangValidator " + glslang)

    shaders_to_compile = []

    shaders_dir = RepoRelative('shaders')
    for filename in os.listdir(shaders_dir):
        if (filename.split(".")[-1] == 'comp'):
            shaders_to_compile.append(os.path.join(shaders_dir, filename))

    for shader in shaders_to_compile:
        try:
            args = [glslang, "-V", shader, "--target-env", "vulkan1.3", "-o", f'{shader}.spv']
            print(args)
            subprocess.check_output(args, universal_newlines=True)
        except subprocess.CalledProcessError as e:
            raise Exception(e.output)

if __name__ == '__main__':
  main()
