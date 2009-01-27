#include <nds.h>
#include "nds_loader_arm7.h"

touchPosition first,tempPos;

void VcountHandler() {
//---------------------------------------------------------------------------------
	static int lastbut = -1;
	
	uint16 but=0, x=0, y=0, xpx=0, ypx=0, z1=0, z2=0;

	but = REG_KEYXY;

	if (!( (but ^ lastbut) & (1<<6))) {
 
		tempPos = touchReadXY();

		if ( tempPos.x == 0 || tempPos.y == 0 ) {
			but |= (1 <<6);
			lastbut = but;
		} else {
			x = tempPos.x;
			y = tempPos.y;
			xpx = tempPos.px;
			ypx = tempPos.py;
			z1 = tempPos.z1;
			z2 = tempPos.z2;
		}
		
	} else {
		lastbut = but;
		but |= (1 <<6);
	}

	IPC->touchX			= x;
	IPC->touchY			= y;
	IPC->touchXpx		= xpx;
	IPC->touchYpx		= ypx;
	IPC->touchZ1		= z1;
	IPC->touchZ2		= z2;
	IPC->buttons		= but;

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
