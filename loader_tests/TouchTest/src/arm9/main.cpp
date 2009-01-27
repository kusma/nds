#include <nds.h>
#include <stdio.h>
#include <nds/arm9/input.h>
#include <math.h>
#include <sys/dir.h>
#include <unistd.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include "nds_loader_arm9.h"

#include "icon_slot1-cart.h"
#include "icon_folder.h"
#include "fade.h"

#include "font.h"

#include "large_font.h"
#include "small_font.h"


#ifdef USE_EFSLIB
extern "C"
{
#include "nitrofs.h"
}
#include "efs_lib.h"    // include EFS lib
#else
#include "fat.h"
#endif

volatile int frame = 0;
void Vblank()
{
	frame++;
}

#define ENTRY_HEIGHT 32

struct DirEntry
{
	char filename[256];
	bool isDir;
	unsigned int tiles[8 * 4 * 32];
	u16 pal[16];
};

std::vector<DirEntry> dirEntries;
size_t maxScrollPos;

void uploadDirEntry(int index)
{
	const int entry = index & 7;
	
	const int char_offset = 4;
	int curr_char_offset = (char_offset + entry * 32 * 4) * ((8 * 8) / 2);
	int text_pal_offset = entry * 16 * 2;
	int pal_offset = (entry + 8) * 16 * 2;
	
	if (index >= 0 && index < int(dirEntries.size()))
	{
		const DirEntry &dirEntry = dirEntries[index];
		memcpy((void*)(CHAR_BASE_BLOCK(0) + curr_char_offset), dirEntry.tiles, 32 * 4 * ((8 * 8) / 2));
		memcpy((void*)((u8*)BG_PALETTE + pal_offset),          dirEntry.pal,   16 * 2);
		
		if (0 == (index & 1)) memcpy((void*)((u8*)BG_PALETTE + text_pal_offset), &fadePal[0],  16 * 2);
		else                  memcpy((void*)((u8*)BG_PALETTE + text_pal_offset), &fadePal[16], 16 * 2);
	}
	else
	{
		memset((void*)(CHAR_BASE_BLOCK(0) + curr_char_offset), 0, 32 * 4 * ((8 * 8) / 2));
		memset((void*)((u8*)BG_PALETTE + pal_offset),          0xFF, 16 * 2);
		memset((void*)((u8*)BG_PALETTE + text_pal_offset),     0xFF, 16 * 2);
	}
}

int scroll = 0;
void setScroll(int newScroll)
{
	int delta = (newScroll >> 5) - (scroll >> 5);
	
	if (delta > 0)
	{
		for (int i = 0; i < delta + 1; ++i)
		{
			uploadDirEntry((scroll >> 5) + 6 + i);
		}
	}
	else if (delta < 0)
	{
		for (int i = 0; i > delta - 1; --i)
		{
			uploadDirEntry((scroll >> 5) + i);
		}
	}
	scroll = newScroll;
}


bool loadNDSIcon(const char *filename, void *tiles, u16 *pal)
{
	FILE *fp = fopen(filename, "rb");
	if (NULL == fp) return false;
	
	fseek(fp, 0x68, SEEK_SET);
	unsigned int bannerOffset = 0;
	fread(&bannerOffset, sizeof(unsigned int), 1, fp);
	
	if (0 == bannerOffset)
	{
		fclose(fp);
		
		/* copy the default tiles instead */
		memcpy(tiles, slot1_cartTiles, ((32 * 32) / 2));
		memcpy(pal, slot1_cartPal, 16 * 2);
		
		return false;
	}
	
#if 0
	// read english banner
	fseek(fp, bannerOffset + 0x340, SEEK_SET);
	u16 banner[0x100 + 1];
	fread(banner, sizeof(u16), 0x100, fp);
	banner[0x100] = '\0';
	
	for (int i = 0; i < 0x100; ++i)
	{
		if ('\0' == banner[i]) break;
		if (banner[i] < 0x80) putchar(banner[i]);
		else putchar('?');
	}
#endif
	
	// read bitmap
	fseek(fp, bannerOffset + 0x20, SEEK_SET);
	fread(tiles, 1, ((32 * 32) / 2), fp);
	
	// read palette
	fseek(fp, bannerOffset + 0x220, SEEK_SET);
	fread(pal, 2, 16, fp);
	
	fclose(fp);
	return true;
}

