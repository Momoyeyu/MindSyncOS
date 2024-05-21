// boot 核心逻辑

#include <stdio.h>
#include "bootpack.h"

void putfonts8_asc_sht(struct Sheet *sht, int bc, int c, int x0, int y0, char *s, int l);
void set490(struct FIFO32 *fifo, int mode);
void HariMain(void)
{
    struct BOOTINFO *bootinfo = (struct BOOTINFO *)ADR_BOOTINFO;
    struct FIFO32 fifo;
    char s[40];
    int fifobuf[128];
    struct TIMER *timer, *timer2, *timer3;
    int mx = (bootinfo->scrnx - 16) / 2;
    int my = (bootinfo->scrny - 28 - 16) / 2;
    unsigned int memtotal;
    struct MouseDescriptor mdec;                                        // mouse
    struct MemoryManager *memman = (struct MemoryManager *)MEMMAN_ADDR; // memory manager
    struct SheetController *sheet_controller;                           // sheet ctl
    struct Sheet *sht_back, *sht_mouse, *sht_win;
    unsigned char *buf_back, buf_mouse[256], *buf_win;

    // initialization
    init_gdtidt();
    init_pic();
    io_sti(); // 初始化完 GDT IDT PIC 之后才开中断
    init_pit();
    fifo32_init(&fifo, 128, fifobuf);

    // init devices
    init_keyboard(&fifo, 256);
    enable_mouse(&fifo, 512, &mdec);
    io_out8(PIC0_IMR, 0xf8); /* PIT和PIC1和键盘设置为许可(11111000) */
    io_out8(PIC1_IMR, 0xef); /* 允许鼠标中断（11101111） */

    timer = timer_alloc();
    timer_init(timer, &fifo, 10); // 10
    timer_settime(timer, 1000);

    timer2 = timer_alloc();
    timer_init(timer2, &fifo, 3); // 3
    timer_settime(timer2, 300);

    timer3 = timer_alloc();
    timer_init(timer3, &fifo, 1); // 1 & 0
    timer_settime(timer3, 50);

    set490(&fifo, 1);

    // init memory management
    memtotal = mem_test(0x00400000, 0xbfffffff);
    memman_init(memman);
    memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 - 0x0009efff */
    memman_free(memman, 0x00400000, memtotal - 0x00400000);

    // allocate memory
    sheet_controller = shtctl_init(memman, bootinfo->vram, bootinfo->scrnx, bootinfo->scrny);
    sht_back = sheet_alloc(sheet_controller);  // background
    sht_mouse = sheet_alloc(sheet_controller); // mouse
    sht_win = sheet_alloc(sheet_controller);   // window
    buf_back = (unsigned char *)memman_alloc_4k(memman, bootinfo->scrnx * bootinfo->scrny);
    buf_win = (unsigned char *)memman_alloc_4k(memman, 160 * 52);

    // ---------------------------------- 显示部分 ----------------------------------
    init_palette();

    // init screen
    sheet_set_buf(sht_back, buf_back, bootinfo->scrnx, bootinfo->scrny, -1); /* 没有透明色 */
    init_screen(buf_back, bootinfo->scrnx, bootinfo->scrny);
    sheet_slide(sht_back, 0, 0);

    // init cursor
    sheet_set_buf(sht_mouse, buf_mouse, 16, 16, 99); /* 透明色号99 */
    init_cursor(buf_mouse, 99);                      /* 透明色号99 */
    sheet_slide(sht_mouse, mx, my);

    // init window
    sheet_set_buf(sht_win, buf_win, 160, 52, -1);
    make_window8(buf_win, 160, 52, "counter");
    sheet_slide(sht_win, 80, 72);

    // set layer
    sheet_set_layer(sht_back, 0);
    sheet_set_layer(sht_win, 1);
    sheet_set_layer(sht_mouse, 2);

    // debug
    sprintf(s, "(%3d, %3d)", mx, my);
    putfont_asc(buf_back, bootinfo->scrnx, 0, 0, COL8_FFFFFF, s);
    sprintf(s, "memory %dMB free : %dKB", memtotal >> 20, memman_available(memman) >> 10);
    putfont_asc(buf_back, bootinfo->scrnx, 0, 32, COL8_FFFFFF, s);

    sheet_refresh(sht_back, 0, 0, bootinfo->scrnx, 48);

    int i, count = 0;
    // 无限循环，等待硬件中断
    for (;;)
    {
        count++;
        io_cli();
        if (fifo32_status(&fifo))
        {
            i = fifo32_get(&fifo);
            io_sti();
            if (256 <= i && i < 512)
            { // keyboard
                sprintf(s, "%02X", i - 256);
                putfonts8_asc_sht(sht_back, COL8_008484, COL8_FFFFFF, 0, 16, s, 2);
            }
            else if (512 <= i && i < 768)
            { // mouse
                if (mouse_decode(&mdec, i - 512))
                {
                    sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
                    if ((mdec.button & 0x01) != 0)
                        s[1] = 'L';
                    if ((mdec.button & 0x02) != 0)
                        s[3] = 'R';
                    if ((mdec.button & 0x04) != 0)
                        s[2] = 'C';
                    putfonts8_asc_sht(sht_back, COL8_008484, COL8_FFFFFF, 32, 16, s, 15);
                    // 移动鼠标
                    mx += mdec.x;
                    my += mdec.y;
                    if (mx < 0)
                        mx = 0;
                    else if (mx > bootinfo->scrnx - 1)
                        mx = bootinfo->scrnx - 1;
                    if (my < 0)
                        my = 0;
                    else if (my > bootinfo->scrny - 1)
                        my = bootinfo->scrny - 1;
                    sprintf(s, "(%3d, %3d)", mx, my);
                    putfonts8_asc_sht(sht_back, COL8_008484, COL8_FFFFFF, 0, 0, s, 10);
                    sheet_slide(sht_mouse, mx, my); /* 内含 sheet_refresh */
                }
            }
            else if (i == 10)
            {
                putfonts8_asc_sht(sht_back, COL8_008484, COL8_FFFFFF, 0, 64, "10[sec]", 7);
                sprintf(s, "%010d", count);
                putfonts8_asc_sht(sht_win, COL8_C6C6C6, COL8_000000, 40, 28, s, 10);
            }
            else if (i == 3)
            {
                putfonts8_asc_sht(sht_back, COL8_008484, COL8_FFFFFF, 0, 80, "3[sec]", 6);
                count = 0;
            }
            else if (i == 1)
            {
                timer_init(timer3, &fifo, 0);
                boxfill8(buf_back, bootinfo->scrnx, COL8_FFFFFF, 8, 96, 15, 111);
                timer_settime(timer3, 50);
                sheet_refresh(sht_back, 8, 96, 16, 112);
            }
            else if (i == 0)
            {
                timer_init(timer3, &fifo, 1);
                boxfill8(buf_back, bootinfo->scrnx, COL8_008484, 8, 96, 15, 111);
                timer_settime(timer3, 50);
                sheet_refresh(sht_back, 8, 96, 16, 112);
            }
        }
        else
        {
            io_sti();
        }
    }
}

void putfonts8_asc_sht(struct Sheet *sht, int bc, int c, int x0, int y0, char *s, int l)
{
    boxfill8(sht->buf, sht->bxsize, bc, x0, y0, x0 + 8 * l - 1, y0 + 15);
    putfont_asc(sht->buf, sht->bxsize, x0, y0, c, s);
    sheet_refresh(sht, x0, y0, x0 + 8 * l, y0 + 16);
    return;
}

void set490(struct FIFO32 *fifo, int mode)
{
    int i;
    struct TIMER *timer;
    if (mode != 0)
    {
        for (i = 0; i < 490; i++)
        {
            timer = timer_alloc();
            timer_init(timer, fifo, 1024 + i);
            timer_settime(timer, 100 * 60 * 60 * 24 * 50 + i * 100);
        }
    }
    return;
}
