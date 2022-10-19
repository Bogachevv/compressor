import os
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


def get_fname(path: str) -> str:
    return path.strip().split('/')[-1]


def main():
    os.chdir("../build")
    min_delta_pow = 4


    max_delta_pow = 28
    min_shape = 0
    max_shape = 7
    deltas = np.logspace(min_delta_pow, max_delta_pow, num=max_delta_pow-min_delta_pow+1, base=2, dtype=int)
    shapes = np.linspace(min_shape, max_shape, num=max_shape-min_shape+1, dtype=int)

    files = [f'../public_tests/0{i}_test_file_input/test_{i+1}' for i in range(0, 7) if i != 3]
    files += ['./war_and_peace.txt', 'idiot.txt']

    for path in files:
        print(f"Start working with {path}")
        with open(f"logs/{get_fname(path)}.log", "w", encoding="utf-8") as f:
            for shape in shapes:
                print(f"{shape=}")
                for delta in deltas:
                    print(f"{calc_compression(delta, 2, shape, path):.7f}", end='\t', file=f)
                print(file=f)


if __name__ == '__main__':
    main()

