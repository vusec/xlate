#!/usr/bin/env python3

from argparse import ArgumentParser
import numpy as np
import matplotlib.pyplot as plt
from matplotlib import rc
import os.path

def main():
    parser = ArgumentParser(description='Plots the AES T-table results.')
    parser.add_argument('input', type=str, help='the path to the directory containing the data.')
    parser.add_argument('-o', '--output', type=str, help='the path to the PDF-file to output.', default='test.pdf')
    parser.add_argument('--cmap', type=str, help='the colour map to use.', default='Greens')
    parser.add_argument('--title', type=str, help='the title to use for the plot.')
    parser.add_argument('--inverse', type=str, help='invert the colour map.')

    args = parser.parse_args()

    rc('text', usetex=True)

    data = np.loadtxt(args.input)
    data = np.array([(row - np.min(row)) / ((np.max(row) - np.min(row)) or 1) for row in data])

    if args.inverse == 'y':
        data = 1.0 - data

    if args.title:
        plt.title(args.title)

    plt.pcolormesh(data, cmap=args.cmap)
    plt.xlabel('$p[0]$')
    plt.ylabel('{\\tt Te0} offset')
    plt.xticks(np.arange(17), [i for i in range(0, 256 + 1, 16)])
    plt.yticks(np.arange(17), ['\\tt' + hex(0x1584c0 + i) for i in range(0, 0x400, 64)])
    plt.tight_layout()
    plt.savefig(args.output)

if __name__ == '__main__':
    main()
