// boot 核心逻辑

#include <stdio.h>
#include "bootpack.h"

void HariMain(void)
{
    struct BOOTINFO *bootinfo = (struct BOOTINFO *)ADR_BOOTINFO;
    char s[40], mcursor[256];
    int mx = (bootinfo->scrnx - 16) / 2;
    int my = (bootinfo->scrny - 28 - 16) / 2;

    init_gdtidt();
    init_pic();
    io_sti();

    init_palette();
    init_screen(bootinfo->vram, bootinfo->scrnx, bootinfo->scrny);

    putfont_asc(bootinfo->vram, bootinfo->scrnx, 9, 17, COL8_000000, "Momoyeyu OS");
    putfont_asc(bootinfo->vram, bootinfo->scrnx, 8, 16, COL8_FFFFFF, "Momoyeyu OS");

    sprintf(s, "scrnx = %d", bootinfo->scrnx);
    putfont_asc(bootinfo->vram, bootinfo->scrnx, 16, 64, COL8_FFFFFF, s);

    init_cursor(mcursor, COL8_008484);
    putblock8(bootinfo->vram, bootinfo->scrnx, mx, my, mcursor, 16, 16);

    io_out8(PIC0_IMR, 0xf9); /* 允许PIC1和键盘中断（11111001） */
    io_out8(PIC1_IMR, 0xef); /* 允许鼠标中断（11101111） */

    // 无限循环，等待硬件中断
    for (;;)
    {
        io_hlt();
    }
}
