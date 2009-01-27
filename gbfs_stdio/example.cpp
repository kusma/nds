#include <gba_console.h>
#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_input.h>
#include <gba_systemcalls.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/dir.h>
#include <vector>
#include <string>

#include "gbfs_stdio.h"

#include <assert.h>
#define ASSERT(expr) assert(expr)

void showfile(const char *file)
{
	int bytes_read;
	char buffer[31];
	
	/* open file */
	FILE *fp = fopen(file, "r");
	if (NULL == fp)
	{
		printf("failed to open file!\n");
		exit(1);
	}
	
	/* clear screen and go home */
	iprintf("\x1b[2J");
	
	/* print all lines */
	for (int i = 0; i < 20; ++i)
	{
		if (NULL == fgets(buffer, 30, fp)) break;
		buffer[30] = '\0';
		iprintf("%s", buffer);
	}
	
	fclose(fp);
	fp = NULL;
		
	while (1)
	{
		VBlankIntrWait();
		scanKeys();
		int keys = keysDown();
		if (keys & KEY_A) return;
	}
}

std::vector<std::string> update_menu(const char *path)
{
	DIR_ITER* dir;
	struct stat st;
	char filename[256];
	
	dir = diropen(path);
	if (NULL == dir)
	{
		printf("failed to open path \"%s\"\n", dir);
		exit(1);
	}
	
	iprintf("\x1b[2J");
	std::vector<std::string> filenames;
	while (dirnext(dir, filename, &st) == 0)
	{
		filenames.push_back(filename);
		printf("   %s (%d bytes)\n", filename, st.st_size);
	}
	return filenames;	
}

void filemenu(const char *path)
{
	int pos = 0;
	int old_pos = -1;
	
	std::vector<std::string> filenames = update_menu(path);

	while (1)
	{
		VBlankIntrWait();
		scanKeys();
		int keys = keysDown();
		
		if (keys & KEY_A)
		{
			std::string file = path + filenames[pos];
			showfile(file.c_str());
			filenames = update_menu(path);
			old_pos = -1;
		}
			
		if (keys & KEY_UP) pos--;
		if (keys & KEY_DOWN) pos++;
		
		/* clamp pos */
		if (pos < 0) pos = 0;
		if (pos >= filenames.size()) pos = filenames.size() - 1;
			
			
		if (pos != old_pos)
		{
			iprintf("\x1b[%d;0H->", pos);
			if (old_pos >= 0) iprintf("\x1b[%d;0H  ", old_pos);
		}
		old_pos = pos;
	}	
}

int main()
{
	irqInit();
	irqEnable(IRQ_VBLANK);
	consoleInit(0, 4, 0, NULL, 0, 15);

	BG_COLORS[0] = RGB5(0, 0, 0);
	BG_COLORS[241] = RGB5(31, 31, 31);
	REG_DISPCNT = MODE_0 | BG0_ON;

	gbfs_init(0);
	
	filemenu("gbfs:/");
			
	return 0;
}
