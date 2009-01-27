#include "compress_texture.h"

int main(int argc, char* argv[])
{
	for (int i = 1; i < argc; ++i)
	{
		if (!compressTexture(argv[i])) return -1;
	}
	
	return 0;
}
