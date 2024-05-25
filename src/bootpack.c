// boot 核心逻辑

#include <stdio.h>
#include "bootpack.h"

void putfonts8_asc_sht(struct Sheet *sht, int bc, int c, int x0, int y0, char *s, int l);
void set490(struct FIFO32 *fifo, int mode);
void make_textbox8(struct Sheet *sht, int x0, int y0, int sx, int sy, int c);

struct TSS32
{
    int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;
    int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
    int es, cs, ss, ds, fs, gs;
    int ldtr, iomap;
};

void task_b_main(struct Sheet *sht_back);

void HariMain(void)
{
    struct BOOTINFO *bootinfo = (struct BOOTINFO *)ADR_BOOTINFO;
    struct FIFO32 fifo;
    char s[40];
    int fifobuf[128];
    struct TIMER *timer, *timer2, *timer3, *timer_ts;
    int mx = (bootinfo->scrnx - 16) / 2;
    int my = (bootinfo->scrny - 28 - 16) / 2;
    unsigned int memtotal;
    struct MouseDescriptor mdec;                                        // mouse
    struct MemoryManager *memman = (struct MemoryManager *)MEMMAN_ADDR; // memory manager
    struct SheetController *sheet_controller;                           // sheet ctl
    struct Sheet *sht_back, *sht_mouse, *sht_win;
    unsigned char *buf_back, buf_mouse[256], *buf_win;
    int cursor_x, cursor_c;
    static char keytable[0x54] = {
        0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0, 0,
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0, 0, 'A', 'S',
        'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0, 0, ']', 'Z', 'X', 'C', 'V',
        'B', 'N', 'M', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.'};

    struct TSS32 tss_a, tss_b;
    struct SegmentDescriptor *gdt = (struct SegmentDescriptor *)ADR_GDT;
    int task_b_esp; // 为任务b声明专用的栈区

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

    // init memory management
    memtotal = mem_test(0x00400000, 0xbfffffff);
    memman_init(memman);
    memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 - 0x0009efff */
    memman_free(memman, 0x00400000, memtotal - 0x00400000);

    // init timers
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

    // init mouse
    sheet_set_buf(sht_mouse, buf_mouse, 16, 16, 99); /* 透明色号99 */
    init_mouse_cursor8(buf_mouse, 99);               /* 透明色号99 */
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

    make_textbox8(sht_win, 8, 28, 144, 16, COL8_FFFFFF);
    cursor_x = 8;
    cursor_c = COL8_FFFFFF;
    sheet_refresh(sht_win, 8, 28, 152, 44);

    tss_a.ldtr = 0;
    tss_a.iomap = 0x40000000;
    tss_b.ldtr = 0;
    tss_b.iomap = 0x40000000;
    set_segmdesc(gdt + 3, 103, (int)&tss_a, AR_TSS32);
    set_segmdesc(gdt + 4, 103, (int)&tss_b, AR_TSS32);
    load_tr(3 * 8);
    task_b_esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 8; // 此处需要减去8
    *((int *)(task_b_esp + 4)) = (int)sht_back;                      // 手动压入参数
    // 为什么不是减去4？因为本来的范围就在（0x01234000～0x01243fff），需要减去4得到0x01243ffc作为栈顶
    // 否则栈顶在0x01244000，超出了范围。由于手动压入了一个参数，需要在减4的基础上再减4，即减8
    tss_b.eip = (int)&task_b_main;
    tss_b.eflags = 0x00000202; /* IF = 1; */
    tss_b.eax = 0;
    tss_b.ecx = 0;
    tss_b.edx = 0;
    tss_b.ebx = 0;
    tss_b.esp = task_b_esp;
    tss_b.ebp = 0;
    tss_b.esi = 0;
    tss_b.edi = 0;
    tss_b.es = 1 * 8;
    tss_b.cs = 2 * 8;
    tss_b.ss = 1 * 8;
    tss_b.ds = 1 * 8;
    tss_b.fs = 1 * 8;
    tss_b.gs = 1 * 8;

    timer_ts = timer_alloc();
    timer_init(timer_ts, &fifo, 2);
    timer_settime(timer_ts, 2);

    int i;
    // 无限循环，等待硬件中断
    for (;;)
    {
        io_cli();
        if (fifo32_status(&fifo))
        {
            i = fifo32_get(&fifo);
            io_sti();
            if (i == 2)
            {
                farjmp(0, 4 * 8);
                timer_settime(timer_ts, 2);
            }
            else if (256 <= i && i < 512)
            { // keyboard
                sprintf(s, "%02X", i - 256);
                putfonts8_asc_sht(sht_back, 0, 16, COL8_FFFFFF, COL8_008484, s, 2);
                if (i < 0x54 + 256)
                {
                    if (keytable[i - 256] != 0 && cursor_x < 144)
                    {
                        s[0] = keytable[i - 256];
                        s[1] = 0;
                        putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, s, 1);
                        cursor_x += 8;
                    }
                }
                if (i == 256 + 0x0e && cursor_x > 8)
                {
                    putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, " ", 1);
                    cursor_x -= 8;
                }
                boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
                sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
            }
            else if (512 <= i && i < 768)
            { // mouse
                if (mouse_decode(&mdec, i - 512))
                {
                    sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
                    if ((mdec.button & 0x01) != 0)
                    {
                        s[1] = 'L';
                        sheet_slide(sht_win, mx - 80, my - 8);
                    }
                    if ((mdec.button & 0x02) != 0)
                        s[3] = 'R';
                    if ((mdec.button & 0x04) != 0)
                        s[2] = 'C';
                    putfonts8_asc_sht(sht_back, 32, 16, COL8_FFFFFF, COL8_008484, s, 15);
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
                    putfonts8_asc_sht(sht_back, 0, 0, COL8_FFFFFF, COL8_008484, s, 10);
                    sheet_slide(sht_mouse, mx, my); /* 内含 sheet_refresh */
                }
            }
            else if (i == 10)
            {
                putfonts8_asc_sht(sht_back, 0, 64, COL8_FFFFFF, COL8_008484, "10[sec]", 7);
            }
            else if (i == 3)
            {
                putfonts8_asc_sht(sht_back, 0, 80, COL8_FFFFFF, COL8_008484, "3[sec]", 6);
            }
            else if (i <= 1)
            {
                if (i)
                {
                    timer_init(timer3, &fifo, 0);
                    cursor_c = COL8_000000;
                }
                else
                {
                    timer_init(timer3, &fifo, 1);
                    cursor_c = COL8_FFFFFF;
                }
                timer_settime(timer3, 50);
                boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
                sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
            }
        }
        else
        {
            io_stihlt();
        }
    }
}

