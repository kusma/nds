#include <gba_console.h>
#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>

#include <stdio.h>

void overlay1_test() __attribute__((section(".iwram1"), long_call));
void overlay2_test() __attribute__((section(".iwram2"), long_call));

void overlay1_test()
{
	printf("inside %s\n", __func__);
}

void overlay2_test()
{
	printf("inside %s\n", __func__);
}

extern unsigned char __iwram_overlay_start[];
extern unsigned char __load_start_iwram0[];
extern unsigned char __load_stop_iwram0[];
extern unsigned char __load_start_iwram1[];
extern unsigned char __load_stop_iwram1[];
extern unsigned char __load_start_iwram2[];
extern unsigned char __load_stop_iwram2[];

int main()
{
	irqInit();
	irqEnable(IRQ_VBLANK);
	consoleInit(0, 4, 0, NULL, 0, 15);
	BG_COLORS[0]=RGB8(58,110,165);
	BG_COLORS[241]=RGB5(31,31,31);

	SetMode(MODE_0 | BG0_ON);

	/* verify that the overlays are indeed overlapping */
	printf("overlay1_test: %p\noverlay2_test: %p\n", overlay1_test, overlay2_test);
	
	/* set overlay 1 and run code from it */
	memcpy(__iwram_overlay_start, __load_start_iwram1, (int)__load_stop_iwram1 - (int)__load_start_iwram1);
	overlay1_test();
	
	/* set overlay 2 and run code from it */
	memcpy(__iwram_overlay_start, __load_start_iwram2, (int)__load_stop_iwram2 - (int)__load_start_iwram2);
	overlay2_test();
	
	/* back to normal */
	memcpy(__iwram_overlay_start, __load_start_iwram0, (int)__load_stop_iwram0 - (int)__load_start_iwram0);
	while(1);
}
