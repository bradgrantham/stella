#!/usr/bin/env python3

import sys

line1 = sys.stdin.readline()
line2 = sys.stdin.readline()
line3 = sys.stdin.readline()

colors = 256 * [0, 0, 0]
print(len(colors))

for row in range(8):
    line = sys.stdin.readline()
    # print("%s" % line)
    values = line.strip().split()
    for column in range(16):
        index = row * 2 + column * 16
        colors[index + 0] = values[column*3:column*3+3]
        colors[index + 1] = values[column*3:column*3+3]
        # print("%d, %s" % (index, values[column*3:column*3+3]))
        # print("%d, %s" % (index, colors[index]))

index = 0
print("uint8_t colu_to_rgb[256][3] = {")
for color in colors[0:256]:
    rgb = [int(c) for c in color]
    print("    {0x%02X, 0x%02X, 0x%02X}," % (rgb[0], rgb[1], rgb[2]))
    index += 1
print("};")
