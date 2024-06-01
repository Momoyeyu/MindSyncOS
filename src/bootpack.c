// boot 核心逻辑

#include <stdio.h>
#include "bootpack.h"

void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c);
void console_task(struct SHEET *sheet);

void HariMain(void)
{
    struct BOOTINFO *bootinfo = (struct BOOTINFO *)ADR_BOOTINFO;
    struct FIFO32 fifo;
    char s[40];
    int fifobuf[128];
    int mx, my, i, cursor_x, cursor_c;
    unsigned int memtotal;
    struct MOUSE_DEC mdec;
    struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
    struct SHTCTL *shtctl;
    static char keytable0[0x80] = {
        0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0, 0,
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0, 0, 'A', 'S',
        'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0, 0, ']', 'Z', 'X', 'C', 'V',
        'B', 'N', 'M', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6', '+', '1',
        '2', '3', '0', '.', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0x5c, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x5c, 0, 0};
    static char keytable1[0x80] = {
        0, 0, '!', 0x22, '#', '$', '%', '&', 0x27, '(', ')', '~', '=', '~', 0, 0,
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '`', '{', 0, 0, 'A', 'S',
        'D', 'F', 'G', 'H', 'J', 'K', 'L', '+', '*', 0, 0, '}', 'Z', 'X', 'C', 'V',
        'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6', '+', '1',
        '2', '3', '0', '.', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, '_', 0, 0, 0, 0, 0, 0, 0, 0, 0, '|', 0, 0};
    unsigned char *buf_back, buf_mouse[256], *buf_win, *buf_cons;
    struct SHEET *sht_back, *sht_mouse, *sht_win, *sht_cons;
    struct TASK *task_a, *task_cons;
    struct TIMER *timer;
    int key_to = 0, key_shift = 0, key_leds = (bootinfo->leds >> 4) & 7;

    // initialization
    init_gdtidt();
    init_pic();
    io_sti(); // 初始化完 GDT IDT PIC 之后才开中断
    init_pit();
    fifo32_init(&fifo, 128, fifobuf, 0);
    init_keyboard(&fifo, 256);
    enable_mouse(&fifo, 512, &mdec);
    io_out8(PIC0_IMR, 0xf8); // PIT和PIC1和键盘设置为许可(11111000)
    io_out8(PIC1_IMR, 0xef); // 允许鼠标中断（11101111）

    // init memory management
    memtotal = memtest(0x00400000, 0xbfffffff);
    memman_init(memman);
    memman_free(memman, 0x00001000, 0x0009e000); // 0x00001000 - 0x0009efff
    memman_free(memman, 0x00400000, memtotal - 0x00400000);

    // ---------------------------------- 显示部分 ----------------------------------
    init_palette();
    shtctl = shtctl_init(memman, bootinfo->vram, bootinfo->scrnx, bootinfo->scrny);
    task_a = task_init(memman); // 获取Harimain任务
    fifo.task = task_a;
    task_run(task_a, 1, 2); // 改变优先级

    // init screen
    sht_back = sheet_alloc(shtctl);
    buf_back = (unsigned char *)memman_alloc_4k(memman, bootinfo->scrnx * bootinfo->scrny);
    sheet_setbuf(sht_back, buf_back, bootinfo->scrnx, bootinfo->scrny, -1); // 没有透明色
    init_screen8(buf_back, bootinfo->scrnx, bootinfo->scrny);

    // init console
    sht_cons = sheet_alloc(shtctl);
    buf_cons = (unsigned char *)memman_alloc_4k(memman, 256 * 165);
    sheet_setbuf(sht_cons, buf_cons, 256, 165, -1); // 无透明色
    make_window8(buf_cons, 256, 165, "console", 0);
    make_textbox8(sht_cons, 8, 28, 240, 128, COL8_000000);
    task_cons = task_alloc();
    task_cons->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 8;
    task_cons->tss.eip = (int)&console_task;
    task_cons->tss.es = 1 * 8;
    task_cons->tss.cs = 2 * 8;
    task_cons->tss.ss = 1 * 8;
    task_cons->tss.ds = 1 * 8;
    task_cons->tss.fs = 1 * 8;
    task_cons->tss.gs = 1 * 8;
    *((int *)(task_cons->tss.esp + 4)) = (int)sht_cons;
    task_run(task_cons, 2, 2); // level=2, priority=2

    // init window
    sht_win = sheet_alloc(shtctl);
    buf_win = (unsigned char *)memman_alloc_4k(memman, 160 * 52);
    sheet_setbuf(sht_win, buf_win, 144, 52, -1); // 没有透明色
    make_window8(buf_win, 144, 52, "task_a", 1);
    make_textbox8(sht_win, 8, 28, 128, 16, COL8_FFFFFF);
    cursor_x = 8;
    cursor_c = COL8_FFFFFF;

    // init timers
    timer = timer_alloc();
    timer_init(timer, &fifo, 1); // 1 or 0
    timer_settime(timer, 50);

    // init mouse
    sht_mouse = sheet_alloc(shtctl);
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
    init_mouse_cursor8(buf_mouse, 99);
    mx = (bootinfo->scrnx - 16) / 2; // 画面中央になるように座標計算
    my = (bootinfo->scrny - 28 - 16) / 2;

    // 图层调整
    sheet_slide(sht_back, 0, 0);
    sheet_slide(sht_cons, 80, 80);
    sheet_slide(sht_win, 64, 56);
    sheet_slide(sht_mouse, mx, my);
    sheet_updown(sht_back, 0);
    sheet_updown(sht_cons, 1);
    sheet_updown(sht_win, 2);
    sheet_updown(sht_mouse, 3);

    sprintf(s, "(%3d, %3d)", mx, my);
    putfonts8_asc_sht(sht_back, 0, 0, COL8_FFFFFF, COL8_008484, s, 10);
    sprintf(s, "memory %dMB   free : %dKB", memtotal >> 20, memman_total(memman) >> 10);
    putfonts8_asc_sht(sht_back, 0, 32, COL8_FFFFFF, COL8_008484, s, 40);

    // 无限循环，等待硬件中断
    for (;;)
    {
        io_cli();
        if (fifo32_status(&fifo))
        {
            i = fifo32_get(&fifo);
            io_sti();
            if (256 <= i && i < 512)
            { // keyboard
                sprintf(s, "%02X", i - 256);
                putfonts8_asc_sht(sht_back, 0, 16, COL8_FFFFFF, COL8_008484, s, 2);
                if (i < 0x80 + 256)
                { // 将按键编码转换为字符编码
                    if (key_shift == 0)
                        s[0] = keytable0[i - 256];
                    else
                        s[0] = keytable1[i - 256];
                }
                else
                    s[0] = 0;
                if ('A' <= s[0] && s[0] <= 'Z')
                { // 当输入字符为英文字母时
                    if (((key_leds & 4) == 0 && key_shift == 0) || ((key_leds & 4) != 0 && key_shift != 0))
                        s[0] += 0x20; // 将大写字母转换为小写字母
                }

                if (s[0] != 0)
                { // 一般字符
                    if (key_to == 0)
                    { // 发送给任务A
                        if (cursor_x < 128)
                        {
                            // 显示一个字符之后将光标后移一位
                            s[1] = 0;
                            putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000,
                                              COL8_FFFFFF, s, 1);
                            cursor_x += 8;
                        }
                    }
                    else
                    { // 发送给命令行窗口
                        fifo32_put(&task_cons->fifo, s[0] + 256);
                    }
                }
                if (i == 256 + 0x0e && cursor_x > 8)
                { // 退格键
                    if (key_to == 0)
                    { // 发送给任务A
                        if (cursor_x > 8)
                        { // 用空白擦除光标后将光标前移一位
                            putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, " ", 1);
                            cursor_x -= 8;
                        }
                    }
                    else // 发送给命令行窗口
                        fifo32_put(&task_cons->fifo, 8 + 256);
                }
                if (i == 256 + 0x0f)
                { // Tab 键
                    if (key_to == 0)
                    {
                        key_to = 1;
                        make_wtitle8(buf_win, sht_win->bxsize, "task_a", 0);
                        make_wtitle8(buf_cons, sht_cons->bxsize, "console", 1);
                    }
                    else
                    {
                        key_to = 0;
                        make_wtitle8(buf_win, sht_win->bxsize, "task_a", 1);
                        make_wtitle8(buf_cons, sht_cons->bxsize, "console", 0);
                    }
                    sheet_refresh(sht_win, 0, 0, sht_win->bxsize, 21);
                    sheet_refresh(sht_cons, 0, 0, sht_cons->bxsize, 21);
                }
                if (i == 256 + 0x2a) // 左Shift ON
                    key_shift |= 1;
                if (i == 256 + 0x36) // 右Shift ON
                    key_shift |= 2;
                if (i == 256 + 0xaa) // 左Shift OFF
                    key_shift &= ~1;
                if (i == 256 + 0xb6) // 右Shift OFF
                    key_shift &= ~2;
                boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
                sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
            }
            else if (512 <= i && i < 768)
            { // mouse
                if (mouse_decode(&mdec, i - 512))
                {
                    sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
                    if ((mdec.btn & 0x01) != 0)
                    {
                        s[1] = 'L';
                        sheet_slide(sht_win, mx - 80, my - 8);
                    }
                    if ((mdec.btn & 0x02) != 0)
                        s[3] = 'R';
                    if ((mdec.btn & 0x04) != 0)
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
                    sheet_slide(sht_mouse, mx, my); // 内含 sheet_refresh
                }
            }
            else if (i <= 1)
            {
                if (i)
                {
                    timer_init(timer, &fifo, 0);
                    cursor_c = COL8_000000;
                }
                else
                {
                    timer_init(timer, &fifo, 1);
                    cursor_c = COL8_FFFFFF;
                }
                timer_settime(timer, 50);
                boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
                sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
            }
        }
        else
        {
            task_sleep(task_a);
            io_sti();
        }
    }
}

