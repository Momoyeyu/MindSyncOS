// FIFO related

#include "bootpack.h"

void fifo8_init(struct FIFO8 *fifo, int size, unsigned char *buf)
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

int fifo8_put(struct FIFO8 *fifo, unsigned char data)
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

int fifo8_get(struct FIFO8 *fifo)
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
int fifo8_status(struct FIFO8 *fifo)
{
    return fifo->size - fifo->free;
}