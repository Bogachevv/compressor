import sys
import os

import matplotlib.pyplot as plt
import matplotlib.scale
import numpy as np


def get_file_size(path: str) -> int:
    f_stat = os.stat(path)
    return f_stat.st_size


def write_settings(delta: int, min_parity: int, shape: int):
    path = r"../settings.h"
    with open(path, "w") as f:
        f.write(f"#define MIN_PARITY {min_parity}\n")
        f.write(f"#define DELTA {delta}\n")
        f.write(f"#define SHAPE {shape}\n")


def calc_compression(delta: int, min_parity: int, shape: int, path: str = "input.txt") -> float:
    os.chdir("../build")
    write_settings(delta, min_parity, shape)
    stream = os.popen('make')
    stream.read()
    stream = os.popen(f'./compress --input {path} --output output.txt --method ppm')
    output = stream.read()
    # print(output)
    compressed_size = get_file_size('output.txt')
    raw_size = get_file_size(path)
    return compressed_size / raw_size
    # return compressed_size


def mean_compression(delta: int, files) -> float:
    return sum(calc_compression(delta, 2, path) for path in files)


def calc_entropy(path: str) -> float:
    dd = [1] * 256
    f_len = 0
    with open(path, "rb") as f:
        while ch := f.read(1):
            ch = ord(ch)
            dd[ch] += 1
            f_len += 1
    return sum((1 / c) * np.log2(c) for c in dd)


def calc_optimal_delta(path: str, shape) -> float:
    min_delta_pow = 2
    max_delta_pow = 14
    deltas = np.logspace(min_delta_pow, max_delta_pow, num=max_delta_pow - min_delta_pow + 1, base=2, dtype=int)
    compress_size = np.array([calc_compression(delta, 2, shape=shape, path=path) for delta in deltas], dtype=float)
    return deltas[np.argmin(compress_size)]


def main():
    os.chdir("../build")
    files = [f'../public_tests/0{i}_test_file_input/test_{i+1}' for i in range(1, 7) if i != 3]
    files += ['./war_and_peace.txt', 'idiot.txt', 'big_binary']
    # path = f'../public_tests/0{6}_test_file_input/test_{7}'
    colors = ['r', 'g', 'b', 'k', 'm', 'y', 'c']
    # delta = 2 ** 18
    for shape in range(0, 8):
        plt.title(f"shape = {shape}")
        for i, path in enumerate(files):
            entropy = calc_entropy(path)
            opt_delta = calc_optimal_delta(path, shape)
            print(f"{entropy}\t{opt_delta}")
            plt.scatter(i, entropy, c='r')
            plt.scatter(i, np.log2(opt_delta), c='b')
        plt.show()


if __name__ == '__main__':
    main()

