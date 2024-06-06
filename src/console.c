// 命令行相关

#include "bootpack.h"

void console_task(struct SHEET *sheet, unsigned int memtotal)
{
    struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
    int *fat = (int *)memman_alloc_4k(memman, 4 * 2880);
    struct FILEINFO *finfo = (struct FILEINFO *)(ADR_DISKIMG + 0x002600);
    struct TIMER *timer;
    struct TASK *task = task_now(); // 获取自身内存地址
    int i, fifobuf[128], cursor_x = 24, cursor_y = 28, cursor_c = -1;
    char s[30], cmdline[30], *p;
    int x, y;

    file_readfat(fat, (unsigned char *)(ADR_DISKIMG + 0x000200));

    fifo32_init(&task->fifo, 128, fifobuf, task);
    timer = timer_alloc();
    timer_init(timer, &task->fifo, 1);
    timer_settime(timer, 50); // 输入闪烁计时器

    putfonts8_asc_sht(sheet, 8, 28, COL8_FFFFFF, COL8_000000, "> ", 2);
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
                    if (cursor_c >= 0)
                        cursor_c = COL8_FFFFFF;
                }
                else
                {
                    timer_init(timer, &task->fifo, 1); // 下次置1
                    if (cursor_c >= 0)
                        cursor_c = COL8_000000;
                }
                timer_settime(timer, 50);
            }
            if (i == 2) /*光标ON */ /*从此开始*/
                cursor_c = COL8_FFFFFF;
            if (i == 3)
            { /*光标OFF */
                boxfill8(sheet->buf, sheet->bxsize, COL8_000000, cursor_x, 28, cursor_x + 7, 43);
                cursor_c = -1;
            }
            if (256 <= i && i <= 511)
            { // 键盘数据（通过任务A）
                if (i == 8 + 256)
                { // 退格键
                    if (cursor_x > 24)
                    {
                        // 用空白擦除光标后将光标前移一位
                        putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ", 1);
                        cursor_x -= 8;
                    }
                }
                else if (i == 10 + 256)
                { // 换行，空格结尾，下一行开头打印提示符>
                    putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ", 1);
                    cmdline[cursor_x / 8 - 3] = 0; // 命令字符串结尾
                    cursor_y = cons_newline(cursor_y, sheet);
                    if ((strcmp(cmdline, "mem") == 0))
                    { // mem命令
                        sprintf(s, "total %dMB", memtotal >> 20);
                        putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s, 30);
                        cursor_y = cons_newline(cursor_y, sheet);

                        sprintf(s, "free %dKB", memman_total(memman) >> 10);
                        putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s, 30);
                        cursor_y = cons_newline(cursor_y, sheet);

                        cursor_y = cons_newline(cursor_y, sheet);
                    }
                    else if (strcmp(cmdline, "cls") == 0)
                    {
                        for (y = 28; y < 28 + 128; y++)
                        {
                            for (x = 8; x < 8 + 240; x++)
                                sheet->buf[x + y * sheet->bxsize] = COL8_000000;
                        }
                        sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
                        cursor_y = 28;
                    }
                    else if (strcmp(cmdline, "dir") == 0)
                    {
                        /* dir命令 */
                        for (x = 0; x < 224; x++)
                        {
                            if (finfo[x].name[0] == 0x00)
                            {
                                break;
                            }
                            if (finfo[x].name[0] != 0xe5)
                            {
                                if ((finfo[x].type & 0x18) == 0)
                                {
                                    sprintf(s, "filename.ext %7d", finfo[x].size);
                                    for (y = 0; y < 8; y++)
                                        s[y] = finfo[x].name[y];
                                    s[9] = finfo[x].ext[0];
                                    s[10] = finfo[x].ext[1];
                                    s[11] = finfo[x].ext[2];
                                    putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s, 30);
                                    cursor_y = cons_newline(cursor_y, sheet);
                                }
                            }
                        }
                        cursor_y = cons_newline(cursor_y, sheet);
                    }
                    else if (strncmp(cmdline, "type ", 5) == 0)
                    {
                        for (x = 5; cmdline[x] == ' ' && x < 30; x++)
                            ;
                        for (y = 0; y < 11; y++)
                            s[y] = ' ';
                        for (y = 0; y < 11 && cmdline[x] != 0; x++)
                        {
                            if (cmdline[x] == '.' && y <= 8)
                                y = 8;
                            else
                            {
                                s[y] = cmdline[x];
                                if ('a' <= s[y] && s[y] <= 'z')
                                    s[y] -= 0x20; // UPPER CASE
                                y++;
                            }
                        }
                        for (x = 0; x < 224; x++)
                        {
                            if (finfo[x].name[0] == 0x00)
                                break;
                            if ((finfo[x].type & 0x18) == 0)
                            {
                                for (y = 0; y < 11; y++)
                                    if (finfo[x].name[y] != s[y])
                                        goto continuing;
                                break; // find the file!
                            }
                        continuing:
                        }
                        if (x < 224 && finfo[x].name[0] != 0x00)
                        { // 找到文件
                            p = (char *)memman_alloc_4k(memman, finfo[x].size);
                            file_loadfile(finfo[x].clustno, finfo[x].size, p, fat, (char *)(ADR_DISKIMG + 0x003e00));
                            cursor_x = 8;
                            s[1] = 0;
                            for (y = 0; y < finfo[x].size; y++)
                            {
                                s[0] = p[y];
                                if (s[0] == 0x09)
                                { // 制表符
                                    for (;;)
                                    {
                                        putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ", 1);
                                        cursor_x += 8;
                                        if (cursor_x == 8 + 240)
                                        {
                                            cursor_x = 8;
                                            cursor_y = cons_newline(cursor_y, sheet);
                                        }
                                        // (cursor_x - 8) 命令行窗口的边框有8个像素，要把那部分给去掉
                                        if (((cursor_x - 8) & 0x1f) == 0)
                                            break; // 被32整除则break，缩进为四个字符
                                    }
                                }
                                else if (s[0] == 0x01)
                                { // 换行符
                                    cursor_x = 8;
                                    cursor_y = cons_newline(cursor_y, sheet);
                                }
                                else if (s[0] == 0x0d)
                                { // 回车
                                  //  暂时不做操作
                                }
                                else
                                { // 一般字符
                                    putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, s, 1);
                                    cursor_x += 8;
                                    if (cursor_x == 8 + 240)
                                    { // 输出占满一行则换行
                                        cursor_x = 8;
                                        cursor_y = cons_newline(cursor_y, sheet);
                                    }
                                }
                            } // end for
                            memman_free_4k(memman, (int)p, finfo[x].size);
                        }
                        else
                        { // 没找到
                            putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, "File not found.", 15);
                            cursor_y = cons_newline(cursor_y, sheet);
                        }
                        cursor_y = cons_newline(cursor_y, sheet);
                    }
                    else if (cmdline[0] != 0)
                    { /*不是命令，也不是空行 */
                        putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, "Bad command.", 12);
                        cursor_y = cons_newline(cursor_y, sheet);
                        cursor_y = cons_newline(cursor_y, sheet);
                    }
                    cursor_y = cons_newline(cursor_y, sheet);
                    // if (cursor_y < 28 + 112)
                    //     cursor_y += 16;
                    // else
                    // { // 窗口自动滚动
                    //     for (y = 28; y < 28 + 112; y++)
                    //     {
                    //         for (x = 16; x < 248; x++)
                    //             sheet->buf[x + y * sheet->bxsize] = sheet->buf[x + (y + 16) * sheet->bxsize];
                    //     }
                    //     for (y = 28 + 112; y < 28 + 128; y++)
                    //     {
                    //         for (x = 16; x < 248; x++)
                    //             sheet->buf[x + y * sheet->bxsize] = COL8_000000;
                    //     }
                    //     sheet_refresh(sheet, 16, 28, 248, 28 + 128);
                    // }
                    putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, "> ", 2);
                    cursor_x = 24;
                }
                else
                { // 一般字符
                    if (cursor_x < 240)
                    {
                        // 显示一个字符之后将光标后移一位
                        s[0] = i - 256;
                        s[1] = 0;
                        cmdline[cursor_x / 8 - 3] = i - 256;
                        putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, s, 1);
                        cursor_x += 8;
                    }
                }
            }
            // 重新显示光标
            if (cursor_c >= 0)
                boxfill8(sheet->buf, sheet->bxsize, cursor_c, cursor_x, cursor_y, cursor_x + 7, cursor_y + 15);
            sheet_refresh(sheet, cursor_x, cursor_y, cursor_x + 8, cursor_y + 16);
        }
    }
}

int cons_newline(int cursor_y, struct SHEET *sheet)
{
    int x, y;
    if (cursor_y < 28 + 112)
        cursor_y += 16; /*换行*/
    else
    {
        /*滚动*/
        for (y = 28; y < 28 + 112; y++)
        {
            for (x = 8; x < 8 + 240; x++)
            {
                sheet->buf[x + y * sheet->bxsize] = sheet->buf[x + (y + 16) * sheet->bxsize];
            }
        }
        for (y = 28 + 112; y < 28 + 128; y++)
        {
            for (x = 8; x < 8 + 240; x++)
            {
                sheet->buf[x + y * sheet->bxsize] = COL8_000000;
            }
        }
        sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
    }
    return cursor_y;
}
