# mini-haywire

A port of m1n1 to Lightning VGA adapter and Lightning Digital AV Adapter.
It expose an UART proxy interface that is mostly compatible with m1n1,
though some features are not supported due to them not applying on this
hardware.

## Building

You will need the llvm toolchain with clang and lld. You will also need
libgcc for arm-none-abi for division routines. Additionally, to wrap
the result up into an img3, you will also need [oldimgtool](https://github.com/justtryingthingsout/oldimgtool)

```
$ git clone --recursive https://github.com/asdfugil/mini-haywire
$ cd mini-haywire
$ make
```

To build on macOS, you will need to install clang and lld then libgcc
for arm-none-eabi:

```
brew install llvm lld gcc-arm-embedded
```

Some linux distributions like Fedora do not provide libgcc in their cross
toolchains, in this case you can obtain a copy from [Arm GNU Toolchain Downloads](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads).

## Power and USB Connection Setup

To connect to a device without a lightning port, you will need two female lightning breakout
boards, note that the Apple Pencil female-to-female lightning charging adapter do not work
as the dongles are powered through the ID1 pin which is not the case when that adapter is used.

The breakout with the adapter will be referred to as the `device` board, and the breakout
with a lightning cable will be referred as the `host` board.

Connect:

- Device PWR -> Host PWR
- Device L0n -> Host L0n
- Device L0p -> Host L0p
- Device L1p -> Host L1n
- Device L1n -> Host L1p

Then, you will need a external power supply of 3.3V - 5V. A Pi Pico will work fine.

- Power supply 3.3V-5V (Pico 3V3) -> Device ID1

For grounding, the ground of everything should be connected together, this includes:

- Power supply GND
- Device GND
- Host GND

After turning on the power supply, the haywire can be accessed as a USB device in DFU
mode from the host breakout board using a standard lightning cable. Serial cables like
the DCSD cable will also be able to get serial output. Do note that the serial output
is from uart2, which is *not* what iBSS uses.

Alternatively, the L1n and L1p pins carries the serial signal without negotitation, so
instead of connecting to the host board, you can connect them to something like a
CH340 serial adapter:

- Device L1p -> CH340 TXD
- Device L1n -> CH340 RXD
- Common Ground -> CH340 GND

## Payloads

Unlike m1n1, mini-haywire **does not** accept appended payload. Instead, they should be
loaded as a ramdisk in iBSS. If you include a kernel, then it must be in form of a zImage
and appended last due to the lack of information in the file.

```
echo "chosen.bootargs=earlycon console=ttySAC2" > variable.txt
cat variable.txt s5l8747-b137.dtb s5l8747-b165.dtb initramfs.gz zImage > payload.bin
oldimgtool -m IMG3 -T rdsk payload.bin payload.img3
```

## License

mini-haywire is licensed under the MIT license, as included in the LICENSE file.

Large parts of mini-haywire is based on m1n1, which is licensed under the MIT license.

    Copyright The Asahi Linux Contributors

Despite the name, mini-haywire is not based on the original mini,
its licensing does not directly apply here.

However, portions of m1n1 are from mini, though those have been relicensed
under MIT license in m1n1.

Portions of mini-haywire are based on mini (in m1n1):

    Copyright (C) 2008-2010 Hector Martin "marcan" <marcan@marcan.st>
    Copyright (C) 2008-2010 Sven Peter <sven@svenpeter.dev>
    Copyright (C) 2008-2010 Andre Heider <a.heider@gmail.com>

mini-haywire embeds libfdt, which is dual BSD and GPL-2 licensed and copyright:

    Copyright (C) 2014 David Gibson david@gibson.dropbear.id.au
    Copyright (C) 2018 embedded brains GmbH
    Copyright (C) 2006-2012 David Gibson, IBM Corporation.
    Copyright (C) 2012 David Gibson, IBM Corporation.
    Copyright 2012 Kim Phillips, Freescale Semiconductor.
    Copyright (C) 2016 Free Electrons
    Copyright (C) 2016 NextThing Co.

The ADT code in mini-haywire is also based on libfdt and subject to the same license.

mini-haywire embeds minlzma, which is MIT licensed and copyright:

    Copyright (c) 2020 Alex Ionescu

mini-haywire embeds a slightly modified version of tinf, which is ZLIB licensed and copyright:

    Copyright (c) 2003-2019 Joergen Ibsen

mini-haywire embeds portions taken from arm-trusted-firmware, which is BSD licensed and copyright:

    Copyright (c) 2013-2020, ARM Limited and Contributors. All rights reserved.

mini-haywire embeds Doug Lea's malloc (dlmalloc), which is in the public domain (CC0).

mini-haywire embeds portions of PDCLib, which is in the public domain (CC0).

