// boot 核心逻辑

#include <stdio.h>
#include "bootpack.h"

extern struct FIFO8 keyfifo;

void HariMain(void)
{
    struct BOOTINFO *bootinfo = (struct BOOTINFO *)ADR_BOOTINFO;
    char s[40], mcursor[256], keybuf[32];
    int mx = (bootinfo->scrnx - 16) / 2;
    int my = (bootinfo->scrny - 28 - 16) / 2;
    int i, j;
    fifo8_init(&keyfifo, 32, keybuf);

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
        io_cli();
        if (fifo8_status(&keyfifo))
        {
            i = fifo8_get(&keyfifo);
            io_sti();
            sprintf(s, "%02X", i);
            boxfill8(bootinfo->vram, bootinfo->scrnx, COL8_000000, 0, 16, 15, 31);
            putfont_asc(bootinfo->vram, bootinfo->scrnx, 0, 16, COL8_FFFFFF, s);
        }
        else
        {
            io_stihlt();
        }
    }
}
