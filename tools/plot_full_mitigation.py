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
        ('pp-no-colour.txt', '{\\tt\\sc Prime + Probe} (no coloring)', 'Greens'),
        ('pp-pg-colour.txt', '{\\tt\\sc Prime + Probe} (page coloring)', 'Greens'),
        ('pp-pt-colour.txt', '{\\tt\\sc Prime + Probe} (full coloring)', 'Greens'),

        ('pa-no-colour.txt', '{\\tt\\sc Prime + Abort} (no coloring)', 'Greens_r'),
        ('pa-pg-colour.txt', '{\\tt\\sc Prime + Abort} (page coloring)', 'Greens_r'),
        ('pa-pt-colour.txt', '{\\tt\\sc Prime + Abort} (full coloring)', 'Greens_r'),

        ('xp-no-colour.txt', '{\\tt\\sc Xlate + Probe} (no coloring)', 'Greens'),
        ('xp-pg-colour.txt', '{\\tt\\sc Xlate + Probe} (page coloring)', 'Greens'),
        ('xp-pt-colour.txt', '{\\tt\\sc Xlate + Probe} (full coloring)', 'Greens'),

        ('xa-no-colour.txt', '{\\tt\\sc Xlate + Abort} (no coloring)', 'Greens_r'),
        ('xa-pg-colour.txt', '{\\tt\\sc Xlate + Abort} (page coloring)', 'Greens_r'),
        ('xa-pt-colour.txt', '{\\tt\\sc Xlate + Abort} (full coloring)', 'Greens_r'),
    ]

    fig = plt.figure(figsize=(10, 12))

    axes = []

    for (name, title, cmap), coord in zip(names, np.ndindex((4, 3))):
        ax = plt.subplot2grid((4, 3), coord)
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