void putfonts8_asc_sht(struct Sheet *sht, int x0, int y0, int c, int bc, char *s, int l)
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

void make_textbox8(struct Sheet *sht, int x0, int y0, int sx, int sy, int c)
{
    int x1 = x0 + sx, y1 = y0 + sy;
    boxfill8(sht->buf, sht->bxsize, COL8_848484, x0 - 2, y0 - 3, x1 + 1, y0 - 3);
    boxfill8(sht->buf, sht->bxsize, COL8_848484, x0 - 3, y0 - 3, x0 - 3, y1 + 1);
    boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, x0 - 3, y1 + 2, x1 + 1, y1 + 2);
    boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, x1 + 2, y0 - 3, x1 + 2, y1 + 2);
    boxfill8(sht->buf, sht->bxsize, COL8_000000, x0 - 1, y0 - 2, x1 + 0, y0 - 2);
    boxfill8(sht->buf, sht->bxsize, COL8_000000, x0 - 2, y0 - 2, x0 - 2, y1 + 0);
    boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, x0 - 2, y1 + 1, x1 + 0, y1 + 1);
    boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, x1 + 1, y0 - 2, x1 + 1, y1 + 1);
    boxfill8(sht->buf, sht->bxsize, c, x0 - 1, y0 - 1, x1 + 0, y1 + 0);
    return;
}

void task_b_main(struct Sheet *sht_back)
{
    struct FIFO32 fifo;
    struct TIMER *timer_ts, *timer_put, *timer_1s;
    int i, fifobuf[128], count = 0, count0 = 0;
    char s[12];
    fifo32_init(&fifo, 128, fifobuf);
    timer_ts = timer_alloc();
    timer_init(timer_ts, &fifo, 2);
    timer_settime(timer_ts, 2);
    timer_put = timer_alloc();
    timer_init(timer_put, &fifo, 1);
    // timer_settime(timer_put, 1);
    timer_1s = timer_alloc();
    timer_init(timer_1s, &fifo, 100);
    timer_settime(timer_1s, 100);
    for (;;)
    {
        count++;
        io_cli();
        if (fifo32_status(&fifo) == 0)
        {
            io_sti();
        }
        else
        {
            i = fifo32_get(&fifo);
            io_sti();
            if (i == 1)
            { // 降低刷新频率，提高程序运行效率
                sprintf(s, "%11d", count);
                putfonts8_asc_sht(sht_back, 0, 144, COL8_FFFFFF, COL8_008484, s, 11);
                timer_settime(timer_put, 1);
            }
            else if (i == 2)
            {
                farjmp(0, 3 * 8);
                timer_settime(timer_ts, 2);
            }
            else if (i == 100)
            {
                sprintf(s, "%11d", count - count0);
                putfonts8_asc_sht(sht_back, 0, 128, COL8_FFFFFF, COL8_008484, s, 11);
                count0 = count;
                timer_settime(timer_1s, 100);
            }
        }
    }
    // 千万不能return! return本身是用于返回母函数调用该函数的地址，
    // 但b进程不是由其他函数调用的子函数，所以使用return会出现错误。
}
