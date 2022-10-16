import numpy as np
from collections import defaultdict
import os


def calc_H(path: str):
    dd = [1] * 256
    f_len = 0
    with open(path, "rb") as f:
        while ch := f.read(1):
            ch = ord(ch)
            dd[ch] += 1
            f_len += 1
    return sum((1 / c) * np.log2(c) for c in dd)


def main():
    os.chdir("../build")
    files = [f'../public_tests/0{i}_test_file_input/test_{i+1}' for i in range(1, 7) if i != 3]
    files += ['./war_and_peace.txt', 'idiot.txt', 'big_binary']
    for path in files:
        print(f"{path}: {calc_H(path):.5f}")


if __name__ == '__main__':
    main()
