/* SPDX-License-Identifier: MIT */

#ifndef USB_H
#define USB_H

#include "iodev.h"
#include "types.h"

void usb_init(void);
void usb_iodev_init(void);
void usb_iodev_shutdown(void);
void usb_iodev_vuart_setup(iodev_id_t iodev);

#endif
