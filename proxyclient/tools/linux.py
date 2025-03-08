#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
import sys, pathlib
from m1n1.setup import *
import serial
import struct
sys.path.append(str(pathlib.Path(__file__).resolve().parents[1]))

import argparse, pathlib

def swap(i):
    return struct.unpack("<I", struct.pack(">I", i))[0]

parser = argparse.ArgumentParser(description='(Linux) kernel loader for m1n1')
parser.add_argument('payload', type=pathlib.Path)
parser.add_argument('dtb', type=pathlib.Path)
parser.add_argument('initramfs', nargs='?', type=pathlib.Path)
parser.add_argument('--compression', choices=['auto', 'none', 'gz', 'xz'], default='auto')
parser.add_argument('-b', '--bootargs', type=str, metavar='"boot arguments"')
parser.add_argument('-t', '--tty', type=str)
parser.add_argument('-r', '--retrive', type=pathlib.Path)
parser.add_argument('-u', '--u-boot', type=pathlib.Path, help="load u-boot before linux")
args = parser.parse_args()

p.free(p.malloc(4)) # ???

payload = args.payload.read_bytes()
dtb = args.dtb.read_bytes()
if args.initramfs is not None:
    initramfs = args.initramfs.read_bytes()
    initramfs_size = len(initramfs)
else:
    initramfs = None
    initramfs_size = 0

if args.bootargs is not None:
    print('Setting boot args: "{}"'.format(args.bootargs))
    p.kboot_set_chosen("bootargs", args.bootargs)

dtb_addr = p.top_of_memory_alloc(len(dtb))
print("Loading DTB to 0x%x..." % dtb_addr)

iface.writemem(dtb_addr, dtb)

if initramfs is not None:
    initramfs_base = p.memalign(65536, initramfs_size)
    print("Loading %d initramfs bytes to 0x%x..." % (initramfs_size, initramfs_base))
    iface.writemem(initramfs_base, initramfs, True)
    p.kboot_set_initrd(initramfs_base, initramfs_size)

if p.kboot_prepare_dt(dtb_addr):
    print("DT prepare failed")
    sys.exit(1)

packed_dtb = p.kboot_get_dt()
fdt_totalsize = swap(p.read32(packed_dtb + 0x4))
print("total size", fdt_totalsize)
dtb_process = iface.readmem(packed_dtb, fdt_totalsize)
args.retrive.write_bytes(dtb_process)

iface.dev.timeout = 40

kernel_size = len(payload)

kernel_base = p.memalign(2 * 1024 * 1024, kernel_size)
boot_addr = kernel_base
print("Kernel_base: 0x%x" % kernel_base)

print("Loading %d bytes to 0x%x..0x%x..." % (kernel_size, kernel_base, kernel_base + kernel_size))
iface.writemem(kernel_base, payload, True)

print(kernel_size)

if kernel_size < 0:
    raise Exception("Decompression error!")

print("Decompress OK...")

print("Ready to boot")

p.kboot_boot(boot_addr)
iface.ttymode()
