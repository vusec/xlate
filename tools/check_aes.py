#!/usr/bin/env python3

from argparse import ArgumentParser
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.figure import figaspect
from matplotlib import rc
import os.path

def main():
    parser = ArgumentParser(description='Plots the AES T-table results.')
    parser.add_argument('input', type=str, help='the path to the directory containing the data.')

    args = parser.parse_args()

    data = np.loadtxt(args.input)

    score = 0

    for x, y in zip([np.argmin(row) for row in data], range(len(data))):
        if x == y:
            score += 1

    print(score)

if __name__ == '__main__':
    main()
