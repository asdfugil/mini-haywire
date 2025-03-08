#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
import sys, pathlib
sys.path.append(str(pathlib.Path(__file__).resolve().parents[1]))

from m1n1 import adt
from m1n1.setup import *

dt = u.adt

clkrstgen = dt["/arm-io/clkrstgen"]

for dev in clkrstgen.device_clocks.clocks:
    print("Name", dev.name, "ID", dev.id)
    print("PS0", dev.ps0, "PS1", dev.ps1, "PS2", dev.ps2, "PS3", dev.ps3, "PS4", dev.ps4)
    if dev.unk1:
        print("Unknown1", dev.unk1)
    if dev.unk2:
        print("Unknown2", dev.unk2)
    if dev.unk3:
        print("Unknown3", dev.unk3)
    if dev.unk4:
        print("Unknown4", dev.unk4)