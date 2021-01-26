import time
import argparse
import os
import subprocess
import shlex
from ruamel import yaml


def runcmd(cmd, *cmd_args, **subprocess_args):
    cmd = shlex.split(cmd) + list(cmd_args)
    #print(" ".join([f"'{a}'" for a in cmd]), flush=True)
    proc = subprocess.run(cmd, **subprocess_args)
    return proc


def getoutput(cmd, *cmd_args, **subprocess_args):
    proc = runcmd(cmd, *cmd_args, **subprocess_args, check=True,
                  stdout=subprocess.PIPE)
    return proc.stdout.decode("utf8")


def start_build(args):
    ts = time.time()
    with open(args.out, "w") as f:
        f.write(str(ts))


def finish_build(args):
    ts = time.time()
    with open(args.out, "r") as f:
        start = float(f.read())
    duration = ts - start
    results = {
        'compile': f"{duration:.3f}s",
        'file_size': f"{os.path.getsize(args.exe)}B"
    }
    s = yaml.dump({args.target: results})
    print(s, flush=True, end="")
    ## too much output:
    #if args.unix:
    #    # https://stackoverflow.com/questions/35485
    #    results['size'] = getoutput('size', args.exe)
    #    #results['symbols'] = getoutput('nm -t d -l -S --size-sort', args.exe)
    s = yaml.dump({args.target: results})
    with open(args.out, "w") as f:
        f.write(s)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers()
    #
    sp = subparsers.add_parser("start")
    sp.set_defaults(func=start_build)
    sp.add_argument('target', type=str, help='the target name')
    #
    sp = subparsers.add_parser("finish")
    sp.set_defaults(func=finish_build)
    sp.add_argument('target', type=str, help='the target name')
    sp.add_argument('exe', type=str, help='the executable file')
    sp.add_argument('-u', '--unix', action="store_true", help='use unix style size reporters')
    #
    args = parser.parse_args()
    args.out = f"{args.target}.dat"
    args.func(args)
