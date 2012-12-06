#include <stdio.h>
#include <stdlib.h>
#include "FSKModem.h"
int main ()
{
	char a[] = {0x01,0x54,0x33,0x45,0x55};
	int len = 5;
	int outLen = 0;
	short* ss = NULL;
	
	unsigned long OutLenIndix;//解到哪点了

	unsigned char retdatabuf[2000];//用于存放解出来的数据,最多1.5k

	ss = Modulate(a,len,&outLen,3);

	Demodulate(retdatabuf, ss,outLen,&OutLenIndix);

	printf("please entry any key\n");
	return 0;
}