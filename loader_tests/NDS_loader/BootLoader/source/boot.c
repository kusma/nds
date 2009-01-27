/*-----------------------------------------------------------------
 boot.c
 
 BootLoader
 Loads a file into memory and runs it

 All resetMemory and startBinary functions are based 
 on the MultiNDS loader by Darkain.
 Original source available at:
 http://cvs.sourceforge.net/viewcvs.py/ndslib/ndslib/examples/loader/boot/main.cpp

License:
 Copyright (C) 2005  Michael "Chishm" Chisholm

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

 If you use this code, please give due credit and email me about your
 project at chishm@hotmail.com
 
Helpful information:
 This code runs from the first Shared IWRAM bank:
 0x037F8000 to 0x037FC000
------------------------------------------------------------------*/

#include <nds/jtypes.h>
#include <nds/dma.h>
#include <nds/system.h>
#include <nds/interrupts.h>
#include <nds/timers.h>
#define ARM9
#undef ARM7
#include <nds/memory.h>
#include <nds/arm9/video.h>
#include <nds/arm9/input.h>
#undef ARM9
#define ARM7
#include <nds/arm7/audio.h>

#include "fat.h"
#include "dldi_patcher.h"

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Important things
#define TEMP_MEM 0x027FE000
#define NDS_HEAD 0x027FFE00
#define TEMP_ARM9_START_ADDRESS (*(vu32*)0x027FFFF4)


//#define STORED_FILE_CLUSTER (*(vu32*)0x027FFFFC)

#define ARM9_START_FLAG (*(vu8*)0x027FFDFF)
const char* bootName = "_BOOT_DS.NDS";

extern unsigned long storedFileCluster;
extern unsigned long initDisc;
extern unsigned long wantToPatchDLDI;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Firmware stuff

#define FW_READ        0x03

