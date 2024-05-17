// 启动区头文件
// 定义位置参考代码上方注释

// -------------------------------------- asmhead.nas --------------------------------------
#define ADR_BOOTINFO 0x00000ff0
struct BOOTINFO
{               /* 0x0ff0-0x0fff */
    char cyls;  /* boot sector 读到 disk 的哪里（范围） */
    char leds;  /* boot 时 keyboard 的 LED 状态 */
    char vmode; /* video mode */
    char reserve;
    short scrnx, scrny; /* 分辨率 */
    char *vram;
};

// -------------------------------------- naskfunc.nas --------------------------------------
void io_hlt(void);
void io_cli(void);
void io_sti(void);
void io_stihlt(void);
int io_in8(int port);
void io_out8(int port, int data);
int io_load_eflags(void);
void io_store_eflags(int eflags);
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);
int load_cr0(void);
void store_cr0(int cr0);
void asm_inthandler21(void);
void asm_inthandler27(void);
void asm_inthandler2c(void);
void asm_inthandler20(void);
unsigned int memtest_sub(unsigned int start, unsigned int end);

// -------------------------------------- graphic.c --------------------------------------
void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);
void init_screen8(char *vram, int x, int y);
void putfont8(char *vram, int xsize, int x, int y, char c, char *font);
void putfont_asc(char *vram, int xsize, int x, int y, char c, char *s);
void init_mouse_cursor8(char *mouse, char bc);
void putblock8(char *vram, int xsize, int x, int y, char *buf, int width, int height);
void make_window8(unsigned char *buf, int xsize, int ysize, char *title);

// 定义了一些颜色常量，用于在函数中指定颜色
#define COL8_000000 0  // 黑
#define COL8_FF0000 1  // 红
#define COL8_00FF00 2  // 绿
#define COL8_FFFF00 3  // 黄
#define COL8_0000FF 4  // 蓝
#define COL8_FF00FF 5  // 紫
#define COL8_00FFFF 6  // 青
#define COL8_FFFFFF 7  // 白
#define COL8_C6C6C6 8  // 浅灰
#define COL8_840000 9  // 暗红
#define COL8_008400 10 // 暗绿
#define COL8_848400 11 // 暗黄
#define COL8_000084 12 // 暗蓝
#define COL8_840084 13 // 暗紫色
#define COL8_008484 14 // 暗青色
#define COL8_848484 15 // 灰色

// -------------------------------------- dsctbl.c --------------------------------------
#define ADR_IDT 0x0026f800
#define LIMIT_IDT 0x000007ff
#define ADR_GDT 0x00270000
#define LIMIT_GDT 0x0000ffff
#define ADR_BOTPAK 0x00280000
#define LIMIT_BOTPAK 0x0007ffff
#define AR_DATA32_RW 0x4092
#define AR_CODE32_ER 0x409a
#define AR_INTGATE32 0x008e

struct SegmentDescriptor
{
    short limit_low, base_low;
    char base_mid, access_right;
    char limit_high, base_high;
};
struct GateDescriptor
{
    short offset_low, selector;
    char dw_count, access_right;
    short offset_high;
};
void init_gdtidt(void);
void set_segmdesc(struct SegmentDescriptor *sd, unsigned int limit, int base, int ar);
void set_gatedesc(struct GateDescriptor *gd, int offset, int selector, int ar);

// -------------------------------------- int.c --------------------------------------
#define PIC0_ICW1 0x0020
#define PIC0_OCW2 0x0020
#define PIC0_IMR 0x0021
#define PIC0_ICW2 0x0021
#define PIC0_ICW3 0x0021
#define PIC0_ICW4 0x0021
#define PIC1_ICW1 0x00a0
#define PIC1_OCW2 0x00a0
#define PIC1_IMR 0x00a1
#define PIC1_ICW2 0x00a1
#define PIC1_ICW3 0x00a1
#define PIC1_ICW4 0x00a1

void init_pic(void);
void inthandler21(int *esp);
void inthandler27(int *esp);
void inthandler2c(int *esp);

// -------------------------------------- fifo.c --------------------------------------
struct FIFO8
{
    unsigned char *buf;
    int write_ptr, read_ptr, size, free, flags;
};
void fifo8_init(struct FIFO8 *fifo, int size, unsigned char *buf);
int fifo8_put(struct FIFO8 *fifo, unsigned char data);
int fifo8_get(struct FIFO8 *fifo);
int fifo8_status(struct FIFO8 *fifo); // 返回缓冲区已使用空间大小

