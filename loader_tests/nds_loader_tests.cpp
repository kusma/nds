// nds_loader_tests.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <wchar.h>
#include "image.h"

int _tmain(int argc, _TCHAR* argv[])
{
	FILE *fp = fopen("test.nds", "rb");
	if (NULL != fp)
	{
		printf("found file\n");

		// find banner
		fseek(fp, 0x68, SEEK_SET);
		unsigned int bannerOffset = 0;
		fread(&bannerOffset, sizeof(unsigned int), 1, fp);
		if (0 != bannerOffset)
		{
			// read version
			fseek(fp, bannerOffset, SEEK_SET);
			unsigned short version;
			fread(&version, sizeof(unsigned short), 1, fp);

			// read english banner
			fseek(fp, bannerOffset + 0x340, SEEK_SET);
			wchar_t banner[0x100 + 1];
			fread(banner, sizeof(wchar_t), 0x100, fp);
			banner[0x100] = '\0';

			// output info
			printf("- version: %d\n", version);
			wprintf(L"- english banner:\n%s\n\n", banner);
			
			// read bitmap
			fseek(fp, bannerOffset + 0x20, SEEK_SET);
			unsigned char bitmap[(32 * 32) / 2];
			fread(bitmap, 1, sizeof(bitmap), fp);
			
			// read palette
			fseek(fp, bannerOffset + 0x220, SEEK_SET);
			unsigned short palette[16];
			fread(palette, 1, sizeof(palette), fp);
			
#if 1
			Image output;
			output.create(32, 32, 24);
			for (int yblock = 0; yblock < 4; ++yblock)
				for (int xblock = 0; xblock < 4; ++xblock)
				{
					int blockIndex = xblock + yblock * 4;

					const int blockSize = (8 * 8) / 2;
					unsigned char *src = &bitmap[blockSize * blockIndex];
					
					for (int y = 0; y < 8; ++y)
						for (int x = 0; x < 8; ++x)
						{
							unsigned char palIndex = src[(y * 8 + x) / 2];
							palIndex >>= (x & 1) * 4;
							
							unsigned short color = palette[palIndex & 0xf];
							unsigned int r = color         & ((1 << 5) - 1);
							unsigned int g = (color >> 5)  & ((1 << 5) - 1);
							unsigned int b = (color >> 10) & ((1 << 5) - 1);
							
							r = int((float(r) / 31) * 255);
							g = int((float(g) / 31) * 255);
							b = int((float(b) / 31) * 255);

							RGBQUAD rgbColor;
							rgbColor.rgbRed   = r;
							rgbColor.rgbGreen = g;
							rgbColor.rgbBlue  = b;
							output.setPixelColor(
								xblock * 8 + x,
								yblock * 8 + y,
								rgbColor
							);
						}
				}
			output.save("icon.png");
#endif
		}
		
		fclose(fp);
	}

	return 0;
}
