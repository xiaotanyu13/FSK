/*
	xiaotanyu13
*/
#include <iostream>
#include <time.h>
#include <stdlib.h>
#include <fstream>
#include "FSKModem.h"
using namespace std;


void TestModulate()
{
	CFSKModem *fsk = new CFSKModem();
	char a[] = {0x01,0x54,0x33,0x45,0x55};
	int len = 5;
	int outLen = 0;
	short* ss = NULL;
	ss = fsk->Modulate(a,len,&outLen);

	for(int i = 0; i < outLen; i ++)
	{
		if(i % MODULATE_SAMPLE == 0)
			printf("\n");
		printf("%d ",ss[i]);
	}
	free(ss);
}


/*****************************************
    功能:把文件 属性打印出来
    本函数调用的函数清单: 无
    调用本函数的函数清单: main
    输入参数:  *DataBuf   
    输出参数:  数据长度
    函数返回值说明:   
    使用的资源 
******************************************/
unsigned long GetHeadData(void)
{
	wav_pcm_header44 hwav;//用于存放WAV的文件头
	FILE *fpWav = fopen("123.wav", "rb");
	fread(&hwav, sizeof(wav_pcm_header44), 1,fpWav);

	if ( (0==memcmp(hwav.ChunkID, "RIFF", 4)) &&(0==memcmp(hwav.Format, "WAVE", 4)) &&
			(0==memcmp(hwav.SubChunk1ID, "fmt ", 4))  &&(1==hwav.AudioFormat)&&(0==memcmp(hwav.SubChunk2ID, "data", 4))
	    )
		{
			printf("Wave audio data format:\n");
			printf("Channel number: %d\n", hwav.NumChannels);
			printf("SampleRate: %dHz\n", hwav.SampleRate);
			printf("BitsPerSample: %dbits\n", hwav.BitsPerSample);
			printf("Audio data size:%d\n\n\n\n", hwav.SubChun2Size);
			return hwav.SubChun2Size;
		}
	else
		{
			printf("not WAV\n" );	
			printf("please entry any key\n");
			char c = getchar();
			putchar(c);
			return 0;
		
		}
}

int  TestDemodulate()
{
	CFSKModem *fsk = new CFSKModem();
	char a[] = {0x01,0x54,0x33,0x45,0x55};
	int len = 5;
	int outLen = 0;
	short* ss = NULL;
	ss = fsk->Modulate(a,len,&outLen);

	unsigned long LenthOfWAV = 0;//WAV文件的长度
	unsigned long OutLenIndix;//解到哪点了

	LenthOfWAV = GetHeadData(); //打印文件属性，并获取文件长度
	if(LenthOfWAV == 0)
	{
		printf("no WAV");//没有wav文件
		return 0;//如果没有正确数据，退出
	}

	unsigned char retdatabuf[2000];//用于存放解出来的数据,最多1.5k

	short *databuf = (short*)malloc((LenthOfWAV+44)*2);//根据长度，申请内存
	FILE *logfile = fopen("123.wav","rb");
	fread(databuf,1,LenthOfWAV+44,logfile);
	memcpy( ((char*)databuf+LenthOfWAV+44),databuf,(LenthOfWAV+44));
	//	int leee = GetValidData(databuf+22,(short *)tempbuf1,LenthOfWAV/2);
	//fsk->Demodulate(retdatabuf, databuf+22,LenthOfWAV/2,&OutLenIndix,1);
	fsk->Demodulate(retdatabuf, ss,outLen,&OutLenIndix,3);

	/**********************************************/
	//保存滤波后的波形
	FILE* file=fopen("234.wav","wb");
	if(file)
		//		fwrite(tempbuf1,leee*2,1,file);//
			fwrite(databuf,LenthOfWAV+44,1,file);
	else
		printf("open 234.wav fail\n");
	/**********************************************/

	if(logfile)
		fclose(logfile);
	if(file)
		fclose(file);
	if(databuf)
		free(databuf);
	printf("please entry any key\n");
	return 0;
}


int main()
{
	TestModulate();
	
	//CFSKModem *fsk = new CFSKModem();

	//TestDemodulate();
	
	return 0;
}