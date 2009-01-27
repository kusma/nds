#include <gba_console.h>
#include <gba_video.h>
#include <gba_interrupt.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/dir.h>

#include "gbfs_stdio.h"
#define TEST(expr) if (!(expr)) printf("TEST failed! (%s:%d)\n", __FILE__, __LINE__)

void test_fopen()
{
	FILE *fp;
	
	fp = fopen("test.txt", "r");
	TEST(NULL == fp);

	fp = fopen("gbfs:/test.txt", "r");
	TEST(NULL != fp);
	fclose(fp);

	chdir("gbfs:/");
	fp = fopen("test.txt", "r");
	TEST(NULL != fp);
	fclose(fp);
	
}

void test_fread()
{
	FILE *fp;
	char buffer[256];
	int bytes_read;
	
	fp = fopen("gbfs:/test.txt", "r");
	TEST(NULL != fp);
	
	if (NULL == fp)
	{
		printf("failed to open file!\n");
		exit(1);
	}
	
	fseek(fp, 0, SEEK_END);
	TEST(87 == ftell(fp));
	rewind(fp);
	TEST(0 == ftell(fp));
	
	/* test simple read */
	bytes_read = fread(buffer, 1, 4, fp);
	TEST(4 == bytes_read);
	TEST(buffer[0] == '1');
	TEST(buffer[1] == '2');
	TEST(buffer[2] == '3');
	TEST(buffer[3] == '4');
	
	/* test reading end-bytes */
	fseek(fp, -4, SEEK_END);
	bytes_read = fread(buffer, 1, 4, fp);
	TEST(4 == bytes_read);
	TEST(buffer[0] == '4');
	TEST(buffer[1] == '3');
	TEST(buffer[2] == '2');
	TEST(buffer[3] == '1');

	/* test cropping output */
	fseek(fp, -3, SEEK_END);
	buffer[3] = 0x7f;
	bytes_read = fread(buffer, 1, 4, fp);
	TEST(3 == bytes_read);
	TEST(buffer[0] == '3');
	TEST(buffer[1] == '2');
	TEST(buffer[2] == '1');
	TEST(buffer[3] == 0x7f);	
	
	fclose(fp);
	fp = NULL;
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
	
	test_fopen();
	test_fread();
	printf("done.\n");
	
	return 0;
}
