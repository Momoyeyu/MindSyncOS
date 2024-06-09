// 命令行相关

#include "bootpack.h"
#include <stdio.h>

void console_task(struct SHEET *sheet, unsigned int memtotal)
{
    struct TIMER *timer;
    struct TASK *task = task_now();
    struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
    int i, fifobuf[128], *fat = (int *)memman_alloc_4k(memman, 4 * 2880);
    struct CONSOLE cons;
    char cmdline[30];
    cons.sht = sheet;
    cons.cur_x = 24;
    cons.cur_y = 28;
    cons.cur_c = -1;
    *((int *)0x0fec) = (int)&cons; // 将cons的地址保存，供外部应用程序调用API时使用

    fifo32_init(&task->fifo, 128, fifobuf, task);
    timer = timer_alloc();
    timer_init(timer, &task->fifo, 1);
    timer_settime(timer, 50); // 输入闪烁计时器
    file_readfat(fat, (unsigned char *)(ADR_DISKIMG + 0x000200));

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
                    if (cons.cur_c >= 0)
                        cons.cur_c = COL8_FFFFFF;
                }
                else
                {
                    timer_init(timer, &task->fifo, 1); // 下次置1
                    if (cons.cur_c >= 0)
                        cons.cur_c = COL8_000000;
                }
                timer_settime(timer, 50);
            }
            if (i == 2) // 光标ON  // 从此开始
                cons.cur_c = COL8_FFFFFF;
            if (i == 3)
            { // 光标OFF
                boxfill8(sheet->buf, sheet->bxsize, COL8_000000, cons.cur_x, 28, cons.cur_x + 7, 43);
                cons.cur_c = -1;
            }
            if (256 <= i && i <= 511) // 键盘数据（通过任务A）
            {
                if (i == 8 + 256) // 退格键
                {
                    if (cons.cur_x > 24)
                    { // 用空白擦除光标后将光标前移一位
                        putfonts8_asc_sht(sheet, cons.cur_x, cons.cur_y, COL8_FFFFFF, COL8_000000, " ", 1);
                        cons.cur_x -= 8;
                    }
                }
                else if (i == 10 + 256)
                { // Enter 换行（执行），空格结尾，下一行开头打印提示符>
                    putfonts8_asc_sht(sheet, cons.cur_x, cons.cur_y, COL8_FFFFFF, COL8_000000, " ", 1);
                    cmdline[cons.cur_x / 8 - 3] = 0; // 命令字符串结尾
                    cons_newline(&cons);
                    cons_runcmd(cmdline, &cons, fat, memtotal);
                    cons_newline(&cons);
                    putfonts8_asc_sht(sheet, 8, cons.cur_y, COL8_FFFFFF, COL8_000000, "> ", 2);
                    cons.cur_x = 24;
                }
                else
                { // 一般字符
                    if (cons.cur_x < 240)
                    {
                        cmdline[cons.cur_x / 8 - 3] = i - 256;
                        cons_putchar(&cons, i - 256, 1);
                    }
                }
            }
            // 重新显示光标
            if (cons.cur_c >= 0)
                boxfill8(sheet->buf, sheet->bxsize, cons.cur_c, cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
            sheet_refresh(sheet, cons.cur_x, cons.cur_y, cons.cur_x + 8, cons.cur_y + 16);
        }
    }
}

void cons_putchar(struct CONSOLE *cons, int chr, char move)
{
    char s[2];
    s[0] = chr;
    s[1] = 0;
    if (s[0] == 0x09)
    { // 制表符
        for (;;)
        {
            putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, " ", 1);
            cons->cur_x += 8;
            if (cons->cur_x == 8 + 240)
            {
                cons_newline(cons); // 换行后cursor_x = 8，下一轮必然break
                break;
            }
            // (cons.cur_x - 8) 命令行窗口的边框有8个像素，要把那部分给去掉
            if (((cons->cur_x - 8) & 0x1f) == 0)
                break; // 被32整除则break
        }
    }
    else if (s[0] == 0x0a) // 换行
        cons_newline(cons);
    else if (s[0] == 0x0d)
    { // 回车
      // 先不做任何操作
    }
    else
    { // 一般字符
        putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, s, 1);
        if (move != 0)
        { // move为0时光标不后移
            cons->cur_x += 8;
            if (cons->cur_x == 8 + 240)
                cons_newline(cons);
        }
    }
    return;
}

void cons_newline(struct CONSOLE *cons)
{ // 命令行换行
    int x, y;
    struct SHEET *sheet = cons->sht;
    if (cons->cur_y < 28 + 112)
        cons->cur_y += 16; // 下一行
    else
    {
        // 之前的行上移
        for (y = 28; y < 28 + 112; y++)
            for (x = 8; x < 8 + 240; x++)
                sheet->buf[x + y * sheet->bxsize] = sheet->buf[x + (y + 16) * sheet->bxsize];
        // 新的一行变黑
        for (y = 28 + 112; y < 28 + 128; y++)
            for (x = 8; x < 8 + 240; x++)
                sheet->buf[x + y * sheet->bxsize] = COL8_000000;
        // 刷新显示
        sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
    }
    cons->cur_x = 8;
    return;
}

void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, unsigned int memtotal)
{
    if (strcmp(cmdline, "mem") == 0)
        cmd_mem(cons, memtotal);
    else if (strcmp(cmdline, "cls") == 0)
        cmd_cls(cons);
    else if (strcmp(cmdline, "dir") == 0)
        cmd_dir(cons);
    else if (strncmp(cmdline, "type ", 5) == 0)
        cmd_type(cons, fat, cmdline);
    else if (cmdline[0] != 0)
        if (cmd_app(cons, fat, cmdline) == 0) // 不是命令，不是应用程序，也不是空行
            cons_putstr0(cons, "Bad command.\n\n");
    return;
}

