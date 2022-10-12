import sys
import os

import matplotlib.pyplot as plt
import matplotlib.scale
import numpy as np


def get_file_size(path: str) -> int:
    f_stat = os.stat(path)
    return f_stat.st_size


def write_settings(delta: int, min_parity: int):
    path = r"../settings.h"
    with open(path, "w") as f:
        f.write(f"#define MIN_PARITY {min_parity}\n")
        f.write(f"#define DELTA {delta}\n")


def calc_compression(delta: int, min_parity: int, path: str = "input.txt") -> float:
    os.chdir("../build")
    write_settings(delta, min_parity)
    stream = os.popen('make')
    stream.read()
    stream = os.popen(f'./compress --input {path} --output output.txt')
    output = stream.read()
    # print(output)
    compressed_size = get_file_size('output.txt')
    raw_size = get_file_size(path)
    # return compressed_size / raw_size
    return compressed_size


def mean_compression(delta: int, files) -> float:
    return sum(calc_compression(delta, 2, path) for path in files)


def main():
    min_delta_pow = 2
    max_delta_pow = 26
    x = np.logspace(min_delta_pow, max_delta_pow, num=max_delta_pow - min_delta_pow + 1, base=2, dtype=int)

    files = [f'../public_tests/0{i}_test_file_input/test_{i+1}' for i in range(2, 7) if i != 3]
    y = np.array([mean_compression(delta, files) for delta in x], dtype=float)
    print(*zip(x, y), sep='\n')
    plt.xlabel("Delta")
    plt.ylabel("Compression")
    plt.xscale("log", base=2)
    plt.yscale("linear")
    # plt.ylim(min_y - 0.1, max_y + 0.1)

    plt.plot(x, y)
    plt.show()


if __name__ == '__main__':
    main()

