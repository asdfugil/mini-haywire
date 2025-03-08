/* SPDX-License-Identifier: MIT */

#include "usb.h"
#include "adt.h"
#include "clkrstgen.h"
#include "iodev.h"
#include "malloc.h"
#include "string.h"
#include "types.h"
#include "usb_dwc2.h"
#include "usbphy.h"
#include "utils.h"
#include "vsprintf.h"

bool usb_is_initialized = false;

#if USB_IODEV_COUNT > 100
#error "USB_IODEV_COUNT is limited to 100 to prevent overflow in ADT path names"
#endif

#define USB_IODEV_WRAPPER(driver, name, pipe)                                                      \
    static ssize_t usb_##driver##_##name##_can_read(void *dev)                                     \
    {                                                                                              \
        return usb_##driver##_can_read(dev, pipe);                                                 \
    }                                                                                              \
                                                                                                   \
    static bool usb_##driver##_##name##_can_write(void *dev)                                       \
    {                                                                                              \
        return usb_##driver##_can_write(dev, pipe);                                                \
    }                                                                                              \
                                                                                                   \
    static ssize_t usb_##driver##_##name##_read(void *dev, void *buf, size_t count)                \
    {                                                                                              \
        return usb_##driver##_read(dev, pipe, buf, count);                                         \
    }                                                                                              \
                                                                                                   \
    static ssize_t usb_##driver##_##name##_write(void *dev, const void *buf, size_t count)         \
    {                                                                                              \
        return usb_##driver##_write(dev, pipe, buf, count);                                        \
    }                                                                                              \
                                                                                                   \
    static ssize_t usb_##driver##_##name##_queue(void *dev, const void *buf, size_t count)         \
    {                                                                                              \
        return usb_##driver##_queue(dev, pipe, buf, count);                                        \
    }                                                                                              \
                                                                                                   \
    static void usb_##driver##_##name##_handle_events(void *dev)                                   \
    {                                                                                              \
        usb_##driver##_handle_events(dev);                                                         \
    }                                                                                              \
                                                                                                   \
    static void usb_##driver##_##name##_flush(void *dev)                                           \
    {                                                                                              \
        usb_##driver##_flush(dev, pipe);                                                           \
    }

USB_IODEV_WRAPPER(dwc2, 0, CDC_ACM_PIPE_0)
USB_IODEV_WRAPPER(dwc2, 1, CDC_ACM_PIPE_1)

#define USB_IODEV_OPS(driver, name, pipe)                                                          \
    {                                                                                              \
        .can_read = usb_##driver##_##name##_can_read,                                              \
        .can_write = usb_##driver##_##name##_can_write,                                            \
        .read = usb_##driver##_##name##_read,                                                      \
        .write = usb_##driver##_##name##_write,                                                    \
        .queue = usb_##driver##_##name##_queue,                                                    \
        .flush = usb_##driver##_##name##_flush,                                                    \
        .handle_events = usb_##driver##_##name##_handle_events,                                    \
    }

static struct iodev_ops iodev_usb_dwc2_ops = USB_IODEV_OPS(dwc2, 0, CDC_ACM_PIPE_0);
static struct iodev_ops iodev_usb_dwc2_sec_ops = USB_IODEV_OPS(dwc2, 1, CDC_ACM_PIPE_1);

struct iodev iodev_usb_vuart = {
    .usage = 0,
    .lock = SPINLOCK_INIT,
};

// cursed one-and-and-half usb phy register accesses

static inline u32 usbphy_write32_internal(uintptr_t addr, u8 phy, u8 shift, u32 value)
{
    u32 phy_mask = MASK(shift) << (shift * phy);
    return mask32(addr, phy_mask, value << (shift * phy));
}

static inline u32 usbphy_mask32_internal(uintptr_t addr, u8 phy, u8 shift, u32 clear, u32 set)
{
    clear = clear << (shift * phy);
    return mask32(addr, clear, set << (shift * phy));
}

__attribute__((unused)) static inline u32 usbphy_read32_internal(uintptr_t addr, u8 phy, u8 shift)
{
    UNUSED(phy);
    return (read32(addr) >> shift) & MASK(shift);
}

