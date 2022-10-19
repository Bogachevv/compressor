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
    os.chdir("../build")
    min_delta_pow = 4
    max_delta_pow = 28
    min_shape = 0
    max_shape = 7
    deltas = np.linspace(min_delta_pow, max_delta_pow, num=max_delta_pow - min_delta_pow + 1, dtype=int)
    shapes = np.linspace(min_shape, max_shape, num=max_shape - min_shape + 1, dtype=int)
    fname = "test_5"
    path = f'./logs/{fname}.log'
    X, Y = np.meshgrid(deltas, shapes)
    with open(path, "r") as f:
        Z = []
        for line in f.readlines():
            Z += list(map(float, line.strip().split('\t')))
        print(f"min={min(Z):.8f}")
        print(f"max={max(Z):.8f}")
        Z = np.array(Z).reshape(len(shapes), len(deltas))
    print(Z)


    fig = plt.figure()
    ax = plt.axes(projection='3d')
    # ax.set_zlim(0, 0.0002000)
    ax.plot_surface(X, Y, Z, rstride=1, cstride=1, cmap='viridis', edgecolor='none')
    ax.set_xlabel("delta")
    ax.set_ylabel("shape")
    plt.show()


if __name__ == '__main__':
    main()

