#include <nds.h>
#include <stdlib.h>

#include "nds_loader_arm7.h"

void VblankHandler(void) {}

int main(int argc, char ** argv)
{
	// Reset the clock if needed
//	rtcReset();
	
	irqInit();
	irqSet(IRQ_VBLANK, VblankHandler);
	irqEnable(IRQ_VBLANK);

	// Keep the ARM7 idle
	while (1)
	{
		runNdsLoaderCheck();
		swiWaitForVBlank();
	}
}


