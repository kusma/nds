#include "FreeImage.h"
#include "image.h"
#include <stdio.h>
#include <vector>

FIBITMAP *loadImage(const char *filename)
{
	FREE_IMAGE_FORMAT fif = FreeImage_GetFIFFromFilename(filename);
	FIBITMAP *temp = FreeImage_Load(fif, filename);
	if (temp == NULL) return (FIBITMAP*)NULL;
	
	FreeImage_FlipVertical(temp);
	return temp;
}

int main(int argc, char* argv[])
{
	int w = -1;
	int h = -1;
	
	for (int i = 1; i < argc; ++i)
	{
		if (argv[i][0] == '-')
		{
			if (argv[i][1] == 'w' && argv[i][2] == '\0')
			{
				i++;
				if (i < argc) w = atoi(argv[i]);
				continue;
			}
			else if (argv[i][1] == 'h' && argv[i][2] == '\0')
			{
				i++;
				if (i < argc) h = atoi(argv[i]);
				continue;
			}
		}
		else 
		{
			const char *filename = argv[i];
			char *array_name = strdup(filename);

			for (size_t i = 0; i < strlen(array_name); ++i) if (array_name[i] == '.') array_name[i] = '_';
			fprintf(stderr, "\"%s\" %d %d\n", array_name, w, h);
			
			FIBITMAP *dib = loadImage(filename);
			if (NULL != dib)
			{
				Image img(dib);
				
				std::vector<unsigned int> data32;
				
				if (w < 0) w = img.getWidth() / 16;
				if (h < 0) h = img.getHeight() / 16;
				
				printf("struct font %s = {\n\t%d,\n\t{\n\t\t", array_name, h);
				
				for (int y = 0; y < img.getHeight(); y += h)
				{
					for (int x = 0; x < img.getWidth(); x += w)
					{
						std::vector<unsigned char> data;
						
						int maxx = 0;
						for (int iy = 0; iy < h; ++iy)
						{
							for (int ix = 0; ix < w; ++ix)
							{
								if (0 != img.getPixelIndex(x + ix, y + iy) && maxx < ix) maxx = ix;
							}
						}
						
						printf("{%d, %d},\n\t\t", maxx + 1, data32.size());
						for (int iy = 0; iy < h; ++iy)
						{
							for (int ix = 0; ix <= maxx; ++ix)
							{
	//							printf("%x", img.getPixelIndex(x + ix, y + iy) - 1);
								data.push_back(std::max(img.getPixelIndex(x + ix, y + iy) - 1, 0));
							}
	//						printf("\n");
						}
						
						for (size_t i = 0; i < data.size(); i += 8)
						{
							size_t range = std::min(data.size() - i, size_t(8));
							unsigned int dstData = 0;
							for (size_t j = 0; j < range; ++j)
							{
								dstData |= data[j + i] << (j * 4);
							}
							data32.push_back(dstData);
						}
					}
				}
				
				printf("\n\t},\n\t{\n\t\t");
				for (size_t i = 0; i < data32.size(); ++i)
				{
					printf("0x%08x,%s", data32[i], ((i % 4) == 3) ? "\n\t\t" : " ");
				}
				printf("\n\t}\n};\n");
				
				free(array_name);
			}
		}
	}
	
	return 0;
}
