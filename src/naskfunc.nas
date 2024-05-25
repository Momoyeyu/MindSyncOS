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
        GLOBAL	_asm_inthandler21, _asm_inthandler27, _asm_inthandler2c, _asm_inthandler20
		EXTERN	_inthandler21, _inthandler27, _inthandler2c, _inthandler20
		GLOBAL	_load_cr0, _store_cr0, _memtest_sub, _load_tr
		GLOBAL	_taskswitch4, _taskswitch3, _farjmp

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

_asm_inthandler21:
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX,ESP
		PUSH	EAX
		MOV		AX,SS
		MOV		DS,AX
		MOV		ES,AX
		CALL	_inthandler21
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		IRETD

_asm_inthandler27:
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX,ESP
		PUSH	EAX
		MOV		AX,SS
		MOV		DS,AX
		MOV		ES,AX
		CALL	_inthandler27
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		IRETD

_asm_inthandler2c:
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX,ESP
		PUSH	EAX
		MOV		AX,SS
		MOV		DS,AX
		MOV		ES,AX
		CALL	_inthandler2c
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		IRETD

_load_cr0: ; int load_cr0(void);
		MOV EAX,CR0
		RET

_store_cr0: ; void store_cr0(int cr0);
		MOV EAX,[ESP+4]
		MOV CR0,EAX
		RET

_memtest_sub: ; unsigned int memtest_sub(unsigned int start, unsigned int end);
		PUSH 	EDI 					; 由于还要使用EBX, ESI, EDI，先压入栈
		PUSH 	ESI 					; ESP - 12
		PUSH 	EBX	
		MOV 	ESI,0xaa55aa55 			; pat0 = 0xaa55aa55;
		MOV 	EDI,0x55aa55aa 			; pat1 = 0x55aa55aa;
		MOV 	EAX,[ESP+12+4] 			; i = start;
mts_loop:
		MOV 	EBX,EAX
		ADD 	EBX,0xffc
		MOV 	EDX,[EBX]
		MOV 	[EBX],ESI
		XOR 	DWORD [EBX],0xffffffff
		CMP		EDI,[EBX]
		JNE		mts_fin
		XOR 	DWORD [EBX],0xffffffff
		CMP 	ESI,[EBX] 
		JNE		mts_fin
		MOV		[EBX],EDX
		ADD		EAX,0x1000
		CMP		EAX,[ESP + 12 + 8] 		; [ESP + 12 + 8] 是 end 在栈中的地址
		JBE		mts_loop
		POP		EBX
		POP		ESI
		POP		EDI
		RET
mts_fin:
		MOV		[EBX],EDX
		POP		EBX
		POP		ESI
		POP		EDI
		RET

_asm_inthandler20:
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX,ESP
		PUSH	EAX
		MOV		AX,SS
		MOV		DS,AX
		MOV		ES,AX
		CALL	_inthandler20
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		IRETD

_load_tr: ; void load_tr(int tr);
		LTR 	[ESP+4] ; tr
		RET

_taskswitch4: ; void taskswitch4(void);
		JMP 	4*8:0
		RET

_taskswitch3: ; void taskswitch3(void);
		JMP 	3*8:0
		RET

_farjmp: ; void farjmp(int eip, int cs);
		JMP 	FAR [ESP+4] ; eip 在 [ESP+4], cs 在 [ESP+8]
		RET

