#include "pimp_gba.h"

#define REG_BASE 0x04000000

#define REG_DISPCNT  (*(volatile unsigned short*)(REG_BASE + 0x0000))
#define	REG_DISPSTAT (*(volatile unsigned short*)(REG_BASE + 0x0004))
#define	REG_VCOUNT   (*(volatile unsigned short*)(REG_BASE + 0x0006))

#define	REG_IE       (*(volatile unsigned short*)(REG_BASE + 0x0200))
#define REG_WAITCNT  (*(volatile unsigned short*)(REG_BASE + 0x0204))
#define	REG_IME      (*(volatile unsigned short*)(REG_BASE + 0x0208))

#define BG_COLORS ((volatile unsigned short*)0x05000000)

enum gba_irq_mask
{
	GBA_IRQ_VBLANK = 1,
};

typedef void (*gba_irq_func)(void);
#define INT_VECTOR (*(gba_irq_func *)(0x03007ffc))
extern void gba_irq_handler(void);

float f = 0;
__attribute__((section("iwram"))) void vblank()
{
	int i;
	static int j = 0;
	
	while (REG_VCOUNT > 0);
	BG_COLORS[0] = 0xF0;
	for (i = 0; i < 500; ++i)
	{
		f += 0.01f;
	}
//	while (REG_VCOUNT < 80);
	BG_COLORS[0] = 0x0;

	BG_COLORS[0] = j++ & 0xFFFF;
}

int gba_main()
{
	REG_WAITCNT = 0x46da; // lets set some cool waitstates...
	REG_DISPCNT = 0;
	
	// initialize interrupts
	INT_VECTOR = gba_irq_handler;
	REG_IE = 0;
	REG_DISPSTAT = 0;
	
	// enable vblank
	REG_IME = 0;
	REG_DISPSTAT |= (int)GBA_IRQ_VBLANK << 3;
	REG_IE       |= GBA_IRQ_VBLANK;
	REG_IME = 1;
	
	while (1);
	
/*	while (1) BG_COLORS[0] = i++ & 0xFFFF;
	return 0; */
}
