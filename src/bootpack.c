// boot 核心逻辑

#include <stdio.h>
#include "bootpack.h"

extern struct FIFO8 keyfifo, mousefifo; // keyboard.c mouse.c
unsigned int memtest(unsigned int start, unsigned int end);

void HariMain(void)
{
    struct BOOTINFO *bootinfo = (struct BOOTINFO *)ADR_BOOTINFO;
    char s[40], mcursor[256], keybuf[32], mousebuf[128];
    int mx = (bootinfo->scrnx - 16) / 2;
    int my = (bootinfo->scrny - 28 - 16) / 2;
    int i;

    // initialize GDT & IDT
    init_gdtidt();
    init_pic();
    io_sti(); // 初始化完 GDT IDT PIC 之后才开中断

    // initialize devices
    io_out8(PIC0_IMR, 0xf9); /* 允许PIC1和键盘中断（11111001） */
    io_out8(PIC1_IMR, 0xef); /* 允许鼠标中断（11101111） */
    fifo8_init(&keyfifo, 32, keybuf);
    fifo8_init(&mousefifo, 128, mousebuf);
    init_keyboard();

    // say hello
    init_palette();
    init_screen(bootinfo->vram, bootinfo->scrnx, bootinfo->scrny);
    putfont_asc(bootinfo->vram, bootinfo->scrnx, 97, 97, COL8_000000, "Momoyeyu OS");
    putfont_asc(bootinfo->vram, bootinfo->scrnx, 96, 96, COL8_FFFFFF, "Momoyeyu OS");

    // test memory
    i = memtest(0x00400000, 0xbfffffff) >> 20; // 1MB = 1024*1024B = 2^20B
    sprintf(s, "memory %dMB", i);
    putfont_asc(bootinfo->vram, bootinfo->scrnx, 0, 32, COL8_FFFFFF, s);

    // draw cursor
    init_cursor(mcursor, COL8_008484);
    putblock8(bootinfo->vram, bootinfo->scrnx, mx, my, mcursor, 16, 16);
    struct MOUSE_DEC mdec;
    enable_mouse(&mdec);

    // 无限循环，等待硬件中断
    for (;;)
    {
        io_cli();
        if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo))
        {
            if (fifo8_status(&keyfifo))
            {
                i = fifo8_get(&keyfifo);
                io_sti();
                sprintf(s, "%02X", i);
                boxfill8(bootinfo->vram, bootinfo->scrnx, COL8_008484, 8, 16, 23, 31);
                putfont_asc(bootinfo->vram, bootinfo->scrnx, 8, 16, COL8_FFFFFF, s);
            }
            else if (fifo8_status(&mousefifo))
            {
                i = fifo8_get(&mousefifo);
                io_sti();
                if (mouse_decode(&mdec, i))
                {
                    sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
                    if ((mdec.btn & 0x01) != 0)
                        s[1] = 'L';
                    if ((mdec.btn & 0x02) != 0)
                        s[3] = 'R';
                    if ((mdec.btn & 0x04) != 0)
                        s[2] = 'C';
                    boxfill8(bootinfo->vram, bootinfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
                    putfont_asc(bootinfo->vram, bootinfo->scrnx, 32, 16, COL8_FFFFFF, s);

                    boxfill8(bootinfo->vram, bootinfo->scrnx, COL8_008484, mx, my, mx + 15, my + 15); /* 隐藏鼠标 */
                    mx += mdec.x;
                    my += mdec.y;
                    if (mx < 0)
                        mx = 0;
                    else if (mx > bootinfo->scrnx - 16)
                        mx = bootinfo->scrnx - 16;
                    if (my < 0)
                        my = 0;
                    else if (my > bootinfo->scrny - 16)
                        my = bootinfo->scrny - 16;
                    sprintf(s, "(%3d, %3d)", mx, my);
                    boxfill8(bootinfo->vram, bootinfo->scrnx, COL8_008484, 0, 0, 79, 15); /* 隐藏旧坐标 */
                    putfont_asc(bootinfo->vram, bootinfo->scrnx, 0, 0, COL8_FFFFFF, s);   /* 显示新坐标 */
                    putblock8(bootinfo->vram, bootinfo->scrnx, mx, my, mcursor, 16, 16);
                }
            }
        }
        else
        {
            io_stihlt();
        }
    }
}

#define EFLAGS_AC_BIT 0x00040000
#define CR0_CACHE_DISABLE 0x60000000

unsigned int memtest(unsigned int start, unsigned int end)
{
    char flg486 = 0;
    unsigned int eflg, cr0, i;

    // 确认是否支持缓存（是386还是486）
    eflg = io_load_eflags();
    eflg |= EFLAGS_AC_BIT; /* AC-bit = 1 */
    io_store_eflags(eflg);
    eflg = io_load_eflags();
    /* 如果是386，AC-bit会自动变回0 */
    if ((eflg & EFLAGS_AC_BIT) != 0)
    {
        flg486 = 1;
    }
    eflg &= ~EFLAGS_AC_BIT;
    io_store_eflags(eflg); /* AC-bit = 0 */

    if (flg486 != 0)
    {
        cr0 = load_cr0();
        cr0 |= CR0_CACHE_DISABLE; /* 禁止缓存 */
        store_cr0(cr0);
    }
    i = memtest_sub(start, end);
    if (flg486 != 0)
    {
        cr0 = load_cr0();
        cr0 &= ~CR0_CACHE_DISABLE; /* 允许缓存 */
        store_cr0(cr0);
    }
    return i;
}