// -------------------------------------- keyboard.c --------------------------------------
#define PORT_KEYDAT 0x0060
#define PORT_KEYSTA 0x0064
#define PORT_KEYCMD 0x0064
#define KEYSTA_SEND_NOTREADY 0x02
#define KEYCMD_WRITE_MODE 0x60
#define KBC_MODE 0x47
extern struct FIFO8 keyfifo;

void init_keyboard(void);
void wait_KBC_sendready(void);
void inthandler21(int *esp);

// -------------------------------------- mouse.c --------------------------------------
#define KEYCMD_SENDTO_MOUSE 0xd4
#define MOUSECMD_ENABLE 0xf4
extern struct FIFO8 mousefifo;

struct MouseDescriptor
{
    unsigned char buf[3], phase;
    int x, y, button;
};

void enable_mouse(struct MouseDescriptor *mdec);
int mouse_decode(struct MouseDescriptor *mdec, unsigned char dat);
void inthandler2c(int *esp);

// -------------------------------------- memory.c --------------------------------------
#define EFLAGS_AC_BIT 0x00040000
#define CR0_CACHE_DISABLE 0x60000000
#define MEMMAN_FREES 4090 /* 大约是32KB*/
#define MEMMAN_ADDR 0x003c0000

struct FreeInfo
{ /* 可用信息 */
    unsigned int addr, size;
};

struct MemoryManager
{ /* 内存管理 */
    int frees, maxfrees, lostsize, losts;
    struct FreeInfo free[MEMMAN_FREES];
};

unsigned int mem_test(unsigned int start, unsigned int end);
void memman_init(struct MemoryManager *manager);
unsigned int memman_available(struct MemoryManager *manager);
unsigned int memman_alloc(struct MemoryManager *manager, unsigned int size);
int memman_free(struct MemoryManager *manager, unsigned int addr, unsigned int size);
unsigned int memman_alloc_4k(struct MemoryManager *manager, unsigned int size);
int memman_free_4k(struct MemoryManager *manager, unsigned int addr, unsigned int size);

// -------------------------------------- sheet.c --------------------------------------
#define MAX_SHEETS 256
#define SHEET_FREE 0
#define SHEET_USED 1

struct Sheet
{
    unsigned char *buf;
    int bxsize, bysize, vx0, vy0, color, layer, flags;
    struct SheetController *controller;
};

struct SheetController
{
    unsigned char *vram, *map;
    int xsize, ysize, top;
    struct Sheet *sheets_ptr[MAX_SHEETS];
    struct Sheet sheets_data[MAX_SHEETS];
};

struct SheetController *shtctl_init(struct MemoryManager *manager, unsigned char *vram, int xsize, int ysize);
struct Sheet *sheet_alloc(struct SheetController *ctl);
void sheet_set_buf(struct Sheet *sheet, unsigned char *buf, int xsize, int ysize, int color);
void sheet_set_layer(struct Sheet *sheet, int layer);
void sheet_refresh(struct Sheet *sheet, int bx0, int by0, int bx1, int by1);
void sheet_slide(struct Sheet *sheet, int new_vx0, int new_vy0);
void sheet_free(struct Sheet *sheet);
void sheet_refresh_sub(struct SheetController *controller, int vx0, int vy0, int vx1, int vy1, int h0, int h1);

// -------------------------------------- timer.c --------------------------------------
#define PIT_CTRL 0x0043
#define PIT_CNT0 0x0040
#define MAX_TIMER 500
#define TIMER_FLAGS_FREE 0  /* 未配置状态 */
#define TIMER_FLAGS_ALLOC 1 /* 已配置状态 */
#define TIMER_FLAGS_USING 2 /* 定时器运行中 */

struct TIMER
{
    unsigned int timeout, flags;
    struct FIFO8 *fifo;
    unsigned char data;
};

struct TIMERCTL
{
    unsigned int count;
    struct TIMER timer[MAX_TIMER];
};

extern struct TIMERCTL timerctl;

void init_pit(void);
struct TIMER *timer_alloc(void);
void timer_free(struct TIMER *timer);
void timer_init(struct TIMER *timer, struct FIFO8 *fifo, unsigned char data);
void timer_settime(struct TIMER *timer, unsigned int timeout);
void inthandler20(int *esp);
