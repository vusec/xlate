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
    parser.add_argument('-o', '--output', type=str, help='the path to the PDF-file to output.', default='test.pdf')

    args = parser.parse_args()

    rc('text', usetex=True)

    names = [
        ('pp-colouring.txt', '{\\tt\\sc Prime + Probe} (coloring)', 'Greens'),
        ('pa-colouring.txt', '{\\tt\\sc Prime + Abort} (coloring)', 'Greens_r'),
        ('xp-colouring.txt', '{\\tt\\sc Xlate + Probe} (coloring)', 'Greens'),
        ('xa-colouring.txt', '{\\tt\\sc Xlate + Abort} (coloring)', 'Greens_r'),

        ('pp-4ways.txt', '{\\tt\\sc Prime + Probe} (4 ways)', 'Greens'),
        ('pa-4ways.txt', '{\\tt\\sc Prime + Abort} (4 ways)', 'Greens_r'),
        ('xp-4ways.txt', '{\\tt\\sc Xlate + Probe} (4 ways)', 'Greens'),
        ('xa-4ways.txt', '{\\tt\\sc Xlate + Abort} (4 ways)', 'Greens_r'),

        ('pp-8ways.txt', '{\\tt\\sc Prime + Probe} (8 ways)', 'Greens'),
        ('pa-8ways.txt', '{\\tt\\sc Prime + Abort} (8 ways)', 'Greens_r'),
        ('xp-8ways.txt', '{\\tt\\sc Xlate + Probe} (8 ways)', 'Greens'),
        ('xa-8ways.txt', '{\\tt\\sc Xlate + Abort} (8 ways)', 'Greens_r'),

        ('pp-12ways.txt', '{\\tt\\sc Prime + Probe} (12 ways)', 'Greens'),
        ('pa-12ways.txt', '{\\tt\\sc Prime + Abort} (12 ways)', 'Greens_r'),
        ('xp-12ways.txt', '{\\tt\\sc Xlate + Probe} (12 ways)', 'Greens'),
        ('xa-12ways.txt', '{\\tt\\sc Xlate + Abort} (12 ways)', 'Greens_r'),
    ]

    fig = plt.figure(figsize=(12.5, 12))

    axes = []

    for (name, title, cmap), coord in zip(names, np.ndindex((4, 4))):
        ax = plt.subplot2grid((4, 4), coord)
        axes.append(ax)

        plt.xticks(rotation=90, fontsize=10)
        plt.yticks(fontsize=10)

        data = np.loadtxt(os.path.join(args.input, name))

        # TODO: fix the output of the programs.
        if name == 'fr.txt' or name == 'ff.txt':
            data = np.transpose(data)

        data = [(row - np.min(row)) / ((np.max(row) - np.min(row)) or 1) for row in data]
        ax.set_title(title, y=1.08)
        ax.pcolormesh(data, cmap=cmap)
        ax.set_xlabel('$p[0]$')
        ax.set_ylabel('{\\tt Te0} offset')
        ax.set_xticklabels([i for i in range(0, 256 + 1, 32)])
        ax.set_yticklabels(['\\tt' + hex(0x1584c0 + i) for i in range(0, 0x400, 64)])
        ax.set_aspect('equal', adjustable='box')

    plt.tight_layout()
    plt.savefig(args.output)

if __name__ == '__main__':
    main()
