// 图层

#include "bootpack.h"
#include <stdio.h>

struct SHTCTL *shtctl_init(struct MEMMAN *man, unsigned char *vram, int xsize, int ysize)
{
    struct SHTCTL *ctl;
    int i;
    ctl = (struct SHTCTL *)memman_alloc_4k(man, sizeof(struct SHTCTL));
    if (ctl == NULL)
    {
        goto err;
    }
    ctl->vram = vram;
    ctl->xsize = xsize;
    ctl->ysize = ysize;
    ctl->top = -1; // 暂时没有任何sheet
    for (i = 0; i < MAX_SHEETS; i++)
    {
        ctl->sheets_data[i].flags = SHEET_FREE; // 标记为未使用
    }
err:
    // 将来可以引入错误处理
    return ctl;
}

struct SHEET *sheet_alloc(struct SHTCTL *ctl)
{
    struct SHEET *sheet;
    int i;
    for (i = 0; i < MAX_SHEETS; i++)
    {
        if (ctl->sheets_data[i].flags == 0)
        {
            sheet = &(ctl->sheets_data[i]);
            sheet->flags = SHEET_USED; // 标记为已使用
            sheet->height = -1;        // 隐藏
            return sheet;
        }
    }
    return NULL; // allocation failed
}

void sheet_setbuf(struct SHEET *sheet, unsigned char *buf, int xsize, int ysize, int color)
{
    sheet->buf = buf;
    sheet->bxsize = xsize;
    sheet->bysize = ysize;
    sheet->color = color;
    return;
}

void sheet_updown(struct SHTCTL *ctl, struct SHEET *sheet, int height)
{
    int old = sheet->height; /* 存储设置前的高度信息 */
    /* 如果指定的高度过高或过低，则进行修正 */
    if (height > ctl->top + 1)
        height = ctl->top + 1;
    else if (height < -1)
        height = -1;
    sheet->height = height;

    // 重新排列
    int h;
    if (height < old)
    {
        if (height < 0) // 隐藏sheet
        {
            for (h = old; h < ctl->top; h++) // 降低 (old, ctl->top] 的图层高度
            {
                ctl->sheets_ptr[h] = ctl->sheets_ptr[h + 1];
                ctl->sheets_ptr[h]->height = h;
            }
            ctl->top -= 1;
        }
        else // 降低sheet
        {
            for (h = old; h > height; h--) // 增高 [height, old) 的图层高度
            {
                ctl->sheets_ptr[h] = ctl->sheets_ptr[h - 1];
                ctl->sheets_ptr[h]->height = h;
            }
            ctl->sheets_ptr[height] = sheet;
        }
        sheet_refresh(ctl);
    }
    else if (height > old)
    {
        if (old >= 0) // 增高sheet
        {
            for (h = old; h < height; h++) // 降低 (old, height] 的图层高度
            {
                ctl->sheets_ptr[h] = ctl->sheets_ptr[h + 1];
                ctl->sheets_ptr[h]->height = h;
            }
        }
        else // 显示sheet
        {
            ctl->top += 1;
            for (h = ctl->top; h > height; h--) // 增高 [height, ctl->top] 的图层高度
            {
                ctl->sheets_ptr[h] = ctl->sheets_ptr[h - 1];
                ctl->sheets_ptr[h]->height = h;
            }
        }
        ctl->sheets_ptr[height] = sheet;
        sheet_refresh(ctl);
    }
    return;
}

void sheet_refresh(struct SHTCTL *ctl)
{
    int h;            // 图层高度
    int buf_x, buf_y; // 相对坐标 (buf_x, buf_y)
    int vx, vy;       // 绝对坐标 (vram_x, vram_y)
    unsigned char *buf, c, *vram = ctl->vram;
    struct SHEET *sheet;
    for (h = 0; h <= ctl->top; h++) // 从 0 到 top 绘制图层
    {
        sheet = ctl->sheets_ptr[h];
        buf = sheet->buf;
        for (buf_y = 0; buf_y < sheet->bysize; buf_y++)
        {
            vy = sheet->vy0 + buf_y;
            for (buf_x = 0; buf_x < sheet->bxsize; buf_x++)
            {
                vx = sheet->vx0 + buf_x;
                c = buf[buf_y * sheet->bxsize + buf_x];
                if (c != sheet->color)
                    vram[vy * ctl->xsize + vx] = c;
            }
        }
    }
    return;
}

void sheet_slide(struct SHTCTL *ctl, struct SHEET *sht, int new_vx0, int new_vy0)
{
    sht->vx0 = new_vx0;
    sht->vy0 = new_vy0;
    if (sht->height >= 0)   /* 如果正在显示*/
        sheet_refresh(ctl); /* 按新图层的信息刷新画面 */
    return;
}

void sheet_free(struct SHTCTL *ctl, struct SHEET *sheet)
{
    if (sheet->height >= 0)
        sheet_updown(ctl, sheet, -1); // 隐藏
    sheet->flags = SHEET_FREE;        // 标志释放
    return;
}
