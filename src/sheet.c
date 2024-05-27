// 图层相关

#include "bootpack.h"
#include <stdio.h>

struct SHTCTL *shtctl_init(struct MEMMAN *manager, unsigned char *vram, int xsize, int ysize)
{
    struct SHTCTL *controller;
    int i;
    controller = (struct SHTCTL *)memman_alloc_4k(manager, sizeof(struct SHTCTL));
    if (controller == NULL)
        return NULL;
    controller->map = (unsigned char *)memman_alloc_4k(manager, xsize * ysize);
    if (controller->map == NULL)
    {
        memman_free_4k(manager, (int)controller, sizeof(struct SHTCTL));
        return NULL;
    }
    controller->vram = vram;
    controller->xsize = xsize;
    controller->ysize = ysize;
    controller->top = -1; // 暂时没有任何sheet
    for (i = 0; i < MAX_SHEETS; i++)
    {
        controller->sheets_data[i].flags = SHEET_FREE; // 标记为未使用
        controller->sheets_data[i].controller = controller;
    }
    return controller;
}

struct SHEET *sheet_alloc(struct SHTCTL *controller)
{
    struct SHEET *sheet;
    int i;
    for (i = 0; i < MAX_SHEETS; i++)
    {
        if (controller->sheets_data[i].flags == 0)
        {
            sheet = &(controller->sheets_data[i]);
            sheet->flags = SHEET_USED; // 标记为已使用
            sheet->layer = -1;         // 隐藏
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

void sheet_updown(struct SHEET *sheet, int layer)
{
    struct SHTCTL *controller = sheet->controller;
    int old = sheet->layer; /* 存储设置前的图层信息 */
    /* 如果指定的图层过高或过低，则进行修正 */
    if (layer > controller->top + 1)
        layer = controller->top + 1;
    else if (layer < -1)
        layer = -1;
    sheet->layer = layer;

    // 重新排列
    int l;
    if (layer < old)
    {
        if (layer < 0) // 隐藏sheet
        {
            for (l = old; l < controller->top; l++) // 降低 (old, controller->top] 的图层
            {
                controller->sheets_ptr[l] = controller->sheets_ptr[l + 1];
                controller->sheets_ptr[l]->layer = l;
            }
            controller->top -= 1;
            sheet_refresh_map(controller, sheet->vx0, sheet->vy0, sheet->vx0 + sheet->bxsize, sheet->vy0 + sheet->bysize, 0);
            sheet_refresh_sub(controller, sheet->vx0, sheet->vy0, sheet->vx0 + sheet->bxsize, sheet->vy0 + sheet->bysize, 0, old - 1);
        }
        else // 降低sheet
        {
            for (l = old; l > layer; l--) // 增高 [layer, old) 的图层
            {
                controller->sheets_ptr[l] = controller->sheets_ptr[l - 1];
                controller->sheets_ptr[l]->layer = l;
            }
            controller->sheets_ptr[layer] = sheet;
            sheet_refresh_map(controller, sheet->vx0, sheet->vy0, sheet->vx0 + sheet->bxsize, sheet->vy0 + sheet->bysize, layer + 1);
            sheet_refresh_sub(controller, sheet->vx0, sheet->vy0, sheet->vx0 + sheet->bxsize, sheet->vy0 + sheet->bysize, layer + 1, old);
        }
    }
    else if (layer > old)
    {
        if (old >= 0) // 增高sheet
        {
            for (l = old; l < layer; l++) // 降低 (old, layer] 的图层高度
            {
                controller->sheets_ptr[l] = controller->sheets_ptr[l + 1];
                controller->sheets_ptr[l]->layer = l;
            }
        }
        else // 显示sheet
        {
            controller->top += 1;
            for (l = controller->top; l > layer; l--) // 增高 [layer, controller->top] 的图层高度
            {
                controller->sheets_ptr[l] = controller->sheets_ptr[l - 1];
                controller->sheets_ptr[l]->layer = l;
            }
        }
        controller->sheets_ptr[layer] = sheet;
        sheet_refresh_map(controller, sheet->vx0, sheet->vy0, sheet->vx0 + sheet->bxsize, sheet->vy0 + sheet->bysize, layer);
        sheet_refresh_sub(controller, sheet->vx0, sheet->vy0, sheet->vx0 + sheet->bxsize, sheet->vy0 + sheet->bysize, layer, layer);
    }
    return;
}

void sheet_refresh(struct SHEET *sheet, int bx0, int by0, int bx1, int by1)
{
    if (sheet->layer >= 0)
        sheet_refresh_sub(sheet->controller, sheet->vx0 + bx0, sheet->vy0 + by0, sheet->vx0 + bx1, sheet->vy0 + by1, sheet->layer, sheet->layer);
    return;
}

void sheet_slide(struct SHEET *sheet, int new_vx0, int new_vy0)
{
    struct SHTCTL *controller = sheet->controller;
    int old_x = sheet->vx0;
    int old_y = sheet->vy0;
    sheet->vx0 = new_vx0;
    sheet->vy0 = new_vy0;
    /* 如果正在显示*/
    if (sheet->layer >= 0)
    { // 采用局部更新
        sheet_refresh_map(controller, old_x, old_y, old_x + sheet->bxsize, old_y + sheet->bysize, 0);
        sheet_refresh_map(controller, sheet->vx0, sheet->vy0, sheet->vx0 + sheet->bxsize, sheet->vy0 + sheet->bysize, sheet->layer);
        sheet_refresh_sub(controller, old_x, old_y, old_x + sheet->bxsize, old_y + sheet->bysize, 0, sheet->layer - 1);
        sheet_refresh_sub(controller, sheet->vx0, sheet->vy0, sheet->vx0 + sheet->bxsize, sheet->vy0 + sheet->bysize, sheet->layer, sheet->layer);
    }
    return;
}

void sheet_free(struct SHEET *sheet)
{
    if (sheet->layer >= 0)
        sheet_updown(sheet, -1); // 隐藏
    sheet->flags = SHEET_FREE;   // 标志释放
    return;
}

// 只更新 [(vx0, vy0), (vx1, vy1)) 内的显存
void sheet_refresh_sub(struct SHTCTL *controller, int vx0, int vy0, int vx1, int vy1, int h0, int h1)
{
    int h, bx, by, vx, vy;
    int bx0, bx1, by0, by1;
    unsigned char *buf, c, *vram = controller->vram, *map = controller->map, sid;
    struct SHEET *sheet;
    if (vx0 < 0)
        vx0 = 0;
    if (vy0 < 0)
        vy0 = 0;
    if (vx1 > controller->xsize)
        vx1 = controller->xsize;
    if (vy1 > controller->ysize)
        vy1 = controller->ysize;
    if (h0 < 0)
        h0 = 0;
    if (h1 > controller->top)
        h1 = controller->top;
    for (h = h0; h <= h1; h++)
    {
        sheet = controller->sheets_ptr[h];
        buf = sheet->buf;
        sid = sheet - controller->sheets_data; // 重要！
        // 缩小搜索范围，减少if次数
        bx0 = vx0 - sheet->vx0;
        bx1 = vx1 - sheet->vx0;
        by0 = vy0 - sheet->vy0;
        by1 = vy1 - sheet->vy0;
        if (bx0 < 0)
            bx0 = 0;
        if (bx1 > sheet->bxsize)
            bx1 = sheet->bxsize;
        if (by0 < 0)
            by0 = 0;
        if (by1 > sheet->bysize)
            by1 = sheet->bysize;
        for (by = by0; by < by1; by++)
        {
            vy = sheet->vy0 + by;
            for (bx = bx0; bx < bx1; bx++)
            {
                vx = sheet->vx0 + bx;
                if (sid == map[vy * controller->xsize + vx]) // 减小访存次数
                    vram[vy * controller->xsize + vx] = buf[by * sheet->bxsize + bx];
            }
        }
    }
    return;
}

void sheet_refresh_map(struct SHTCTL *controller, int vx0, int vy0, int vx1, int vy1, int h0)
{
    int h, bx, by, vx, vy, bx0, by0, bx1, by1;
    unsigned char *buf, sid, *map = controller->map;
    struct SHEET *sht;
    if (vx0 < 0)
        vx0 = 0;
    if (vy0 < 0)
        vy0 = 0;
    if (vx1 > controller->xsize)
        vx1 = controller->xsize;
    if (vy1 > controller->ysize)
        vy1 = controller->ysize;
    if (h0 < 0)
        h0 = 0;
    for (h = h0; h <= controller->top; h++)
    {
        sht = controller->sheets_ptr[h];
        sid = sht - controller->sheets_data; /* 将进行了减法计算的地址作为图层号码使用 */
        buf = sht->buf;
        bx0 = vx0 - sht->vx0;
        by0 = vy0 - sht->vy0;
        bx1 = vx1 - sht->vx0;
        by1 = vy1 - sht->vy0;
        if (bx0 < 0)
            bx0 = 0;
        if (by0 < 0)
            by0 = 0;
        if (bx1 > sht->bxsize)
            bx1 = sht->bxsize;
        if (by1 > sht->bysize)
            by1 = sht->bysize;
        for (by = by0; by < by1; by++)
        {
            vy = sht->vy0 + by;
            for (bx = bx0; bx < bx1; bx++)
            {
                vx = sht->vx0 + bx;
                if (buf[by * sht->bxsize + bx] != sht->color)
                    map[vy * controller->xsize + vx] = sid;
            }
        }
    }
    return;
}
