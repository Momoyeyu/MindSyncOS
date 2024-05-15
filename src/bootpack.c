// boot 核心逻辑

#include <stdio.h>
#include "bootpack.h"

extern struct FIFO8 keyfifo, mousefifo; // keyboard.c mouse.c

void HariMain(void)
{
    struct BOOTINFO *bootinfo = (struct BOOTINFO *)ADR_BOOTINFO;
    char s[40], keybuf[32], mousebuf[128];
    int mx = (bootinfo->scrnx - 16) / 2;
    int my = (bootinfo->scrny - 28 - 16) / 2;
    unsigned int memtotal;
    struct MouseDescriptor mdec;                                        // mouse
    struct MemoryManager *memman = (struct MemoryManager *)MEMMAN_ADDR; // memory manager
    struct SheetController *shtctl;                                     // sheet ctl
    struct Sheet *sht_back, *sht_mouse;                                 // background, mouse
    unsigned char *buf_back, buf_mouse[256];

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
    enable_mouse(&mdec);

    // init memory management
    memtotal = mem_test(0x00400000, 0xbfffffff);
    memman_init(memman);
    memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 - 0x0009efff */
    memman_free(memman, 0x00400000, memtotal - 0x00400000);
    // alloc memory for background and mouse
    shtctl = shtctl_init(memman, bootinfo->vram, bootinfo->scrnx, bootinfo->scrny);
    sht_back = sheet_alloc(shtctl);  // background
    sht_mouse = sheet_alloc(shtctl); // mouse
    buf_back = (unsigned char *)memman_alloc_4k(memman, bootinfo->scrnx * bootinfo->scrny);

    // ---------------------------------- 显示部分 ----------------------------------
    init_palette();

    // init screen
    sheet_set_buf(sht_back, buf_back, bootinfo->scrnx, bootinfo->scrny, -1); /* 没有透明色 */
    init_screen(buf_back, bootinfo->scrnx, bootinfo->scrny);
    sheet_slide(shtctl, sht_back, 0, 0);
    sheet_set_layer(shtctl, sht_back, 0);

    // init cursor
    sheet_set_buf(sht_mouse, buf_mouse, 16, 16, 99); /* 透明色号99 */
    init_cursor(buf_mouse, 99);                      /* 透明色号99 */
    sheet_slide(shtctl, sht_mouse, mx, my);
    sheet_set_layer(shtctl, sht_mouse, 1);

    // debug
    sprintf(s, "(%3d, %3d)", mx, my);
    putfont_asc(buf_back, bootinfo->scrnx, 0, 0, COL8_FFFFFF, s);
    sprintf(s, "memory %dMB free : %dKB", memtotal >> 20, memman_available(memman) >> 10);
    putfont_asc(buf_back, bootinfo->scrnx, 0, 32, COL8_FFFFFF, s);

    sheet_refresh(shtctl);

    int i;
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
                boxfill8(buf_back, bootinfo->scrnx, COL8_008484, 8, 16, 23, 31);
                putfont_asc(buf_back, bootinfo->scrnx, 8, 16, COL8_FFFFFF, s);
                sheet_refresh(shtctl);
            }
            else if (fifo8_status(&mousefifo))
            {
                i = fifo8_get(&mousefifo);
                io_sti();
                if (mouse_decode(&mdec, i))
                {
                    sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
                    if ((mdec.button & 0x01) != 0)
                        s[1] = 'L';
                    if ((mdec.button & 0x02) != 0)
                        s[3] = 'R';
                    if ((mdec.button & 0x04) != 0)
                        s[2] = 'C';
                    boxfill8(buf_back, bootinfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
                    putfont_asc(buf_back, bootinfo->scrnx, 32, 16, COL8_FFFFFF, s);
                    // 移动鼠标
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
                    boxfill8(buf_back, bootinfo->scrnx, COL8_008484, 0, 0, 79, 15); /* 隐藏旧坐标 */
                    putfont_asc(buf_back, bootinfo->scrnx, 0, 0, COL8_FFFFFF, s);   /* 显示新坐标 */
                    sheet_slide(shtctl, sht_mouse, mx, my);                         /* 内含 sheet_refresh */
                }
            }
        }
        else
        {
            io_stihlt();
        }
    }
}