void boot_readFirmware (uint32 address, uint8 * buffer, uint32 size) {
  uint32 index;

  // Read command
  while (REG_SPICNT & SPI_BUSY);
  REG_SPICNT = SPI_ENABLE | SPI_CONTINUOUS | SPI_DEVICE_NVRAM;
  REG_SPIDATA = FW_READ;
  while (REG_SPICNT & SPI_BUSY);

  // Set the address
  REG_SPIDATA =  (address>>16) & 0xFF;
  while (REG_SPICNT & SPI_BUSY);
  REG_SPIDATA =  (address>>8) & 0xFF;
  while (REG_SPICNT & SPI_BUSY);
  REG_SPIDATA =  (address) & 0xFF;
  while (REG_SPICNT & SPI_BUSY);

  for (index = 0; index < size; index++) {
    REG_SPIDATA = 0;
    while (REG_SPICNT & SPI_BUSY);
    buffer[index] = REG_SPIDATA & 0xFF;
  }
  REG_SPICNT = 0;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Loader functions

void CpuFastSet (u32 src, u32 dest, u32 ctrl)
{
	__asm volatile ("swi 0x0C0000\n");
}

/*
static inline void dmaFillWords(const void* src, void* dest, uint32 size) {
	DMA_SRC(3)  = (uint32)src;
	DMA_DEST(3) = (uint32)dest;
	DMA_CR(3)   = DMA_COPY_WORDS | DMA_SRC_FIX | (size>>2);
	while(DMA_CR(3) & DMA_BUSY);
} */

static inline void copyLoop (u32* dest, const u32* src, u32 size) {
	do {
		*dest++ = *src++;
	} while (size -= 4);
}

#define resetCpu() \
		__asm volatile("swi 0x000000")

/*-------------------------------------------------------------------------
resetMemory1_ARM9
Clears the ARM9's icahce and dcache
Written by Darkain.
Modified by Chishm:
 Changed ldr to mov & add
 Added clobber list
--------------------------------------------------------------------------*/
#define resetMemory1_ARM9_size 0x400
void __attribute__ ((long_call)) resetMemory1_ARM9 (void) 
{
	REG_IME = 0;
	REG_IE = 0;
	REG_IF = ~0;

  __asm volatile(
    //clean and flush cache
    "mov r1, #0                   \n"
    "outer_loop:                  \n"
    " mov r0, #0                  \n"
    " inner_loop:                 \n"
    "  orr r2, r1, r0             \n"
    "  mcr p15, 0, r2, c7, c14, 2 \n"
    "  add r0, r0, #0x20          \n"
    "  cmp r0, #0x400             \n"
    " bne inner_loop              \n"
    " add r1, r1, #0x40000000     \n"
    " cmp r1, #0x0                \n"
    "bne outer_loop               \n"
  
    "mov r10, #0                  \n"
    "mcr p15, 0, r10, c7, c5, 0   \n"  //Flush ICache
    "mcr p15, 0, r10, c7, c6, 0   \n"  //Flush DCache
    "mcr p15, 0, r10, c7, c10, 4  \n"  //empty write buffer

	"mcr p15, 0, r10, c3, c0, 0   \n"  //disable write buffer       (def = 0)

    "mcr p15, 0, r10, c2, c0, 0   \n"  //disable DTCM and protection unit

    "mcr p15, 0, r10, c6, c0, 0   \n"  //disable protection unit 0  (def = 0)
    "mcr p15, 0, r10, c6, c1, 0   \n"  //disable protection unit 1  (def = 0)
    "mcr p15, 0, r10, c6, c2, 0   \n"  //disable protection unit 2  (def = 0)
    "mcr p15, 0, r10, c6, c3, 0   \n"  //disable protection unit 3  (def = 0)
    "mcr p15, 0, r10, c6, c4, 0   \n"  //disable protection unit 4  (def = ?)
    "mcr p15, 0, r10, c6, c5, 0   \n"  //disable protection unit 5  (def = ?)
    "mcr p15, 0, r10, c6, c6, 0   \n"  //disable protection unit 6  (def = ?)
    "mcr p15, 0, r10, c6, c7, 0   \n"  //disable protection unit 7  (def = ?)

    "mcr p15, 0, r10, c5, c0, 3   \n"  //IAccess
    "mcr p15, 0, r10, c5, c0, 2   \n"  //DAccess

	//"ldr r10, =0x0080000A         \n"	// Relocated code can't load data
	"mov r10, #0x00800000		  \n"	// Use mov instead
	"add r10, r10, #0x00A		  \n"
	"mcr p15, 0, r10, c9, c1, 0   \n"  //DTCM base  (def = 0x0080000A) ???
	
	//"ldr r10, =0x0000000C         \n"	// Relocated code can't load data
	"mov r10, #0x0000000C		  \n"	// Use mov instead
	"mcr p15, 0, r10, c9, c1, 1   \n"  //ITCM base  (def = 0x0000000C) ???

    "mov r10, #0x1F               \n"
	"msr cpsr, r10                \n"
	: // no outputs
	: // no inputs
	: "r0","r1","r2","r10"			// Clobbered registers
	);

	register int i;
	
	for (i=0; i<16*1024; i+=4) {  //first 16KB
		(*(vu32*)(i+0x00000000)) = 0x00000000;      //clear ITCM
		(*(vu32*)(i+0x00800000)) = 0x00000000;      //clear DTCM
	}
	
	for (i=16*1024; i<32*1024; i+=4) {  //second 16KB
		(*(vu32*)(i+0x00000000)) = 0x00000000;      //clear ITCM
	}

	(*(vu32*)0x00803FFC) = 0;   //IRQ_HANDLER ARM9 version
	(*(vu32*)0x00803FF8) = ~0;  //VBLANK_INTR_WAIT_FLAGS ARM9 version

	// Return to loop
	*((vu32*)0x027FFE04) = (u32)0xE59FF018;		// ldr pc, 0x027FFE24
	*((vu32*)0x027FFE24) = (u32)0x027FFE04;		// Set ARM9 Loop address
	__asm volatile("bx %0" : : "r" (0x027FFE04) ); 
}

/*-------------------------------------------------------------------------
resetMemory2_ARM9
Clears the ARM9's DMA channels and resets video memory
Written by Darkain.
Modified by Chishm:
 * Changed MultiNDS specific stuff
--------------------------------------------------------------------------*/
#define resetMemory2_ARM9_size 0x400
void __attribute__ ((long_call)) resetMemory2_ARM9 (void) 
{
 	register int i;
  
	//clear out ARM9 DMA channels
	for (i=0; i<4; i++) {
		DMA_CR(i) = 0;
		DMA_SRC(i) = 0;
		DMA_DEST(i) = 0;
		TIMER_CR(i) = 0;
		TIMER_DATA(i) = 0;
	}

	VRAM_CR = (VRAM_CR & 0xffff0000) | 0x00008080 ;
	(*(vu32*)0x027FFE04) = 0;   // temporary variable
	PALETTE[0] = 0xFFFF;
	dmaFillWords((void*)0x027FFE04, PALETTE+1, (2*1024)-2);
	dmaFillWords((void*)0x027FFE04, OAM,     2*1024);
	dmaFillWords((void*)0x027FFE04, (void*)0x04000000, 0x56);  //clear main display registers
	dmaFillWords((void*)0x027FFE04, (void*)0x04001000, 0x56);  //clear sub  display registers
	dmaFillWords((void*)0x027FFE04, VRAM,  656*1024);

	
	REG_DISPSTAT = 0;
	videoSetMode(0);
	videoSetModeSub(0);
	VRAM_A_CR = 0;
	VRAM_B_CR = 0;
// Don't mess with the ARM7's VRAM
//	VRAM_C_CR = 0;
	VRAM_D_CR = 0;
	VRAM_E_CR = 0;
	VRAM_F_CR = 0;
	VRAM_G_CR = 0;
	VRAM_H_CR = 0;
	VRAM_I_CR = 0;
//	VRAM_CR   = 0x03000000;
	REG_POWERCNT  = 0x820F;

	//set shared ram to ARM7
	WRAM_CR = 0x03;

	// Return to loop
	*((vu32*)0x027FFE04) = (u32)0xE59FF018;		// ldr pc, 0x027FFE24
	*((vu32*)0x027FFE24) = (u32)0x027FFE04;		// Set ARM9 Loop address
	__asm volatile("bx %0" : : "r" (0x027FFE04) ); 
}


/*-------------------------------------------------------------------------
startBinary_ARM9
Jumps to the ARM9 NDS binary in sync with the display and ARM7
Written by Darkain.
Modified by Chishm:
 * Removed MultiNDS specific stuff
--------------------------------------------------------------------------*/
#define startBinary_ARM9_size 0xA0
void __attribute__ ((long_call)) startBinary_ARM9 (void)
{
	REG_EXMEMCNT = 0xE880;
	// set ARM9 load address to 0 and wait for it to change again
	ARM9_START_FLAG = 0;
	while(REG_VCOUNT!=191);
	while(REG_VCOUNT==191);
	while ( ARM9_START_FLAG != 1 );
	// wait for vblank
	while(REG_VCOUNT!=191);
	while(REG_VCOUNT==191);
	resetCpu();
}

/*-------------------------------------------------------------------------
resetMemory_ARM7
Clears all of the NDS's RAM that is visible to the ARM7
Written by Darkain.
Modified by Chishm:
 * Added STMIA clear mem loop
--------------------------------------------------------------------------*/
void resetMemory_ARM7 (void)
{
	int i;
	u8 settings1, settings2;
	u32 settingsOffset = 0;
	
	REG_IME = 0;

	for (i=0; i<16; i++) {
		SCHANNEL_CR(i) = 0;
		SCHANNEL_TIMER(i) = 0;
		SCHANNEL_SOURCE(i) = 0;
		SCHANNEL_LENGTH(i) = 0;
	}
	SOUND_CR = 0;

	//clear out ARM7 DMA channels and timers
	for (i=0; i<4; i++) {
		DMA_CR(i) = 0;
		DMA_SRC(i) = 0;
		DMA_DEST(i) = 0;
		TIMER_CR(i) = 0;
		TIMER_DATA(i) = 0;
	}

	//switch to user mode
  __asm volatile(
	"mov r6, #0x1F                \n"
	"msr cpsr, r6                 \n"
	:
	:
	: "r6"
	);

  __asm volatile (
	// clear exclusive IWRAM
	// 0380:0000 to 0380:FFFF, total 64KiB
	"mov r0, #0 				\n"	
	"mov r1, #0 				\n"
	"mov r2, #0 				\n"
	"mov r3, #0 				\n"
	"mov r4, #0 				\n"
	"mov r5, #0 				\n"
	"mov r6, #0 				\n"
	"mov r7, #0 				\n"
	"mov r8, #0x03800000		\n"	// Start address part 1
	"sub r8, #0x00008000		\n" // Start address part 2
	"mov r9, #0x03800000		\n" // End address part 1
	"orr r9, r9, #0x10000		\n" // End address part 2
	"clear_EIWRAM_loop:			\n"
	"stmia r8!, {r0, r1, r2, r3, r4, r5, r6, r7} \n"
	"cmp r8, r9					\n"
	"blt clear_EIWRAM_loop		\n"

	// clear most of EWRAM - except after 0x023FFE00, which has the ARM9 loop
	"mov r8, #0x02000000		\n"	// Start address
	"mov r9, #0x02400000		\n" // End address part 1
	"sub r9, #0x00000200		\n" // End address part 2
	"clear_EXRAM_loop:			\n"
	"stmia r8!, {r0, r1, r2, r3, r4, r5, r6, r7} \n"
	"cmp r8, r9					\n"
	"blt clear_EXRAM_loop		\n"
	:
	:
	: "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9"
	);
  
	REG_IE = 0;
	REG_IF = ~0;
	(*(vu32*)(0x04000000-4)) = 0;  //IRQ_HANDLER ARM7 version
	(*(vu32*)(0x04000000-8)) = ~0; //VBLANK_INTR_WAIT_FLAGS, ARM7 version
	REG_POWERCNT = 1;  //turn off power to stuffs
	
	// Get settings location
	boot_readFirmware((u32)0x00020, (u8*)&settingsOffset, 0x2);
	settingsOffset *= 8;
	
	// Reload DS Firmware settings
	boot_readFirmware(settingsOffset + 0x070, &settings1, 0x1);
	boot_readFirmware(settingsOffset + 0x170, &settings2, 0x1);
	
	if ((settings1 & 0x7F) == ((settings2+1) & 0x7F)) {
		boot_readFirmware(settingsOffset + 0x000, (u8*)0x027FFC80, 0x70);
	} else {
		boot_readFirmware(settingsOffset + 0x100, (u8*)0x027FFC80, 0x70);
	}
}


void loadBinary_ARM7 (u32 fileCluster)
{
	u32 ndsHeader[0x170>>2];

	// read NDS header
	fileRead ((char*)ndsHeader, fileCluster, 0, 0x170);
	// read ARM9 info from NDS header
	u32 ARM9_SRC = ndsHeader[0x020>>2];
	char* ARM9_DST = (char*)ndsHeader[0x028>>2];
	u32 ARM9_LEN = ndsHeader[0x02C>>2];
	// read ARM7 info from NDS header
	u32 ARM7_SRC = ndsHeader[0x030>>2];
	char* ARM7_DST = (char*)ndsHeader[0x038>>2];
	u32 ARM7_LEN = ndsHeader[0x03C>>2];
	
	// Load binaries into memory
	fileRead(ARM9_DST, fileCluster, ARM9_SRC, ARM9_LEN);
	fileRead(ARM7_DST, fileCluster, ARM7_SRC, ARM7_LEN);

	// first copy the header to its proper location, excluding
	// the ARM9 start address, so as not to start it
	TEMP_ARM9_START_ADDRESS = ndsHeader[0x024>>2];		// Store for later
	ndsHeader[0x024>>2] = 0;
	dmaCopyWords(3, (void*)ndsHeader, (void*)NDS_HEAD, 0x170);
}

/*-------------------------------------------------------------------------
startBinary_ARM7
Jumps to the ARM7 NDS binary in sync with the display and ARM9
Written by Darkain.
Modified by Chishm:
 * Removed MultiNDS specific stuff
--------------------------------------------------------------------------*/
#define startBinary_ARM7_size 0xB0
void startBinary_ARM7 (void)
{
	while(REG_VCOUNT!=191);
	while(REG_VCOUNT==191);
	
	// copy NDS ARM9 start address into the header, starting ARM9
	ARM9_START_FLAG = 1;

	while(REG_VCOUNT!=191);
	while(REG_VCOUNT==191);
	// Start ARM7
	resetCpu();
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Main function

int main (void) {
	u32 fileCluster = storedFileCluster;
	// Init card
	if(!FAT_InitFiles(initDisc))
	{
		return -1;
	}
	if ((fileCluster < CLUSTER_FIRST) || (fileCluster >= CLUSTER_EOF)) 	/* Invalid file cluster specified */
	{
		fileCluster = getBootFileCluster(bootName);
	}
	if (fileCluster == CLUSTER_FREE)
	{
		return -1;
	}
	
	// ARM9 clears its memory part 1
	// copy ARM9 function to RAM, and make the ARM9 jump to it
	copyLoop((void*)TEMP_MEM, (void*)resetMemory1_ARM9, resetMemory1_ARM9_size);
	(*(vu32*)0x027FFE24) = (u32)TEMP_MEM;	// Make ARM9 jump to the function	
	// Wait until the ARM9 has completed its task
	while ((*(vu32*)0x027FFE24) == (u32)TEMP_MEM);

	// ARM9 clears its memory part 2
	// copy ARM9 function to RAM, and make the ARM9 jump to it
	copyLoop((void*)TEMP_MEM, (void*)resetMemory2_ARM9, resetMemory2_ARM9_size);
	(*(vu32*)0x027FFE24) = (u32)TEMP_MEM;	// Make ARM9 jump to the function
	// Wait until the ARM9 has completed its task
	while ((*(vu32*)0x027FFE24) == (u32)TEMP_MEM);

	// Get ARM7 to clear RAM
	resetMemory_ARM7();	
	
	// ARM9 enters a wait loop
	// copy ARM9 function to RAM, and make the ARM9 jump to it
	copyLoop((void*)TEMP_MEM, (void*)startBinary_ARM9, startBinary_ARM9_size);
	(*(vu32*)0x027FFE24) = (u32)TEMP_MEM;	// Make ARM9 jump to the function
	
	// Load the NDS file
	loadBinary_ARM7(fileCluster);
	
	// Patch with DLDI if desired
	if (wantToPatchDLDI) {
		dldiPatchBinary ((u8*)((u32*)NDS_HEAD)[0x0A], ((u32*)NDS_HEAD)[0x0B]);
	}
	
	startBinary_ARM7();

	return 0;
}

