#!/usr/bin/python3
# coding: utf-8

import os
import sys
import subprocess

# alias n="n.py"
def main():
    match sys.argv[1:]:
        case ['init']: return init()
        case ['clean']: return e('rm -rf build')

        case ['run']: return run('debug', [])
        case ['run', '--', *_]: return run('debug', sys.argv[3:])

        case ['run', 'release']: return run('release', [])
        case ['run', 'release', '--', *_]: return run('release', sys.argv[4:])

    return 1

def init():
    return not (e('rm -rf build') == 0 and
                e('meson setup build') == 0 and
                e('meson configure --warnlevel 3 --debug -Db_sanitize=address,undefined build') == 0)

def run(build_type, args):
    if not os.path.exists('build'):
        if init() != 0:
            return 1

    if build_type != get_buildtype():
        match build_type:
            case   'debug': sanitize = 'address,undefined'
            case 'release': sanitize = 'none'

        if rc := e(f'meson configure --buildtype {build_type} -Db_sanitize={sanitize} build'):
            return rc

    return not (e('meson compile -C build') == 0 and
                e(f'./build/clox {" ".join(args)}') == 0)

def e(command):
    print(f'\033[92m+ {command}\033[0m')
    return subprocess.run(command, shell=True).returncode

def get_buildtype():
    config = os.popen('meson configure build').readlines()

    for line in config:
        if 'buildtype' not in line:
            continue

        return [n for n in line.split(' ') if n != ''][1]

if __name__ == '__main__':
    exit(main())
