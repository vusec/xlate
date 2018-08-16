#!/usr/bin/env python3

from argparse import ArgumentParser
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.figure import figaspect
from matplotlib import rc
import os.path

def main():
    names = [
        '\\flushreload',
        '\\flushflush',
        '\\primeprobe',
        '\\primeabort',
        '\\xlateprobe',
        '\\xlateabort',
    ]

    data = np.loadtxt('timings.txt')

    print('\\begin{table}[ht!]')
    print('\\centering')
    print('\\small')
    print('\\begin{tabular}{ l | c }')
    print('Name & Time \\\\')
    print('\\hline')

    for i, name, row in zip(range(len(names)), names, data):
        if i % 2 == 0:
            print('\\rowcolor{blue!5}')

        print('{} & {} \\\\'.format(name, np.average(row)))

    print('\\end{tabular}')
    print('\\end{table}')

if __name__ == '__main__':
    main()
