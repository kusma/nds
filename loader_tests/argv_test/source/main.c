#include <nds.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	int i;
	defaultExceptionHandler();
	
	powerON(POWER_ALL);
	irqInit();
	irqEnable(IRQ_VBLANK);
	
	// setup subscreen
	videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE); //sub bg 0 will be used to print text
	vramSetBankC(VRAM_C_SUB_BG);
	SUB_BG0_CR = BG_MAP_BASE(31);
	BG_PALETTE_SUB[255] = RGB15(31,31,31); //by default font will be rendered with color 255
	consoleInitDefault((u16*)SCREEN_BASE_BLOCK_SUB(31), (u16*)CHAR_BASE_BLOCK_SUB(0), 16);

	puts("argv:");
	for (i = 0; i < argc; ++i)
	{
//		puts(argv[i]);
		printf("%p - \"%s\"", argv[i], argv[i]);
	}
	puts("end of argv.");
	
	while(1);
	
	return 0;
}