void scanDirectory()
{
	char path[256];
	getcwd(path, 256);

	iprintf("scanning directory \"%s\"...\n", path);
	dirEntries.clear();

	DIR_ITER *dp = diropen(".");

	if (NULL == dp)
	{
		iprintf ("Unable to open the directory.\n");
	}
	else
	{
		struct stat st;
		DirEntry dirEntry;
		int entry = 0;
		while (0 == dirnext(dp, dirEntry.filename, &st))
		{
			char *ext = strrchr(dirEntry.filename, '.');
			if (!S_ISDIR(st.st_mode) && (NULL == ext || 0 != stricmp(ext, ".nds"))) continue;
			if (S_ISDIR(st.st_mode) && (0 == strcmp(dirEntry.filename, "."))) continue;
			if (strlen(dirEntry.filename) > 1 && dirEntry.filename[0] == '.' && dirEntry.filename[1] != '.' ) continue;
			
			dirEntry.isDir = 0 != (st.st_mode & S_IFDIR);
			
			memset(dirEntry.tiles, 5 | (5 << 4), 32 * 4 * ((8 * 8) / 2));
			
			int y = 2;
			int w = 256 - 8 * 8;
			drawString(&dirEntry.tiles[(4 * 4) * 8] + y, 0, w, (4 * ((8 * 8) / 2)) / 4, &large_font_png, dirEntry.filename);
			
			if (st.st_mode & S_IFDIR)
			{
				drawString((unsigned int*)&dirEntry.tiles[(4 * 4) * 8] + y + 16, 0, w, (4 * ((8 * 8) / 2)) / 4, &small_font_png, "folder");
				memcpy(dirEntry.tiles, folder_iconTiles, ((32 * 32) / 2));
				memcpy(dirEntry.pal, folder_iconPal, 16 * 2);
			}
			else
			{
				char temp[256];
				if (st.st_size < 1024)                    snprintf(temp, 256, "%d bytes", int(st.st_size));
				else if (st.st_size < 1024 * 1024)        snprintf(temp, 256, "%d kB",    int(st.st_size) / 1024);
				else if (st.st_size < 1024 * 1024 * 1024) snprintf(temp, 256, "%d MB",    int(st.st_size) / (1024 * 1024));
				else                                      snprintf(temp, 256, "%d GB",    int(st.st_size) / (1024 * 1024 * 1024));
				drawString((unsigned int*)&dirEntry.tiles[(4 * 4) * 8] + y + 16, 0, w, (4 * ((8 * 8) / 2)) / 4, &small_font_png, temp);
				loadNDSIcon(dirEntry.filename, dirEntry.tiles, dirEntry.pal);
			}
			
			dirEntries.push_back(dirEntry);
			entry++;
		}
	}
	dirclose(dp);
	
	scroll = 0;
	maxScrollPos = std::max(0, int(dirEntries.size()) - 6) * ENTRY_HEIGHT;
	
	for (int i = 0; i < 7; ++i)
	{
		uploadDirEntry(i);
	}
}

int main(void)
{
	powerON(POWER_ALL_2D);
	
	// put the main screen on the bottom lcd
	lcdMainOnBottom();
	
	// Initialise the interrupt system
	irqInit();
	// install our simple vblank handler
	irqSet(IRQ_VBLANK, Vblank);
	
	irqEnable(IRQ_VBLANK | IRQ_HBLANK);
	
	//enable vram and map it to the right places
	vramSetMainBanks(	VRAM_A_MAIN_BG_0x06000000, VRAM_B_LCD,
						VRAM_C_SUB_BG , VRAM_D_LCD);
	
	//set the video mode
	videoSetMode(   MODE_0_2D | DISPLAY_BG0_ACTIVE);
	videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE);
	WIN_IN = 1;
	
	BG0_CR = BG_MAP_BASE(31);
	
	SUB_BG0_CR = BG_MAP_BASE(31);
	BG_PALETTE_SUB[255] = RGB15(31,31,31);
	consoleInitDefault((u16*)SCREEN_BASE_BLOCK_SUB(31), (u16*)CHAR_BASE_BLOCK_SUB(0), 16);
	
	
#ifdef USE_EFSLIB
#if 1
	iprintf("initing nitroFS...\n");
	if (!nitroFSInit("dummy"))
	{
		iprintf("failed to init nitroFS\n");
		while(1);
	}
	chdir("nitro:/");
#else
	iprintf("initing EFS...\n");
	if(!EFS_Init(EFS_AND_FAT/* | EFS_DEFAULT_DEVICE*/, NULL))
	{
		iprintf("failed to init EFS\n");
		while(1);
	}
	chdir("efs:/");
