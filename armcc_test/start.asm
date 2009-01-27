;	start.asm for the GBA
;	Link this with C code to make it run on the GBA.
;	Most of it has been stolen from ARM docs, the rest is by me ;)
;	Eloist (eloist@gmx.de)
;	Hacked some more by Erik "kusma" Faye-Lund (kusmabite@gmail.com)
	
	PRESERVE8
	AREA my_1st_start, CODE, READONLY
	
	IMPORT |Image$$RO$$Base|  ; End of ROM code (=start of ROM data)
	IMPORT |Image$$RO$$Limit| ; End of ROM code (=start of ROM data)
	
;	IMPORT |Image$$RW$$Base|  ; Base of RAM to initialize
	
	IMPORT |Image$$ZI$$Base|  ; Base and limit of area
	IMPORT |Image$$ZI$$Limit| ; to zero initialize
	
	IMPORT |Image$$IWRAM$$Base|
	IMPORT |Image$$IWRAM$$Limit|
	IMPORT |Load$$IWRAM$$Base|
	
	IMPORT gba_main
	
Cpu_Mode_USR EQU 0x10
Cpu_Mode_IRQ EQU 0x12
Cpu_Mode_SVC EQU 0x13

Cpu_Stack_USR EQU 0x03007F00 ; GBA USR stack adress
Cpu_Stack_IRQ EQU 0x03007FA0 ; GBA IRQ stack adress
Cpu_Stack_SVC EQU 0x03007FE0 ; GBA SVC stack adress
	
	ENTRY
	b       start
	DCD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	DCD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	DCD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
start
	mov     r0, #Cpu_Mode_IRQ:OR:0x80:OR:0x40 ; Disable interrupts
	msr     CPSR_c, r0                        ; Enable IRQ mode
	ldr     r13, =Cpu_Stack_IRQ               ; Setup IRQ stack pointer
	
	mov     r0, #Cpu_Mode_SVC:OR:0x80:OR:0x40 ; Disable interrupts
	msr     CPSR_c, r0                        ; Enable SVC mode
	ldr     r13, =Cpu_Stack_SVC               ; Setup SVC stack pointer
	
	ldr     r0, =|Load$$IWRAM$$Base|   ; source
	ldr     r1, =|Image$$IWRAM$$Base|  ; destination
	ldr     r2, =|Image$$IWRAM$$Limit| ; destination + length
	
	subs    r2, r2, r1
	beq     nocopy_iwram
iwram_loop
	ldr     r3, [r0], #4
	str     r3, [r1], #4
	subs    r2, #4
	bne     iwram_loop
nocopy_iwram
	
	
;	ldr     r0, =|Image$$RO$$Limit|           ; Get pointer to ROM data
;	ldr     r0, =0x08000000
;	ldr     r1, =|Image$$RW$$Base|            ; and RAM copy
	ldr     r3, =|Image$$ZI$$Base|            ; Zero init base => top of initialized data
;	cmp     r0, r1                            ; Check that they are different
;	beq     %F1
0
;	cmp     r1, r3                            ; Copy init data
;	ldrcc   r2, [r0], #4
;	strcc   r2, [r1], #4
;	bcc     %B0
1
	ldr     r1, =|Image$$ZI$$Limit|           ; Top of zero init segment
	mov     r2, #0
2
	cmp     r3, r1                            ; Zero init
	strcc   r2, [r3], #4
	bcc     %B2
	
	mov     r0, #Cpu_Mode_SVC:OR:0x40         ; Only IRQ enabled
	msr     CPSR_c, r0
	
	mov     r0, #Cpu_Mode_USR:OR:0x40
	msr     CPSR_c, r0                        ; Switch to USR mode
	ldr     sp, =Cpu_Stack_USR                ; Setup USR stack pointer
	
	mov     r0, #0 ; argc
	mov     r1, #0 ; argv
	bl      gba_main   ; branch+link to start of C code
end
	b end          ; stall on return
	
	END