void cmd_mem(struct CONSOLE *cons, unsigned int memtotal)
{
    struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
    char s[60];
    sprintf(s, "total %dMB\nfree %dKB\n", memtotal >> 20, memman_total(memman) >> 10);
    cons_putstr0(cons, s);
    return;
}

void cmd_cls(struct CONSOLE *cons)
{
    int i, j;
    for (i = 28; i < 28 + 128; i++)
        for (j = 8; j < 8 + 240; j++)
            cons->sht->buf[j + i * cons->sht->bxsize] = COL8_000000;
    sheet_refresh(cons->sht, 8, 28, 8 + 240, 28 + 128);
    cons->cur_y = 28;
    return;
}
//
// 0x01……只读文件（不可写入）
// 0x02……隐藏文件
// 0x04……系统文件
// 0x08……非文件信息（比如磁盘名称等）
// 0x10……目录
//
void cmd_dir(struct CONSOLE *cons)
{ // dir命令，显示当前目录文件
    struct FILEINFO *finfo = (struct FILEINFO *)(ADR_DISKIMG + 0x002600);
    int i, j;
    char s[30];
    // 从头到尾检查fileinfo
    for (i = 0; i < 224; i++)
    {
        if (finfo[i].name[0] == 0x00) // 0x00 表示此后没有更多文件信息
            break;
        if (finfo[i].name[0] != 0xe5) // 0xe5 表示文件已经被删除
        {
            if ((finfo[i].type & 0x18) == 0) // 排除“非文件信息”与“目录”
            {
                sprintf(s, "filename.ext %7d\n", finfo[i].size);
                for (j = 0; j < 8; j++)
                    s[j] = finfo[i].name[j];
                s[9] = finfo[i].ext[0];
                s[10] = finfo[i].ext[1];
                s[11] = finfo[i].ext[2];
                cons_putstr0(cons, s);
            }
        }
    }
    cons_newline(cons);
    return;
}

void cmd_type(struct CONSOLE *cons, int *fat, char *cmdline)
{ // 在命令行输出文件内容
    struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
    int i;
    char *p;
    for (i = 5; cmdline[i] == ' ' && i < 30; i++)
        ; // 定位到参数的开头
    struct FILEINFO *finfo = file_search(cmdline + i, (struct FILEINFO *)(ADR_DISKIMG + 0x002600), 224);
    if (finfo)
    { // 找到文件，输出内容
        // 将文件内容写进内存p
        p = (char *)memman_alloc_4k(memman, finfo->size);
        file_loadfile(finfo->clustno, finfo->size, p, fat, (char *)(ADR_DISKIMG + 0x003e00));
        cons_putstr1(cons, p, finfo->size);          // 输出文件内容
        memman_free_4k(memman, (int)p, finfo->size); // 释放内存
    }
    else // 没找到，输出报错
        cons_putstr0(cons, "File not found.\n");
    cons_newline(cons);
    return;
}

int cmd_app(struct CONSOLE *cons, int *fat, char *cmdline)
{
    struct FILEINFO *finfo;
    struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *)ADR_GDT;
    char name[18], *p;
    int i;
    for (i = 0; i < 13; i++)
    {
        if (cmdline[i] == 0 || cmdline[i] == ' ')
            break;
        name[i] = cmdline[i];
    }
    name[i] = 0;
    finfo = file_search(name, (struct FILEINFO *)(ADR_DISKIMG + 0x002600), 224);
    if (finfo == 0 && name[i - 1] != '.')
    { // 由于找不到文件，故在文件名后面加上“.hrb”后重新寻找
        name[i] = '.';
        name[i + 1] = 'H';
        name[i + 2] = 'R';
        name[i + 3] = 'B';
        name[i + 4] = 0;
        finfo = file_search(name, (struct FILEINFO *)(ADR_DISKIMG + 0x002600), 224);
    }

    if (finfo)
    { // 找到文件的情况
        p = (char *)memman_alloc_4k(memman, finfo->size);
        *((int *)0xfe8) = (int)p; // 向hrb_api直接传递数据（代码段）
        file_loadfile(finfo->clustno, finfo->size, p, fat, (char *)(ADR_DISKIMG + 0x003e00));
        set_segmdesc(gdt + 1003, finfo->size - 1, (int)p, AR_CODE32_ER);
        // farjmp(0, 1003 * 8); // 切换到hlt进程
        farcall(0, 1003 * 8); // 切换到hlt进程
        memman_free_4k(memman, (int)p, finfo->size);
        return 1;
    }
    // 没有找到文件的情况
    // putfonts8_asc_sht(cons->sht, 8, cons->cur_y, COL8_FFFFFF, COL8_000000, "File not found.", 15);
    // cons_newline(cons);
    return 0;
}

void cons_putstr0(struct CONSOLE *cons, char *s)
{
    for (; *s != 0; s++)
        cons_putchar(cons, *s, 1);
    return;
}

void cons_putstr1(struct CONSOLE *cons, const char *s, int l)
{
    int i;
    for (i = 0; i < l; i++)
        cons_putchar(cons, s[i], 1);
    return;
}

void hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax)
{ // 寄存器顺序是按照PUSHAD的顺序写的
    struct CONSOLE *cons = (struct CONSOLE *)*((int *)0x0fec);
    int cs_base = *((int *)0xfe8); // 获取段寄存器基址
    if (edx == 1)
        cons_putchar(cons, eax & 0xff, 1);
    else if (edx == 2)
        cons_putstr0(cons, (char *)ebx + cs_base); // 此处加上基址
    else if (edx == 3)
        cons_putstr1(cons, (char *)ebx + cs_base, ecx); // 此处加上基址
    return;
}
