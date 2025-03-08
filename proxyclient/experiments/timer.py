#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
import sys, pathlib
import datetime
sys.path.append(str(pathlib.Path(__file__).resolve().parents[1]))

from m1n1.setup import *

freq = 24000000
cnth = 0x3c700080
cntl = 0x3c700084
ctl = 0x3c700088

timer_enable = 1 << 3

def get_ticks():
    return (p.read32(cnth) << 32 | p.read32(cntl))

def udelay(usec):
    ticks = get_ticks()
    ticks += (freq * usec) / 1000000
    while (get_ticks() < ticks):
        continue
    return

# enable timer
p.write32(ctl, timer_enable)

start = datetime.datetime.now()

udelay(10 * 1000000)

end = datetime.datetime.now()

elpased = end.timestamp() - start.timestamp()
print(elpased)
