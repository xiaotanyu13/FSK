#include <stdio.h>

#include "FSKModem.h"
int main ()
{
	int i ;
	char a[] = {0x01,0x02,0x11,0x12,0x55};
	int len = 5;
	int outLen;
	short* ss = Modulate(a,len,&outLen,3);
	if(ss == NULL)
		printf("error\n");

	for(i = 0;i < outLen; i++)
	{
		if(i % 8 == 0)
			printf("\n");
		printf("%d ",ss[i]);
		
	}
	return 0;
}