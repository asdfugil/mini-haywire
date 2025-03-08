#ifndef USBPHY_H
#define USBPHY_H

#include "types.h"

#define USBPHY_PWRCON       0x00 // power
#define USBPHY_CLKCON       0x04 // clock
#define USBPHY_RSTCON       0x08 // reset
#define USBPHY_RSTCON_RESET BIT(0)

#define USBPHY_UNK14  0x14
#define USBPHY_PULCON 0x1c // pullup
#define USBPHY_CONDET 0x28
#define USBPHY_CFG0   0x40
#define USBPHY_CFG1   0x44
#define USBPHY_COJCON 0x100 // cursed one-and-a-half phy configuration

#define USBPHY_COJCON_PHY0_PWR_ON BIT(3)
#define USBPHY_COJCON_PHY1_PWR_ON BIT(4)

#define USBPHY_PWRCON_REFCLK 2
#define USBPHY_CLKCON_REFCLK 1

#define USBPHY_PWRCON_CLKSEL GENMASK(2, 1)
#define USBPHY_CLKCON_CLKSEL GENMASK(1, 0)

#define USBPHY_DWC2_CLKCON 0xe00

// bit shifts for second phy of one-and-a-half usb phy
// this is completely insane and inconsistent!
#define USBPHY_PWRCON_SHIFT 5
#define USBPHY_CLKCON_SHIFT 5
#define USBPHY_RSTCON_SHIFT 5
#define USBPHY_PULCON_SHIFT 9
#define USBPHY_CONDET_SHIFT 10
#define USBPHY_CFG0_SHIFT   11
#define USBPHY_CFG1_SHIFT   13
#endif
