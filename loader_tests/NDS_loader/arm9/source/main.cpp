/*---------------------------------------------------------------------------------
	$Id: template.c,v 1.4 2005/09/17 23:15:13 wntrmute Exp $

	Basic Hello World

	$Log: template.c,v $
	Revision 1.4  2005/09/17 23:15:13  wntrmute
	corrected iprintAt in templates
	
	Revision 1.3  2005/09/05 00:32:20  wntrmute
	removed references to IPC struct
	replaced with API functions
	
	Revision 1.2  2005/08/31 01:24:21  wntrmute
	updated for new stdio support

	Revision 1.1  2005/08/03 06:29:56  wntrmute
	added templates


---------------------------------------------------------------------------------*/
#include <nds.h>
#include <nds/arm9/console.h> //basic print funcionality
#include <stdio.h>
#include <fat.h>
#include <sys/stat.h>

#include <nds/arm9/video.h>
#include <string.h>

#include "nds_loader_arm9.h"
#include "file_browse.h"

const char DEFAULT_FILE[] = "_BOOT_DS.NDS";

void stop (void) 
{
	while (1) {
		swiWaitForVBlank();
	}
}

//---------------------------------------------------------------------------------
int main(void) {
//---------------------------------------------------------------------------------

	struct stat st;

	// Enable VBlank for Vblank Interrupt Wait
	irqInit();
	irqEnable(IRQ_VBLANK);

	videoSetMode(0);	//not using the main screen

	// Subscreen as a console
	videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE);
	vramSetBankH(VRAM_H_SUB_BG);
	SUB_BG0_CR = BG_MAP_BASE(15);
	BG_PALETTE_SUB[0]=0;   
	BG_PALETTE_SUB[255]=0xffff;
	consoleInitDefault((u16*)SCREEN_BASE_BLOCK_SUB(15), (u16*)CHAR_BASE_BLOCK_SUB(0), 16);

	iprintf ("Init'ing FAT...");
	if (fatInitDefault()) {
		iprintf ("okay\n");
	} else {
		iprintf ("fail!\n");
		stop();
	} 
	
	if (stat (DEFAULT_FILE, &st) >= 0) {
		iprintf ("Running %s\n", DEFAULT_FILE);
		runNdsFile (DEFAULT_FILE);
	} else {
		std::string filename = browseForFile (".nds");
		iprintf ("Running %s\n", filename.c_str());
		runNdsFile (filename.c_str());
	}

	iprintf ("Start failed\n");
	
	stop();

	return 0;
}
