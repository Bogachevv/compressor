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


def main():
    min_delta_pow = 4
    max_delta_pow = 14
    deltas = np.logspace(min_delta_pow, max_delta_pow, num=7, base=2, dtype=int)
    x = np.linspace(0, 8, num=7, dtype=int)

    files = [f'../public_tests/0{i}_test_file_input/test_{i+1}' for i in range(1, 7) if i != 3]
    files += ['./war_and_peace.txt', 'idiot.txt', 'big_binary']
    # path = f'../public_tests/0{6}_test_file_input/test_{7}'
    colors = ['r', 'g', 'b', 'k', 'm', 'y', 'c']
    # delta = 2 ** 18
    for path in files:
        print(f"Testing file {path}")
        for i, delta in enumerate(deltas):
            y = np.array([calc_compression(delta, 2, shape, path) for shape in x], dtype=float)
            print(*zip(x, y), sep='\n')
            # plt.ylim(min_y - 0.1, max_y + 0.1)
            plt.xlabel("Shape")
            plt.ylabel("Compression")
            # plt.xscale("log", base=2)
            plt.xscale("linear")
            plt.yscale("linear")
            plt.title(f"{path}")
            plt.plot(x, y, colors[i])
        plt.show()


if __name__ == '__main__':
    main()

