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
void load_tr(int tr);
void asm_inthandler20(void);
void asm_inthandler21(void);
void asm_inthandler27(void);
void asm_inthandler2c(void);
unsigned int memtest_sub(unsigned int start, unsigned int end);
void farjmp(int eip, int cs);

// -------------------------------------- memory.c --------------------------------------
#define EFLAGS_AC_BIT 0x00040000
#define CR0_CACHE_DISABLE 0x60000000
#define MEMMAN_FREES 4090 /* 大约是32KB*/
#define MEMMAN_ADDR 0x003c0000

struct FREEINFO
{ /* 可用信息 */
    unsigned int addr, size;
};

struct MEMMAN
{ /* 内存管理 */
    int frees, maxfrees, lostsize, losts;
    struct FREEINFO free[MEMMAN_FREES];
};

unsigned int memtest(unsigned int start, unsigned int end);
void memman_init(struct MEMMAN *manager);
unsigned int memman_total(struct MEMMAN *manager);
unsigned int memman_alloc(struct MEMMAN *manager, unsigned int size);
int memman_free(struct MEMMAN *manager, unsigned int addr, unsigned int size);
unsigned int memman_alloc_4k(struct MEMMAN *manager, unsigned int size);
int memman_free_4k(struct MEMMAN *manager, unsigned int addr, unsigned int size);

// -------------------------------------- sheet.c --------------------------------------
#define MAX_SHEETS 256
#define SHEET_FREE 0
#define SHEET_USED 1

struct SHEET
{
    unsigned char *buf;
    int bxsize, bysize, vx0, vy0, color, layer, flags;
    struct SHTCTL *controller;
};

struct SHTCTL
{
    unsigned char *vram, *map;
    int xsize, ysize, top;
    struct SHEET *sheets_ptr[MAX_SHEETS];
    struct SHEET sheets_data[MAX_SHEETS];
};

struct SHTCTL *shtctl_init(struct MEMMAN *manager, unsigned char *vram, int xsize, int ysize);
struct SHEET *sheet_alloc(struct SHTCTL *ctl);
void sheet_setbuf(struct SHEET *sheet, unsigned char *buf, int xsize, int ysize, int color);
void sheet_updown(struct SHEET *sheet, int layer);
void sheet_refresh(struct SHEET *sheet, int bx0, int by0, int bx1, int by1);
void sheet_slide(struct SHEET *sheet, int new_vx0, int new_vy0);
void sheet_free(struct SHEET *sheet);

// -------------------------------------- graphic.c --------------------------------------
void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);
void init_screen8(char *vram, int x, int y);
void putfont8(char *vram, int xsize, int x, int y, char c, char *font);
void putfonts8_asc(char *vram, int xsize, int x, int y, char c, char *s);
void init_mouse_cursor8(char *mouse, char bc);
void putblock8(char *vram, int xsize, int x, int y, char *buf, int width, int height);
void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act);
void putfonts8_asc_sht(struct SHEET *sht, int x0, int y0, int c, int bc, char *s, int l);

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
#define AR_TSS32 0x0089

struct SEGMENT_DESCRIPTOR
{
    short limit_low, base_low;
    char base_mid, access_right;
    char limit_high, base_high;
};
struct GATE_DESCRIPTOR
{
    short offset_low, selector;
    char dw_count, access_right;
    short offset_high;
};
void init_gdtidt(void);
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar);

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
struct FIFO32
{
    int *buf;
    int p, q, size, free, flags;
    struct TASK *task;
};
void fifo32_init(struct FIFO32 *fifo, int size, int *buf, struct TASK *task);
int fifo32_put(struct FIFO32 *fifo, int data);
int fifo32_get(struct FIFO32 *fifo);
int fifo32_status(struct FIFO32 *fifo); // 返回缓冲区已使用空间大小

// -------------------------------------- keyboard.c --------------------------------------
#define PORT_KEYDAT 0x0060
#define PORT_KEYSTA 0x0064
#define PORT_KEYCMD 0x0064
#define KEYSTA_SEND_NOTREADY 0x02
#define KEYCMD_WRITE_MODE 0x60
#define KBC_MODE 0x47

void init_keyboard(struct FIFO32 *fifo, int data0);
void wait_KBC_sendready(void);
void inthandler21(int *esp);

// -------------------------------------- mouse.c --------------------------------------
#define KEYCMD_SENDTO_MOUSE 0xd4
#define MOUSECMD_ENABLE 0xf4

struct MOUSE_DEC
{
    unsigned char buf[3], phase;
    int x, y, btn;
};

void enable_mouse(struct FIFO32 *fifo, int data0, struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat);
void inthandler2c(int *esp);

// -------------------------------------- timer.c --------------------------------------
#define PIT_CTRL 0x0043
#define PIT_CNT0 0x0040
#define MAX_TIMER 500
#define TIMER_FLAGS_FREE 0  /* 未配置状态 */
#define TIMER_FLAGS_ALLOC 1 /* 已配置状态 */
#define TIMER_FLAGS_USING 2 /* 定时器运行中 */

struct TIMER
{
    struct TIMER *next;
    unsigned int timeout, flags;
    struct FIFO32 *fifo;
    int data;
};

struct TIMERCTL
{
    unsigned int count, next;
    struct TIMER *t0;
    struct TIMER timers0[MAX_TIMER];
};

extern struct TIMERCTL timerctl;

void init_pit(void);
struct TIMER *timer_alloc(void);
void timer_free(struct TIMER *timer);
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data);
void timer_settime(struct TIMER *timer, unsigned int timeout);
void inthandler20(int *esp);

// -------------------------------------- mtask.c --------------------------------------
#define MAX_TASKS 1000 // 最大任务数量
#define TASK_GDT0 3    // 定义从GDT的几号开始分配给TSS
#define MAX_TASKS_LV 100
#define MAX_TASKLEVELS 10
struct TSS32
{
    int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;
    int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
    int es, cs, ss, ds, fs, gs;
    int ldtr, iomap;
};
struct TASK
{
    int sel, flags; // sel用来存放GDT的编号
    int level, priority;
    struct TSS32 tss;
};

struct TASKLEVEL
{
    int running; /*正在运行的任务数量*/
    int now;     /*这个变量用来记录当前正在运行的是哪个任务*/
    struct TASK *tasks[MAX_TASKS_LV];
};
struct TASKCTL
{
    int now_lv;     /*现在活动中的LEVEL */
    char lv_change; /*在下次任务切换时是否需要改变LEVEL */
    struct TASKLEVEL level[MAX_TASKLEVELS];
    struct TASK tasks0[MAX_TASKS];
};
extern struct TASKCTL *taskctl;
extern struct TIMER *task_timer;

struct TASK *task_init(struct MEMMAN *memman);
struct TASK *task_alloc(void);
void task_run(struct TASK *task, int level, int priority);
void task_switch(void);
void task_sleep(struct TASK *task);
struct TASK *task_now(void);
void task_add(struct TASK *task);
void task_remove(struct TASK *task);
void task_idle(void);
