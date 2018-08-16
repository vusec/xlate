#!/usr/bin/env python3

import numpy as np
from matplotlib import rc
import matplotlib.colors
import matplotlib.pyplot as plt

def pastel(colour, weight=2.4):
    """ Convert colour into a nice pastel shade"""
    converter = matplotlib.colors.ColorConverter()

    rgb = np.asarray(converter.to_rgba(colour)[:3])
    # scale colour
    maxc = max(rgb)
    if maxc < 1.0 and maxc > 0:
        # scale colour
        scale = 1.0 / maxc
        rgb = rgb * scale
    # now decrease saturation
    total = rgb.sum()
    slack = 0
    for x in rgb:
        slack += 1.0 - x

    # want to increase weight from total to weight
    # pick x s.t.  slack * x == weight - total
    # x = (weight - total) / slack
    x = (weight - total) / slack

    rgb = [c + (x * (1.0 - c)) for c in rgb]

    return rgb

def darken(colour, value):
    converter = matplotlib.colors.ColorConverter()

    rgb = np.asarray(converter.to_rgba(colour)[:3])
    h, s, v = matplotlib.colors.rgb_to_hsv(rgb)
    v = max(v - value, 0)

    return matplotlib.colors.hsv_to_rgb((h, s, v))

bandwidth = np.array([
    np.average([45858.54, 41084.56, 39693.97]),
    np.average([200.24, 624.76, 964.57]),
    np.average([8539.85, 7550.78, 7802.75]),
    np.average([867.23, 536.36, 645.51]),

    np.average([47509.35, 42111.45, 40685.44]),
    np.average([5495.75, 5172.52, 5242.63]),
    np.average([7981.35, 7344.10, 7718.37]),
    np.average([440.70, 500.37, 537.79]),
])

errors = np.array([
    np.average([4.77, 3.72, 3.48]),
    np.average([200.24, 2180.17, 1810.00]),
    np.average([448.78, 615.85, 495.74]),
    np.average([67.34, 71.18, 95.95]),

    np.average([4.26, 3.61, 3.03]),
    np.average([70.41, 64.65, 72.59]),
    np.average([537.92, 570.90, 495.45]),
    np.average([68.09, 66.40, 80.81]),
])

bit_errors = np.array([
    np.average([14.13, 10.07, 11.02]),
    np.average([4358.04, 3847.03, 3103.62]),
    np.average([923.89, 1206.02, 888.92]),
    np.average([123.24, 120.91, 178.46]),

    np.average([12.42, 10.56, 8.46]),
    np.average([134.96, 124.27, 137.55]),
    np.average([978.35, 1097.36, 882.23]),
    np.average([136.80, 132.66, 161.32]),
])

labels = ['\\tt\\sc Flush + Reload', '\\tt\\sc Flush + Flush', '\\tt\\sc Prime + Probe', '\\tt\\sc Xlate + Probe']
indices = np.arange(len(labels))

rc('text', usetex=True)

plt.subplot(121)

bar1 = plt.bar(indices + .25, bandwidth[:4], .2, color=pastel('orange'))
bar2 = plt.bar(indices + .25, errors[:4], .2, bottom=bandwidth[:4], color=darken(pastel('orange'), .4))

bar3 = plt.bar(indices + .55, bandwidth[4:], .2, color=pastel('green'))
bar4 = plt.bar(indices + .55, errors[4:], .2, bottom=bandwidth[4:], color=darken(pastel('green'), .4))

plt.xticks(indices + .5, labels, rotation=45)
plt.ylabel('Bandwidth (bytes/sec)')
plt.yscale('log')
plt.figlegend(
    (bar1[0], bar2[0], bar3[0], bar4[0]),
    ('Cross-Thread (correct)', 'Cross-Thread (raw)', 'Cross-Core (correct)', 'Cross-Core (raw)'),
    loc='upper right',
    prop={'size': 11},
)

plt.subplot(122)

bar1 = plt.bar(indices + .25, bit_errors[:4], .2, color=pastel('orange'))
bar2 = plt.bar(indices + .55, bit_errors[4:], .2, color=pastel('green'))

plt.xticks(indices + .5, labels, rotation=45)
plt.ylabel('Bit errors (bits/sec)')
plt.yscale('log')

plt.tight_layout()
plt.savefig('comparison.pdf')
