// FIFO related

#include "bootpack.h"

void fifo32_init(struct FIFO32 *fifo, int size, unsigned char *buf)
{
    fifo->size = size;
    fifo->buf = buf;
    fifo->free = size;
    fifo->flags = 0;
    fifo->p = 0;
    fifo->q = 0;
    return;
}

#define FLAGS_OVERRUN 0x0001

int fifo32_put(struct FIFO32 *fifo, unsigned char data)
{
    if (fifo->free == 0)
    {
        fifo->flags |= FLAGS_OVERRUN;
        return -1;
    }
    fifo->buf[fifo->p] = data;
    fifo->free -= 1;
    fifo->p += 1;
    if (fifo->p == fifo->size)
    {
        fifo->p = 0;
    }
    return 0;
}

int fifo32_get(struct FIFO32 *fifo)
{
    int data;
    if (fifo->free == fifo->size)
    {
        return -1;
    }
    data = fifo->buf[fifo->q];
    fifo->q += 1;
    fifo->free += 1;
    if (fifo->q == fifo->size)
    {
        fifo->q = 0;
    }
    return data;
}

// 返回缓冲区已使用空间大小
int fifo32_status(struct FIFO32 *fifo)
{
    return fifo->size - fifo->free;
}