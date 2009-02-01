#include <nds.h>
#include "nds_loader_arm7.h"

touchPosition first,tempPos;

void VcountHandler() {
	inputGetAndSend();
}

int main()
{
	// read User Settings from firmware
	readUserSettings();
	
	irqInit();
	initClockIRQ();
	
	SetYtrigger(80);
	irqSet(IRQ_VCOUNT, VcountHandler);
	
	irqEnable(IRQ_VCOUNT);
	
	// Keep the ARM7 mostly idle
	while (1)
	{
		runNdsLoaderCheck();
		swiWaitForVBlank();
	}
}
