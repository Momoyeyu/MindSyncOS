; naskfunc
; TAB=4

[FORMAT "WCOFF"]                ; 设置对象文件的模式为WCOFF
[INSTRSET "i486p"]              ; 指定可以使用的指令集，直到486处理器的指令
[BITS 32]                      ; 指定生成32位模式下的机器代码
[FILE "naskfunc.nas"]          ; 提供源文件名信息

        GLOBAL  _io_hlt, _io_cli, _io_sti, _io_stihlt
        GLOBAL  _io_in8,  _io_in16,  _io_in32
        GLOBAL  _io_out8, _io_out16, _io_out32
        GLOBAL  _io_load_eflags, _io_store_eflags
        GLOBAL  _load_gdtr, _load_idtr

[SECTION .text]

_io_hlt:    ; void io_hlt(void);
        HLT             ; 暂停CPU直到下一个中断发生
        RET

_io_cli:    ; void io_cli(void);
        CLI             ; 清除中断允许标志位，禁止中断
        RET

_io_sti:    ; void io_sti(void);
        STI             ; 设置中断允许标志位，允许中断
        RET

_io_stihlt: ; void io_stihlt(void);
        STI             ; 设置中断允许标志位，允许中断
        HLT             ; 暂停CPU直到下一个中断发生
        RET

_io_in8:    ; int io_in8(int port);
        MOV        EDX,[ESP+4]      ; 加载端口号到EDX寄存器
        MOV        EAX,0             ; 清除EAX寄存器
        IN         AL,DX            ; 从端口EDX读取8位数据到AL寄存器
        RET

_io_in16:   ; int io_in16(int port);
        MOV        EDX,[ESP+4]      ; 加载端口号到EDX寄存器
        MOV        EAX,0             ; 清除EAX寄存器
        IN         AX,DX            ; 从端口EDX读取16位数据到AX寄存器
        RET

_io_in32:   ; int io_in32(int port);
        MOV        EDX,[ESP+4]      ; 加载端口号到EDX寄存器
        IN         EAX,DX           ; 从端口EDX读取32位数据到EAX寄存器
        RET

_io_out8:   ; void io_out8(int port, int data);
        MOV        EDX,[ESP+4]      ; 加载端口号到EDX寄存器
        MOV        AL,[ESP+8]       ; 加载要发送的数据到AL寄存器
        OUT        DX,AL            ; 将AL寄存器的8位数据发送到端口EDX
        RET

_io_out16:  ; void io_out16(int port, int data);
        MOV        EDX,[ESP+4]      ; 加载端口号到EDX寄存器
        MOV        EAX,[ESP+8]      ; 加载要发送的数据到EAX寄存器
        OUT        DX,AX            ; 将AX寄存器的16位数据发送到端口EDX
        RET

_io_out32:  ; void io_out32(int port, int data);
        MOV        EDX,[ESP+4]      ; 加载端口号到EDX寄存器
        MOV        EAX,[ESP+8]      ; 加载要发送的数据到EAX寄存器
        OUT        DX,EAX           ; 将EAX寄存器的32位数据发送到端口EDX
        RET

_io_load_eflags: ; int io_load_eflags(void);
        PUSHFD                  ; 将EFLAGS寄存器的值压栈
        POP        EAX           ; 从栈中弹栈到EAX寄存器
        RET

_io_store_eflags: ; void io_store_eflags(int eflags);
        MOV        EAX,[ESP+4]     ; 加载eflags参数到EAX寄存器
        PUSH       EAX            ; 将EAX寄存器的值压栈
        POPFD                   ; 从栈中弹栈到EFLAGS寄存器
        RET

_load_gdtr:		; void load_gdtr(int limit, int addr);
		MOV		AX,[ESP+4]		; limit
		MOV		[ESP+6],AX
		LGDT	[ESP+6]
		RET

_load_idtr:		; void load_idtr(int limit, int addr);
		MOV		AX,[ESP+4]		; limit
		MOV		[ESP+6],AX
		LIDT	[ESP+6]
		RET
