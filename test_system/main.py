import sys
import os
import numpy as np


def get_file_size(path: str) -> int:
    f_stat = os.stat(path)
    return f_stat.st_size


def write_settings(delta: int, min_parity: int):
    path = r"../settings.h"
    with open(path, "w") as f:
        f.write(f"#define MIN_PARITY {min_parity}\n")
        f.write(f"#define DELTA {delta}\n")


def calc_compression(delta: int, min_parity: int) -> float:
    os.chdir("../build")
    write_settings(128, 2)
    os.system('make')
    stream = os.popen('./compress --output output.txt')
    output = stream.read()
    print(output)
    compressed_size = get_file_size('output.txt')
    raw_size = get_file_size('input.txt')
    return compressed_size / raw_size


def main():
    x = np.logspace(2, 16, num=14, base=2, dtype=int)
    print(x)
    compression = calc_compression(1024, 2)
    print(f"{compression=}")


if __name__ == '__main__':
    main()
