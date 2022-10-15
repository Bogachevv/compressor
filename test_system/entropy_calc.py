import matplotlib.pyplot as plt
import numpy as np


def calc_m(seq: str) -> int:
    return len(set(seq))


def predict_delta(x: int):
    return 0.0463 * (x**2) - 2.2580 * x + 31.2828


def main():
    path = '../public_tests/04_test_file_input/test_5'
    p = 100
    with open(path, "rb") as f:
        seq = f.read(p)
        x = []
        y = []
        i = 0
        while True:
            x.append(i)
            y.append(calc_m(seq))
            ch = f.read(1)
            if not ch:
                break
            seq = seq[1::] + ch
            i += 1

    mean = sum(y) / len(y)
    print(f"Predict delta: {predict_delta(mean)}")
    plt.xlabel("Position")
    plt.ylabel("Metric")
    plt.title(path + '\n' + f'{mean=:.5f}')
    plt.plot(x, y, 'm')
    plt.show()


if __name__ == '__main__':
    main()
