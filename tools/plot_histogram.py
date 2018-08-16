#!/usr/bin/env python3

from argparse import ArgumentParser
import numpy as np
import matplotlib.pyplot as plt
from matplotlib import rc

def main():
    parser = ArgumentParser(description='Plots a histogram of cache hits and misses.')
    parser.add_argument('input', type=str, help='the path to the data file.')
    parser.add_argument('-o', '--output', type=str, help='the path to the PDF-file to output.', default='test.pdf')
    parser.add_argument('--min', type=int, help='the minimum latency to show.', default=0)
    parser.add_argument('--max', type=int, help='the maximum latency to show.', default=300)
    parser.add_argument('--bins', type=int, help='the amount of bins to use for the histogram.', default=100)
    parser.add_argument('--title', type=str, help='the title to use for the plot.')

    args = parser.parse_args()

    rc('text', usetex=True)

    plt.hist(np.loadtxt(args.input), args.bins, range=(args.min, args.max),
        color=('red', 'lime'), label=('Cache Miss', 'Cache Hit'))
    plt.xlim(args.min, args.max)
    plt.xlabel('Latency')
    plt.ylabel('Occurrences')
    plt.legend()

    if args.title:
        plt.title(args.title)

    plt.tight_layout()
    plt.savefig(args.output)

if __name__ == '__main__':
    main()
