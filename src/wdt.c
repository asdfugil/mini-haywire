#include "wdt.h"
#include "adt.h"
#include "utils.h"

#define WDT_CTL       0x0
#define WDT_CTL_RESET BIT(20)
#define WDT_COUNT     0x4

static u32 wdt_base = 0;

void wdt_disable(void)
{
    int path[8];
    int node = adt_path_offset_trace(adt, "/arm-io/wdt", path);

    if (node < 0) {
        printf("WDT node not found!\n");
        return;
    }

    if (adt_get_reg(adt, path, "reg", 0, &wdt_base, NULL)) {
        printf("Failed to get WDT reg property!\n");
        return;
    }

    printf("WDT registers @ 0x%x\n", wdt_base);

    write32(wdt_base + WDT_CTL, 0);

    printf("WDT disabled\n");
}

void wdt_reboot(void)
{
    if (!wdt_base)
        return;

    write32(wdt_base + WDT_CTL, WDT_CTL_RESET);
}