#define usbphy_write32(base, reg, phy, value)                                                      \
    usbphy_write32_internal(base + reg, phy, reg##_SHIFT, value)

#define usbphy_mask32(base, reg, phy, set, clear)                                                  \
    usbphy_mask32_internal(base + reg, phy, reg##_SHIFT, set, clear)

#define usbphy_read32(base, reg, phy, value) usbphy_read32_internal(base + reg, phy, reg##_SHIFT)

#define usbphy_clear32(base, reg, phy, clear)                                                      \
    usbphy_mask32_internal(base + reg, phy, reg##_SHIFT, clear, 0)

#define usbphy_set32(base, reg, phy, set)                                                          \
    usbphy_mask32_internal(base + reg, phy, reg##_SHIFT, set, set)

#define FMT_DWC2_PATH   "/arm-io/usb-complex%d/usb-device%d"
#define FMT_USBPHY_PATH "/arm-io/otgphyctrl%d"

int usbphy_init(u32 idx)
{
    char usbphy_adt_path[sizeof(FMT_USBPHY_PATH)];

    snprintf(usbphy_adt_path, sizeof(usbphy_adt_path), FMT_USBPHY_PATH, idx);

    int otgphyctrl_path[8];
    uintptr_t usbphy_base = 0;

    int otgctl_offset = adt_path_offset_trace(adt, usbphy_adt_path, otgphyctrl_path);

    if (otgctl_offset < 0)
        return -1;

    if (adt_get_reg(adt, otgphyctrl_path, "reg", 0, &usbphy_base, NULL) < 0) {
        printf("USB%d: failed to get %s reg\n", idx, usbphy_adt_path);
        return -1;
    }

    u32 cfg0, cfg1;
    if (ADT_GETPROP(adt, otgctl_offset, "uotgtune1-device", &cfg0) < 0) {
        printf("USB%d: Error getting CFG0 from %s\n", idx, usbphy_adt_path);
        return -1;
    }
    if (ADT_GETPROP(adt, otgctl_offset, "uotgtune2-device", &cfg1) < 0) {
        printf("USB%d: Error getting CFG1 from %s\n", idx, usbphy_adt_path);
        return -1;
    }

    set32(usbphy_base + USBPHY_COJCON, USBPHY_COJCON_PHY0_PWR_ON << idx);

    udelay(100);

    usbphy_write32(usbphy_base, USBPHY_PWRCON, idx,
                   FIELD_PREP(USBPHY_PWRCON_CLKSEL, USBPHY_PWRCON_REFCLK));
    usbphy_write32(usbphy_base, USBPHY_PULCON, idx, BIT(2));
    usbphy_write32(usbphy_base, USBPHY_CFG0, idx, cfg0);
    usbphy_write32(usbphy_base, USBPHY_CFG1, idx, cfg1);

    udelay(10);

    usbphy_mask32(usbphy_base, USBPHY_CLKCON, idx, USBPHY_CLKCON_CLKSEL, USBPHY_CLKCON_REFCLK);
    usbphy_set32(usbphy_base, USBPHY_RSTCON, idx, USBPHY_RSTCON_RESET);

    udelay(20);

    usbphy_clear32(usbphy_base, USBPHY_RSTCON, idx, USBPHY_RSTCON_RESET);

    udelay(1000);

    usbphy_set32(usbphy_base, USBPHY_PULCON, idx, BIT(1));

    return 0;
}

dwc2_dev_t *usb_iodev_bringup(u32 idx)
{
    char dwc2_adt_path[sizeof(FMT_DWC2_PATH)];
    snprintf(dwc2_adt_path, sizeof(dwc2_adt_path), FMT_DWC2_PATH, idx, idx);

    int dwc2Path[8];
    uintptr_t DWC2Base = 0;

    if (adt_path_offset_trace(adt, dwc2_adt_path, dwc2Path) < 0)
        return NULL;

    if (adt_get_reg(adt, dwc2Path, "reg", 0, &DWC2Base, NULL) < 0) {
        printf("USB%d: failed to get %s reg\n", idx, dwc2_adt_path);
        return NULL;
    }

    return usb_dwc2_init(DWC2Base);
}

void usb_init(void)
{
    if (usb_is_initialized)
        return;

    if (adt_path_offset(adt, "/arm-io/otgphyctrl1") > 0 &&
        adt_path_offset(adt, "/arm-io/usb-complex1") > 0) {
        usbphy_init(0);
        usbphy_init(1);
    }

    usb_is_initialized = true;
}

void usb_iodev_init(void)
{
    for (int i = 0; i < USB_IODEV_COUNT; i++) {
        dwc2_dev_t *opaque;
        struct iodev *usb_iodev;

        opaque = usb_iodev_bringup(i);
        if (!opaque)
            continue;

        usb_iodev = memalign(SPINLOCK_ALIGN, sizeof(*usb_iodev));
        if (!usb_iodev)
            continue;

        usb_iodev->ops = &iodev_usb_dwc2_ops;
        usb_iodev->opaque = opaque;
        usb_iodev->usage = USAGE_CONSOLE | USAGE_UARTPROXY;
        spin_init(&usb_iodev->lock);

        iodev_register_device(IODEV_USB0 + i, usb_iodev);
        printf("USB%d: initialized at %p\n", i, opaque);
    }
}

void usb_iodev_shutdown(void)
{
    for (int i = 0; i < USB_IODEV_COUNT; i++) {
        struct iodev *usb_iodev = iodev_unregister_device(IODEV_USB0 + i);
        if (!usb_iodev)
            continue;

        printf("USB%d: shutdown\n", i);
        usb_dwc2_shutdown(usb_iodev->opaque);
        free(usb_iodev);
    }
}

void usb_iodev_vuart_setup(iodev_id_t iodev)
{
    if (iodev < IODEV_USB0 || iodev >= IODEV_USB0 + USB_IODEV_COUNT)
        return;

    iodev_usb_vuart.ops = &iodev_usb_dwc2_sec_ops;
    iodev_usb_vuart.opaque = iodev_get_opaque(iodev);
}
