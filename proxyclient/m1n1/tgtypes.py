# SPDX-License-Identifier: MIT
from construct import *

__all__ = ["BootArgs_r1"]

BootArgs_r1 = Struct(
    "revision"              / Hex(Int16ul),
    "version"               / Hex(Int16ul),
    "virt_base"             / Hex(Int32ul),
    "phys_base"             / Hex(Int32ul),
    "mem_size"              / Hex(Int32ul),
    "top_of_kernel_data"    / Hex(Int32ul),
    "video" / Struct(
        "base"      / Hex(Int32ul),
        "display"   / Hex(Int32ul),
        "stride"    / Hex(Int32ul),
        "width"     / Hex(Int32ul),
        "height"    / Hex(Int32ul),
        "depth"     / Hex(Int32ul),
    ),
    "machine_type"          / Hex(Int32ul),
    "devtree"               / Hex(Int32ul),
    "devtree_size"          / Hex(Int32ul),
    "cmdline"               / PaddedString(256, "ascii"),
    "boot_flags"            / Hex(Int32ul),
)
