[BITS 32]
    MOV     AL, 'A'
    CALL    2*8:0xB59   ; far-CALL
fin:
    HLT
    JMP     fin
