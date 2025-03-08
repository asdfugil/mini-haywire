# SPDX-License-Identifier: MIT
import os, struct, sys, time

from .proxy import *
from .proxyutils import *
from .tgtypes import *
from .utils import *

# Create serial connection
iface = UartInterface()
# Construct m1n1 proxy layer over serial connection
p = M1N1Proxy(iface, debug=False)
# Customise parameters of proxy and serial port
# based on information sent over the connection
bootstrap_port(iface, p)

# Initialise the Proxy interface from values fetched from
# the remote end
u = ProxyUtils(p)
# Build a Register Monitoring object on Proxy Interface
mon = RegMonitor(u)

fb = u.ba.video.base

print(f"mini base: 0x{u.base:x}")
