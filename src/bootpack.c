// boot 核心逻辑

#include <stdio.h>
#include "bootpack.h"

void HariMain(void)
{
    struct BOOTINFO *bootinfo = (struct BOOTINFO *)0x0ff0;
    char s[40], mcursor[256];
    int mx = (bootinfo->scrnx - 16) / 2;
    int my = (bootinfo->scrny - 28 - 16) / 2;

    init_gdtidt();
    init_palette();
    init_screen(bootinfo->vram, bootinfo->scrnx, bootinfo->scrny);

    putfont_asc(bootinfo->vram, bootinfo->scrnx, 9, 17, COL8_000000, "Momoyeyu OS");
    putfont_asc(bootinfo->vram, bootinfo->scrnx, 8, 16, COL8_FFFFFF, "Momoyeyu OS");

    sprintf(s, "scrnx = %d", bootinfo->scrnx);
    putfont_asc(bootinfo->vram, bootinfo->scrnx, 16, 64, COL8_FFFFFF, s);

    init_cursor(mcursor, COL8_008484);
    putblock8(bootinfo->vram, bootinfo->scrnx, mx, my, mcursor, 16, 16);

    // 无限循环，等待硬件中断
    for (;;)
    {
        io_hlt();
    }
}