void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c)
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

void console_task(struct SHEET *sheet)
{
    struct TIMER *timer;
    struct TASK *task = task_now(); // 获取自身内存地址
    int i, fifobuf[128], cursor_x = 16, cursor_c = COL8_000000;
    char s[2];

    fifo32_init(&task->fifo, 128, fifobuf, task);
    timer = timer_alloc();
    timer_init(timer, &task->fifo, 1);
    timer_settime(timer, 50); // 输入闪烁计时器

    putfonts8_asc_sht(sheet, 8, 28, COL8_FFFFFF, COL8_000000, ">", 1);
    for (;;)
    {
        io_cli();
        if (fifo32_status(&task->fifo) == 0)
        {
            task_sleep(task);
            io_sti();
        }
        else
        {
            i = fifo32_get(&task->fifo);
            io_sti();
            if (i <= 1)
            { // 光标用定时器
                if (i != 0)
                {
                    timer_init(timer, &task->fifo, 0); // 下次置0
                    cursor_c = COL8_FFFFFF;
                }
                else
                {
                    timer_init(timer, &task->fifo, 1); // 下次置1
                    cursor_c = COL8_000000;
                }
                timer_settime(timer, 50);
            }
            if (256 <= i && i <= 511)
            { // 键盘数据（通过任务A）
                if (i == 8 + 256)
                { // 退格键
                    if (cursor_x > 16)
                    {
                        // 用空白擦除光标后将光标前移一位
                        putfonts8_asc_sht(sheet, cursor_x, 28, COL8_FFFFFF, COL8_000000, " ", 1);
                        cursor_x -= 8;
                    }
                }
                else
                { // 一般字符
                    if (cursor_x < 240)
                    {
                        // 显示一个字符之后将光标后移一位
                        s[0] = i - 256;
                        s[1] = 0;
                        putfonts8_asc_sht(sheet, cursor_x, 28, COL8_FFFFFF,
                                          COL8_000000, s, 1);
                        cursor_x += 8;
                    }
                }
            }
            // 重新显示光标
            boxfill8(sheet->buf, sheet->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
            sheet_refresh(sheet, cursor_x, 28, cursor_x + 8, 44);
        }
    }
}
