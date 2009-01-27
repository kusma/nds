	PRESERVE8
	AREA iwram, CODE, READONLY
	
	IMPORT vblank
	EXPORT gba_irq_handler

REG_BASE EQU 0x4000000
REG_IE   EQU 0x200

gba_irq_handler
	push {r5-r12,lr}
	mov  r2, #REG_BASE        ; get I/O base address
	ldr  r1, [r2, #REG_IE]    ; read IE and IF
	and  r1, r1, r1, lsr #16  ; isolate occurred AND enabled irqs,
	
	ldrh r0, [r2, #-8]  ; mix up with BIOS flags at 0x3007FF8 (mirrored at 0x3FFFFF8)
	orr  r0, r0, r1     ;
	strh r0, [r2, #-8]  ;
	
	add  r3, r2, #0x200  ; try to reach REG_IF
	strh r0, [r3, #2]    ; ACK interrupts
	
	tst  r0, #1  ; check for blank interrupt
	blne  vblank  ; exec
	
	pop {r5-r12,lr}
	bx  lr      ; return to BIOS
	END