#endif
#else
	iprintf("initing libfat...\n");
	if (!fatInitDefault())
//	if (!fatInit(4, true))
	{
		iprintf("failed to init libfat\n");
		while(1);
	}
	iprintf("done.\n");
#endif

	
	// setup map
	memset((u8*)CHAR_BASE_BLOCK(0), 1 | (1 << 4), 8 * 4 * 32 * ((8 * 8) / 2));
	memcpy((void*)CHAR_BASE_BLOCK(0), fadeTiles, 3 * ((8 * 8) / 2));
	
	int char_offset = 4;
	
	for (int e = 0; e < 8; ++e)
	{
		int icon_pal = e + 8;
		int text_pal = e;
		
		// fill in icon map
		int dst_offset = e * 32 * 4 + 1;
		int src_offset = char_offset + e * 32 * 4;
		for (int y = 0; y < 4; ++y)
		{
			for (int x = 0; x < 4; ++x)
			{
				((u16*)SCREEN_BASE_BLOCK(31))[dst_offset + x] = 
					(x + y * 4 + src_offset) | (icon_pal << 12);
			}
			dst_offset += 32;
		}
		
		// fill in fade-line
		u16 *dst = ((u16*)SCREEN_BASE_BLOCK(31)) + 32 * 4 * e;
		for (int y = 0; y < 4; ++y)
		{
			dst[y * 32] = 0 | (text_pal << 12);
			for (int x = 0; x < 2; ++x)
			{
				dst[x + 5 + y * 32] = (1 + x) | (text_pal << 12);
			}
		}
		
		// fill in text-map
		dst = ((u16*)SCREEN_BASE_BLOCK(31)) + 32 * 4 * e + 7; // first tile on top
		for (int y = 0; y < 4; ++y)
		{
			for (int x = 0; x < 32 - 7; ++x)
			{
				dst[x + y * 32] = (src_offset + 4 * 4 + (x * 4 + y)) | (text_pal << 12);
			}
		}
	}
	
//	chdir(".svn");
//	chdir("..");
	scanDirectory();
	
	float scrollPos = 0.0f;
	float scrollDir = 0.0f;
	const float scrollFriction = 0.9f;
	const float clampFriction = 0.9f;
	int  dragAnchor = 0;
	bool draging = false;
	
	while (1)
	{
		scanKeys();
		
		// process input
		int down = keysDown();
		int up   = keysUp();
		
		touchPosition touchPos = touchReadXY();
		
		if (down & KEY_TOUCH)
		{
			// always stop when tapping the screen
			scrollDir = 0.0f;
			
			if (touchPos.px > SCREEN_WIDTH / 2)
			{
				dragAnchor = touchPos.py;
				draging = true;
			}
			else
			{
				int canvasPos = touchPos.py + int(scrollPos);
				int entry = canvasPos / ENTRY_HEIGHT;
				iprintf("entry: %d\n", entry);
				if ((entry >= 0) && (entry < int(dirEntries.size())))
				{
					const DirEntry &e = dirEntries[entry];
					if (e.isDir)
					{
						chdir(e.filename);
						scanDirectory();
						scrollPos = -16;
					}
					else
					{
						iprintf("running \"%s\"\n", e.filename);
						runNdsFile(e.filename);
					}
				}
			}
		}
		else if (up & KEY_TOUCH)
		{
			draging = false;
		}
		
		// animate
		if (!draging)
		{
			scrollPos += scrollDir;
			scrollDir *= scrollFriction;
			if (fabs(scrollDir) < 0.01f) scrollDir = 0.0f;
			
			if (scrollPos < 0.0f)
			{
				// clamp to start
				scrollDir = 0.0f;
				scrollPos *= clampFriction;
			}
			else if (scrollPos > maxScrollPos)
			{
				// clamp to end
				scrollDir = 0.0f;
				scrollPos = maxScrollPos + (scrollPos - maxScrollPos) * clampFriction;
			}
			
			setScroll(int(scrollPos));
		}
		else
		{
			int scrollDelta = dragAnchor - touchPos.py;
			scrollPos += scrollDelta;
			dragAnchor = touchPos.py;
			
			setScroll(int(scrollPos));
			
			// track scrollDir
			scrollDir += scrollDelta;
			scrollDir *= 0.75;
		}
		
		swiWaitForVBlank();
		BG_OFFSET[0].y = scroll;
	}
	return 0;
}


