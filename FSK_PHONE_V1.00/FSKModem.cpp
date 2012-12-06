/************************************************************************/
/* 
FSKModem.cpp
xiaotanyu13
2012/11/12
xiaot.yu@sunyard.com

封装了FSK调制解调的函数，采用类的方法来实现*/
/************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h> 
#include "FSKModem.h"

extern "C" {
#include "fftsg_h.c"
}

#define FSK_MODEM_GLOBALS



CFSKModem::CFSKModem()
{
	this->m_bIobitFlag = false;
	this->m_cPackageCount = 0;
	this->m_fPoint = (float*)malloc(2 * MAX_N_POINTS * sizeof(float));//滤波的时候保存数据 
	// 这个值只在滤波的时候用到了，其他时候都没有用处，这个参数需要具体的讨论一下
}

/*


输入参数：
type:	类型有0 和 1
0表示 0的波
1表示 1的波
输出参数：
retData:	保存buffer的指针
返回值：
对retData所作的偏移值
*/
int CFSKModem::ModulateBit(int type,short* retData)
{
	int index = 0;
	switch(type)
	{
	case 1: // 如果当前比特是1，则添加1 1的波形是有变化
		for(int i = 0; i < MODULATE_SAMPLE/2; i++)
		{
			*(retData + index++) = ((m_bIobitFlag == true) ?AMP_U:AMP_D);
		}
		m_bIobitFlag = !m_bIobitFlag;
		for(int i = 0; i < MODULATE_SAMPLE/2; i++)
		{
			*(retData + index++) = ((m_bIobitFlag == true) ?AMP_U:AMP_D);
		}
		m_bIobitFlag = !m_bIobitFlag;
		break;
	case 0: // 如果是0，则添加0,0的波形是没有变化
		for(int i = 0; i < MODULATE_SAMPLE; i ++)
		{
			*(retData + index++) = ((m_bIobitFlag == true) ?AMP_U:AMP_D);
		}
		m_bIobitFlag = !m_bIobitFlag;
		break;
	}
	return index;
}
/*
调制一个字节的数据,一个字节之后还需要加两个0
输入参数：
byte：	需要调制的字节
输出参数：
retData：保存调制后的数据数组
返回值：
对retData所作的偏移
*/
int CFSKModem::ModulateByte(BYTE byte,short* retData)
{
	int offset = 0;
	for(int i = 0; i < BYTE_LEN; i ++)
	{
		if((byte << i) & 0x80)
		{
			offset += this->ModulateBit(1,retData + offset);
		}
		else
		{
			offset += this->ModulateBit(0,retData + offset);
		}
	}

	return offset;
}
/*
组同步域，固定为0x01015555
输入参数：

输出参数：
data：保存调制后的数据数组
返回值：
对data所作的偏移
*/
int CFSKModem::PackSYNC(BYTE* data)
{
	int offset = 0;
	data[offset++] = 0x55;
	data[offset++] = 0x55;
	data[offset++] = 0x01;
	data[offset++] = 0x01;
	return offset;
}
/*
组计数域，1个字节
输入参数：

输出参数：
data：保存调制后的数据数组
返回值：
对data所作的偏移
*/
int CFSKModem::PackCount(BYTE* data)
{
	this->m_cPackageCount ++;
	this->m_cPackageCount = this->m_cPackageCount % 256;
	//BYTE* count = (BYTE*)&this->m_cPackageCount;
	int offset = 0;
	data[offset++] = this->m_cPackageCount;
	return offset;

}
/*
组数据长度域，2字节
输入参数：
len：	数据的长度
输出参数：
data：保存调制后的数据数组
返回值：
对data所作的偏移
*/
int CFSKModem::PackLength(int len,BYTE* data)
{
	int offset = 0;
	data[offset++] = len % 256;
	data[offset++] = len / 256;
	return offset;
}
/*
组数据域
输入参数：
dataIn：需要调制的数据
len：	数据的长度
输出参数：
data：保存调制后的数据数组
返回值：
对data所作的偏移
*/
int CFSKModem::PackData(char* dataIn,int len,BYTE* data)
{
	memcpy(data,(BYTE*)dataIn,len);
	return len;
}
/*
组校验域
输入参数：
crc：	校验值
输出参数：
data：保存调制后的数据数组
返回值：
对data所作的偏移
*/
int CFSKModem::PackCRC(short crc,BYTE* data)
{
	int offset = 0;
	data[offset++] = crc % 256;
	data[offset++] = crc / 256;
	return offset;
}
/*
组包，包格式：
同步域	包计数域	数据长度域	数据域	校验域
4字节	1字节		2字节		n字节	2字节
输入参数：
data:	上层传递下来的数据
len:	数据的长度
输出参数：
outLen：返回值的长度
返回值：
按照以上格式组好的包
*/
BYTE* CFSKModem::PackField(char* data,int len,int *outLen)
{
	int bufLen = len + 9;
	BYTE *buf = NULL;
	int offset = 0;
	int dataLen = 0;
	//buf = (BYTE*)calloc(bufLen,0);
	buf = new BYTE[bufLen];
	offset += this->PackSYNC(buf + offset);
	offset += this->PackCount(buf + offset);
	offset += this->PackLength(len,buf+offset);
	dataLen = this->PackData(data,len,buf+offset);
	// crc校验只需要校验数据域就行
	short crc = this->CalculateCRC(buf+offset,dataLen);
	offset += dataLen;
	offset += this->PackCRC(crc,buf+offset);
	*outLen = offset;
	return buf;
}

/*
对数据进行crc校验，并且返回crc校验值
输入参数：
buf：	需要计算crc的数据
len：	数据的长度
输出参数：

返回值：
crc校验结果
*/
short CFSKModem::CalculateCRC(BYTE *buf,int len)
{
	BYTE hi,lo;
	int i;
	BYTE j;
	short crc;
	crc=0xFFFF;
	for (i=0;i<len;i++)
	{
		crc=crc ^ *buf;
		for(j=0;j<8;j++)
		{
			BYTE chk;
			chk=crc&1;
			crc=crc>>1;
			crc=crc&0x7fff;
			if (chk==1)
				crc=crc^0x8408;
			crc=crc&0xffff;
		}
		buf++;
	}
	hi=crc%256;
	lo=crc/256;
	crc=(hi<<8)|lo;
	return crc;
}


/*
调制数据
输入参数：
data：	从应用层传下来的命令
len：	data的长度
输出参数：
outFrameLen：	调制成音频数据的长度
返回值：
调制成的音频数据,这个返回值需要用户自己去销毁
*/
short* CFSKModem::Modulate(char* data,int len,int
						   * outFrameLen)
{
	int packageLen = 0;
	BYTE* packBuf = this->PackField(data,len,&packageLen);
	// 开始调制
	short* voiceData = NULL;
	int offset = 0;
	int voiceLen = packageLen * MODULATE_SAMPLE * BYTE_LEN;
	voiceData = new short[voiceLen];

	for(int i = 0; i < packageLen; i ++)
	{
		offset += this->ModulateByte(packBuf[i],voiceData+offset);
	}
	free(packBuf);
	*outFrameLen = offset;
	return voiceData;
}


void CFSKModem::FindFrame(short *pData, unsigned long len, long *start, long *end)
{
	unsigned long i, j;
	unsigned long E;
	BYTE flag = 0;

	*start = *end = -1;
	if(len < 128)
		return;
	for(i = 0; i <= len - E_POINTS; i += E_POINTS) {
		E = 0;
		for(j = 0; j < E_POINTS; j ++) {
			E += abs(pData[i+j]);
		}
		if(!flag) { // find start pos of sound
			if(E > E_SOUND_THRESHOLD) {
				if(i >= E_POINTS)
					*start = i - E_POINTS;
				else
					*start = i;
				flag = 1;
			}
		}
		else {
			if(E < E_SILENCE_THRESHOD) {
				*end = i + E_POINTS;
				//*end = i;
				return;
			}
		}
	}
	// start pos found
	if(flag)
		*end = len -1;
}
unsigned long CFSKModem::GetN(unsigned long len)
{
	unsigned long N;

	if(len <= 64) {
		N = 64;
	}
	else if(len <= 128) {
		N = 128;
	}
	else if(len <= 512) {
		N = 512;
	}
	else if(len <= 1204) {
		N = 1024;
	}
	else if(len <= 2048) {
		N = 2048;
	}
	if(len <= 4096) {
		N = 4096;
	}
	else if(len <= 8192) {
		N = 8192;
	}
	else if(len <= 16384) {
		N = 16384;
	}
	else if(len <= 32768) {
		N = 32768;
	}
	else if(len <= 64*1024) {
		N = 64*1024;
	}
	else if(len <= 128*1024) {
		N = 128*1024;
	}
	else if(len <= 256*1024) {
		N = 256*1024;
	}
	else if(len <= 512*1024) {
		N = 512*1024;
	}
	else {
		return 0;
	}

	return N;
}

int CFSKModem::GetValidData(short *InDataBuf,short *OutDataBuf,unsigned long lenth)
{
	unsigned long i = 0;//指向第i个点
	unsigned long NumberOfLow = 0;//小幅度波的个数
	unsigned long k = 0;//指向第K个有效数据
	int Lenthofdata = 0;
	int isEnd = 0;

	if(InDataBuf[lenth-1]>1500 || InDataBuf[lenth-1]<-1500) 
	{
		isEnd++;
	}
	if(InDataBuf[lenth-2]>1500 || InDataBuf[lenth-2]<-1500) {
		isEnd++;
	}
	if(InDataBuf[lenth-3]>1500 || InDataBuf[lenth-3]<-1500) {
		isEnd++;
	}
	if(InDataBuf[lenth-4]>1500 || InDataBuf[lenth-4]<-1500) {
		isEnd++;
	}
	if(InDataBuf[lenth-5]>1500 || InDataBuf[lenth-5]<-1500) {
		isEnd++;
	}
	if(InDataBuf[lenth-6]>1500 || InDataBuf[lenth-6]<-1500) {
		isEnd++;
	}
	if(InDataBuf[lenth-7]>1500 || InDataBuf[lenth-7]<-1500) {
		isEnd++;
	}
	if(InDataBuf[lenth-8]>1500 || InDataBuf[lenth-8]<-1500){
		isEnd++;
	}
	if(InDataBuf[lenth-9]>1500 || InDataBuf[lenth-9]<-1500) {
		isEnd++;
	}
	if(InDataBuf[lenth-10]>1500 || InDataBuf[lenth-10]<-1500) {
		isEnd++;
	}
	if(isEnd >6)
	{
		return -1;
	}

	for(i=0;i<lenth;i++)
	{   
		NumberOfLow = 0;
		while((InDataBuf[i] < 500)&&(InDataBuf[i] > -500)) //去除 连续 3点小于500的点
		{
			i++;
			NumberOfLow++;
			if(i == lenth)
			{
				goto endLow;
			}
		}
		if(NumberOfLow < 3)//对于中间出现的小幅度波小于3，要退回去
		{
			i -= NumberOfLow;
		} 
		OutDataBuf[Lenthofdata] = InDataBuf[i];
		Lenthofdata++;

	}
endLow:
	//去除干扰，连续超过20个大于或者小于0 的, 把多余的去掉
	short currentData = OutDataBuf[0];
	int tempLenth = Lenthofdata;
	int j = 1;

	for(i=0; i<tempLenth; i++)
	{   
		if(i == 47)
			i = i;
		if((currentData <0 && OutDataBuf[i] <=0) ||(currentData >0 && OutDataBuf[i] >=0) )
		{
			j++;
		}
		else
		{	
			currentData = OutDataBuf[i];
			if(j>40)
			{
				memcpy(&OutDataBuf[i]-(j-20),OutDataBuf+(i-20),(tempLenth-(i-20))*2);
				i = i - (j-20);
				tempLenth = tempLenth - (j-40); 
			}
			j = 1;
		}
	}
	if(j>40)
	{
		memcpy(&OutDataBuf[i]-(j-20),OutDataBuf+(i-20),(tempLenth-(i-20))*2);
		i = i - (j-20);
		tempLenth = tempLenth - (j-40); 
	}
	j = 1;

	return tempLenth;
}

/*****************************************
功能:找到同步头  55550101 或 EFEF0101
本函数调用的函数清单: 无
调用本函数的函数清单: main
输入参数:  *DataBuf   
输出参数:  第几个点开始是数据
函数返回值说明:   
使用的资源 
******************************************/
BYTE CFSKModem::FindHead(short * InDataBuf,unsigned long lenth,unsigned long *endlen,BYTE MobileType)
{
	unsigned long i = 0;//指向第i个点

	float LengthOfZorePassage = 0;//过零点之间宽度
	float LastRatio = 0;///过零点上一次的比率
	float	RatioOfZorePassage = 0;//过零点这一次的比率

	unsigned long NumberOfLow = 0;//小幅度波的个数
	unsigned long DataHead = 0;//同步头
	unsigned long datastart = 0;//当前在解的波的开始点
	BYTE bit0flag = 0;//用于实现两个小波表示1

	for(;i<lenth;)
	{   
		NumberOfLow = 0;
		while((InDataBuf[i] < 500)&&(InDataBuf[i] > -500)) //直到有大于500的点，去"串扰"
		{
			i++;
			NumberOfLow++;
			if(i == lenth)
			{
				return 0;
			}
		}
		if(NumberOfLow < 5)//对于中间出现的小幅度波小于5，要退回去
		{
			i -= NumberOfLow;
		}
		datastart = i;//当前波的开始点
		LastRatio = RatioOfZorePassage;//保存上一次的过零点比率
		if(InDataBuf[i] >= 0)//如果采样值大于等于0
		{
			while(InDataBuf[i] >= 0)//直到采样值小于0
			{
				i++;//下一个采样点	
			}
		}
		else//如果采样值小于0
		{
			while(InDataBuf[i] < 0)//直到采样值大于?
			{
				i++;//下一个采样点				
			}
		}
		RatioOfZorePassage = (float(abs(InDataBuf[i]))) 
			/ ( float(abs(InDataBuf[i - 1])) + float(abs(InDataBuf[i])) );

		//记下当前波过零点之间的宽度	
		if(i == 590)  
			i = i+0;
		LengthOfZorePassage =LastRatio  + (i - datastart - 1) + (1 - RatioOfZorePassage); 
		if(( LengthOfZorePassage >=  (3.0/ 2.0)*((float) MobileType+1.0) )
			&&(LengthOfZorePassage<(12.0/ 3.0)*((float) MobileType+1.0))) //如果是大波	

		{
			if(bit0flag == 1)
			{
				DataHead = 0;//如果小波之后是大波，则重新开始找头
				bit0flag = 0;
			}
			DataHead = DataHead<<1;
			DataHead &= 0xFFFFFFFE;	
			//	0xEFEF0101
		}
		else if((LengthOfZorePassage>= (1.0/3.0)*((float) MobileType+1.0))
			&&(LengthOfZorePassage< (3.0/2.0)*((float) MobileType+1.0))&&(i != *endlen)) //如果是小波
		{
			if(bit0flag == 0)//如果是第一个小波
			{
				bit0flag = 1;
				continue;
			}
			else//如果是第二个小波
			{
				DataHead = DataHead<<1;		
				DataHead |= 0x00000001;
				//	0xEFEF0101
				bit0flag = 0;//两个小波为"1"
			}			
		}
		//else if(LengthOfZorePassage < (2.0/3.0)) //如果是超小波，认为是杂波
		else 
		{
			DataHead = 0;//如果其它，则重新开始找头
		}


		if(( DataHead == 0x55550101 )||( DataHead == 0xEFEF0101 ))//这里需要注意，发上去和发下来的同步头是不同的
		{
			printf("\ndatahead = %x \n",DataHead);	//打印数据头
			*endlen = i;
			return 1;//从i点开始，都是数据了
		}
	}
	printf("no head\n");	//找不到同步头
	return 0;
}


/*****************************************
功能:   解出全部据数
本函数调用的函数清单: 无
调用本函数的函数清单: main
输入参数:  *InDataBuf    采样值地址
lenth        总长度
startlen     开始的地方
MmobileType  手机类型
输出参数:  *OutDataBuf   数据保存的地方
*endlen       解到哪点了
函数返回值说明:    0为出错，1为成功解出8位数据
使用的资源 
******************************************/
BYTE CFSKModem::GetAllData(BYTE *OutDataBuf, short *InDataBuf,
									unsigned long lenth,unsigned long *endlen,BYTE MobileType)
{	
	unsigned long i = *endlen - 1;//指向第i个点
	unsigned long j = 0;//指向第j个解出的数据
	unsigned long DataLenth = 0;//整个数据的长度，由数据前两字节表示

	float 	LengthOfZorePassage = 0;//过零点之间宽度
	float	LastRatio = 0;///过零点上一次的比率
	float	RatioOfZorePassage = 0;//过零点这一次的比率
	unsigned long datastart = 0;//当前在解的波的开始点

	BYTE bit0flag = 0;//用于实现两个小波表示1
	BYTE bitindex = 0;//解出来的数据的位数
	unsigned short crc = 0;//用于校验
	/************************************/		
	//下面的参数用于实现0 、1 幅度的对比//
	unsigned short highest = 0;//每一位中，采样值的最高点//
	unsigned long sum0 = 0;//0的总和
	unsigned long sum1 = 0;// 1的总和
	unsigned short number0 = 0;//0的个数
	unsigned short number1 = 0;// 1的个数
	unsigned long k = 0;//用于实现查找每一位中的最高采样率 等 普通循环
	float RatioOf0b1 = 0;// 所有1 中最高采样之和 与 所有0中最高采样之和    的比率。
	/************************************/	
	/************************************/		
	//下面的参数用于实现波形兼容性检查////
	float MaxOf0Wide = 0;//0中宽度偏差之最
	float MaxOf1Wide = 0;// 1中宽度偏差之最
	/************************************/	
	for(;i<lenth ;)
	{   
		datastart = i;//当前波的开始点
		LastRatio = RatioOfZorePassage;//保存上一次的过零点比率
		if(InDataBuf[i] >= 0)//如果采样值大于等于0
		{
			while(InDataBuf[i] >= 0)//直到采样值小于0
			{
				i++;//下一个采样点	
			}
		}
		else//如果采样值小于0
		{
			while(InDataBuf[i] < 0)//直到采样值大于0
			{
				i++;//下一个采样点				
			}
		}
		RatioOfZorePassage = \
			(float(abs(InDataBuf[i]))) / ( float(abs(InDataBuf[i - 1])) + float(abs(InDataBuf[i])) );

		if(i == 28036)
		{
			i = i;
		}
		//记下当前波过零点之间的宽度	
		LengthOfZorePassage =LastRatio  + (i - datastart - 1) + (1 - RatioOfZorePassage);
		if(( LengthOfZorePassage >=  (3.0/ 2.0)*((float) MobileType+1.0) )
			&&(LengthOfZorePassage<(12.0/ 3.0)*((float) MobileType+1.0))) //如果是大波	
		{
			if(bit0flag == 1)//小波后面是大波就是出错了
			{
				printf("\ndata error,0after1 \n");	
				*endlen = i;
				return 0;
			}
			/********************************************************/
			highest = abs(InDataBuf[datastart]);//置初值
			for(k = datastart+1;k < i;k++)//找出该位中的最高采样值
			{
				if( (abs(InDataBuf[k])) > highest )
				{
					highest = abs(InDataBuf[k]);
				}
			}
			if(highest < 300)//最高采样值不应该比300还低
			{
				printf("\ndata error,0 smoll than 300 \n");	
				*endlen = i;
				return 0;
			}
			number0++;//多少个0
			sum0 += highest;//统计0的采样值之和
			/********************************************************/
			if(  fabs(LengthOfZorePassage - (MobileType+1)*2) > fabs(MaxOf0Wide) )
			{
				MaxOf0Wide = (LengthOfZorePassage - (MobileType+1)*2);
			}
			/********************************************************/
			OutDataBuf[j]  &= ~(1<<(7-bitindex));
			bitindex++;
		}

		else if((LengthOfZorePassage>= (1.0/3.0)*((float) MobileType+1.0))
			&&(LengthOfZorePassage< (3.0/2.0)*((float) MobileType+1.0))&&(i != *endlen)) //如果是小波
		{
			/********************************************************/
			if( fabs(LengthOfZorePassage - (MobileType+1)) > fabs(MaxOf1Wide) )
			{
				MaxOf1Wide = (LengthOfZorePassage - (MobileType+1));
			}
			/********************************************************/
			if(bit0flag == 0)//如果是第一个小波
			{
				bit0flag = 1;
				continue;
			}
			else//如果是第二个小波
			{   
				/********************************************************/
				highest = abs(InDataBuf[datastart]);//置初值
				for(k = datastart+1;k < i;k++)//找出该位中的最高采样值
				{
					if( abs(InDataBuf[k]) > highest )
					{	
						highest = abs(InDataBuf[k]);						
					}
				}
				if(highest < 300)//最高采样值不应该比300还低
				{
					printf("\ndata error,1 smoll than 300 \n");	
					*endlen = i;
					return 0;
				}
				number1++;//多少个1
				sum1 += highest;//统计1的采样值之和
				/********************************************************/
				OutDataBuf[j] |= 1<<(7-bitindex);			
				bitindex++;
				bit0flag = 0;//两个小波为"1"
			}			
		}
		else 
		{	
			if(i == *endlen)//第一个波只取了一点，所以肯定是非常小的
			{
				continue;
			}
			printf("\ndata error,too long or small \n");	
			*endlen = i;
			return 0;//出现其它的波，直接认为出错了
		}


		if( bitindex == 8 )//8位1字节
		{
			printf("%02x,", OutDataBuf[j]);//打印数据
			j++;
			bitindex = 0;
		}
		if((j == 1) && (bitindex == 0))// 一开始1个字节是计数器
			this->m_cPackageCount = OutDataBuf[0];
		if((j == 3)&&(bitindex == 0))// 接下来两个字节是数据长度
		{
			DataLenth =  OutDataBuf[1] | (OutDataBuf[2] << 8);
		}
		//if(( j == 4)&&(bitindex == 0))
		if(( j == DataLenth + 3 + 2) && (j >= 3))//全部解出来了
		{
#if 0
			MaxOf0Wide = (3*MaxOf0Wide/(MobileType + 1)/2);
			MaxOf1Wide = (3*MaxOf1Wide/(MobileType + 1));
			printf("\nMaxOf0Wide is %f%%\n",(3*MaxOf0Wide/(MobileType + 1)/2)*100);//打印
			printf("\nMaxOf1Wide is %f%%\n",(3*MaxOf1Wide/(MobileType + 1))*100);//打印
			for(k = 2; k < DataLenth+1; k++)//计算校验，除长度之外，全部异或
			{
				crc ^= OutDataBuf[k];
			}						
			if(crc != OutDataBuf[DataLenth + 1])//如果校验通不过
			{			
				printf("\n CRC error \n");	//打印校验出错
				*endlen = i;
				return 0;//校验出错
			}
#endif
			/*************************************************************************CRC16**/
			crc = OutDataBuf[DataLenth + 3] | (OutDataBuf[DataLenth + 4] << 8);

			/*	BYTE OutDataBuf111[3000] = {0};
			BYTE OutDataBuf1111[4] = {0x55,0x55,0x01,0x01};
			memset(OutDataBuf111, 0,3000);
			memcpy(OutDataBuf111,OutDataBuf1111,4);
			memcpy(OutDataBuf111+4,OutDataBuf,DataLenth);*/
			if(crc != this->CalculateCRC(OutDataBuf+3,DataLenth))//如果校验通不过
			{			
				printf("\n CRC error \n");	//打印校验出错
				*endlen = i;
				return 0;//校验出错
			}
			/***********************************************************************************/


			printf("\n end \n\n");
			*endlen = i;
			return 1;
		}
	}
	printf("\ndata error,no data \n");
	*endlen = i;
	return 0;//没有数据
}

/*****************************************
功能:   滤波 (平均值滤波，让波形更平滑一些)
本函数调用的函数清单: 无
调用本函数的函数清单: Demodulate
输入参数:  *InDataBuf  从该地址开始滤
length      滤这么长
LowF       截取低频
HighF      截取高频
SampleRate 采样率
输出参数:   
函数返回值说明: 无
使用的资源 
******************************************/
/**/
#if 1
void CFSKModem::SmoothingWave(short *InDataBuf,unsigned long length, 
							  unsigned long LowF, unsigned long HighF, unsigned long SampleRate)
{
	unsigned long i, j, k, N;
	long start, end;
	unsigned long l, h;

	/*
	unsigned long NumberOfLow = 0;//小幅度波的个数
	for(;i<length;)
	{   
	NumberOfLow = 0;
	while((InDataBuf[i] < 500)&&(InDataBuf[i] > -500)) //去除 连续 3点小于500的点
	{
	i++;
	NumberOfLow++;
	if(i == length)
	{
	return;
	}
	}
	if(NumberOfLow < 3)//对于中间出现的小幅度波小于3，要退回去
	{
	i -= NumberOfLow;
	}
	//InDataBuf[i] = (InDataBuf[i - 1] + InDataBuf[i] + InDataBuf[i + 1])/3;

	//i++;		
	}*/

#if 0
	for(i = 0; i < length;) {
		FindFrame(InDataBuf + i, length - i, &start, &end);
		if(start >= 0) {
			if(end == -1)
				end = length - i - 1;
			N = GetN(end - start + 512); // 前后各至少填充256点,每点值为0
			memset(this->m_fPoint, 0, 2 * N * sizeof(float));
			for (j = 256; j < N; j ++)	{
				this->m_fPoint[2*j] = InDataBuf[i+start+j-256];
			}
			/* Calculate FFT. */
			cdft(N*2, -1, this->m_fPoint);
			/* Filter */
			l = (unsigned long)(LowF/((float)SampleRate/N));
			h = (unsigned long)(HighF/((float)SampleRate/N));
			for(k = 0; k < l; k ++) {
				this->m_fPoint[2*k] = this->m_fPoint[2*k+1] = 0;
			}
			for(k = h; k < N; k ++) {
				this->m_fPoint[2*k] =  this->m_fPoint[2*k+1] = 0;
			}

			/* Clear time-domain samples and calculate IFFT. */
			memset(InDataBuf+i+start, 0, (end-start)*2);
			icdft(N*2, -1, this->m_fPoint);
			for(k = 0; k < end-start; k ++) {
				InDataBuf[i+start+k] = (short)this->m_fPoint[2*k+256];
			}
			i += end;
		}
		else
			break;
	}
#else
	for(i = 0; i < length;) {
		this->FindFrame(InDataBuf + i, length - i, &start, &end);
		if(start >= 0) {
			if(end == -1)
				end = length - i - 1;
			N = this->GetN(end - start + 512); // 前后各至少填充256点,每点值为0
			memset(this->m_fPoint, 0, 2 * N * sizeof(float));
			for (j = 256; j < end - start + 256; j ++)	{
				this->m_fPoint[j] = InDataBuf[i+start+j-256];
			}
			/* Calculate FFT. */
			rdft(N, 1, this->m_fPoint);
			/* Filter */
			l = (unsigned long)(LowF/((float)SampleRate/N));
			h = (unsigned long)(HighF/((float)SampleRate/N));
			for(k = 0; k < l; k ++) {
				this->m_fPoint[2*k] = this->m_fPoint[2*k+1] = 0;
			}
			for(k = h; k < N; k ++) {
				this->m_fPoint[2*k] =  this->m_fPoint[2*k+1] = 0;
			}

			/* Clear time-domain samples and calculate IFFT. */
			memset(InDataBuf+i+start, 0, (end-start)*2);

			rdft(N, -1, this->m_fPoint);
			for (j = 0; j <= N - 1; j++) {
				//this->m_fPoint[j] *= 2.0/ N;
				this->m_fPoint[j] /= N;
			}

			for(k = 0; k < end-start; k ++) {
				InDataBuf[i+start+k] = (short)this->m_fPoint[k+256];
			}

			i += end;
		}
		else
			break;
	}

#endif
}
#endif

#if 0
void SmoothingWave(short *InDataBuf,unsigned long lenth)
{
	unsigned long i = 0;//指向第i个点
	unsigned long NumberOfLow = 0;//小幅度波的个数
	for(;i<lenth;)
	{   
		NumberOfLow = 0;
		while((InDataBuf[i] < 500)&&(InDataBuf[i] > -500)) //去除 连续 3点小于500的点
		{
			i++;
			NumberOfLow++;
			if(i == lenth)
			{
				return;
			}
		}
		if(NumberOfLow < 3)//对于中间出现的小幅度波小于3，要退回去
		{
			i -= NumberOfLow;
		}
		//InDataBuf[i] = (InDataBuf[i - 2] + InDataBuf[i - 1]*2 + InDataBuf[i]*4 + InDataBuf[i + 1]*2 + InDataBuf[i + 2])/10;
		InDataBuf[i] = (InDataBuf[i - 1] + InDataBuf[i] + InDataBuf[i + 1])/3;
		i++;		
	}
}
#endif
/*****************************************
功能:   去扰  (去除波形中过小的干扰，实质也是滤波)
本函数调用的函数清单: 无
调用本函数的函数清单: Demodulation
输入参数:  *InDataBuf    从该地址开始   
lenth  		 长度	
MobileType	 以该类型来去扰
输出参数:   
函数返回值说明: 无
使用的资源 
******************************************/
void CFSKModem::DisInterference(short *InDataBuf,unsigned long lenth,BYTE MobileType)
{
	unsigned long i = 0;//指向第i个点
	unsigned long j = 0;//用于超小波宽度中采样值取反
	unsigned long NumberOfLow = 0;//小幅度波的个数
	float LengthOfZorePassage = 0;//过零点之间宽度
	float LastRatio = 0;///过零点上一次的比率
	float	RatioOfZorePassage = 0;//过零点这一次的比率


	unsigned long datastart = 0;//当前在解的波的开始点

	for(;i<lenth;)
	{   
		NumberOfLow = 0;
		while((InDataBuf[i] < 500)&&(InDataBuf[i] > -500)) //去除 连续 3点小于500的点
		{
			i++;
			NumberOfLow++;
			if(i == lenth)
			{
				return;
			}
		}
		if(NumberOfLow < 3)//对于中间出现的小幅度波小于3，要退回去
		{
			i -= NumberOfLow;
		}

		datastart = i;//当前波的开始点
		LastRatio = RatioOfZorePassage;//保存上一次的过零点比率
		if(InDataBuf[i] >= 0)//如果采样值大于等于0
		{
			while(InDataBuf[i] >= 0)//直到采样值小于0
			{
				i++;//下一个采样点	
			}
		}
		else//如果采样值小于0
		{
			while(InDataBuf[i] < 0)//直到采样值大于?
			{
				i++;//下一个采样点				
			}
		}
		RatioOfZorePassage = (float(abs(InDataBuf[i]))) 
			/ ( float(abs(InDataBuf[i - 1])) + float(abs(InDataBuf[i])) );

		//记下当前波过零点之间的宽度	
		LengthOfZorePassage =LastRatio  + (i - datastart - 1) + (1 - RatioOfZorePassage);
		if(LengthOfZorePassage <  ((float)MobileType+1.0)/3.0) //如果是超小波，认为是干扰		
		{
			for(j = datastart;j < i;j++)
			{
				InDataBuf[j] = 0 - InDataBuf[j];
			}			
		}		

	}

}

/*****************************************
功能:  解调 从InDataBuf开始Lenth 这么长的数据 里，用MobileType方式，解调出数据，存在OutDataBuf里
反回时，解到哪个点放在OutLenIndix里
本函数调用的函数清单: 无
调用本函数的函数清单: main
输入参数:  *InDataBuf    采样值地址
lenth        总长度
MmobileType  手机类型
输出参数:  *OutDataBuf   数据保存的地方
*OutLenIndix  解到哪里

函数返回值说明:    0:出错，1:没有滤波  2:需要滤波
使用的资源 
******************************************/
int    CFSKModem::Demodulate(BYTE *OutDataBuf, short *InDataBuf,
									  unsigned long lenth,unsigned long *OutLenIndix,BYTE MobileType)
{
	BYTE LoopForSmooth = 0;// 0 是第一次，1是第二次
	BYTE DemodulationResult = 0;// 找同步头和解调的结果，1为成功，0为失败

	for(LoopForSmooth = 0;LoopForSmooth < 2; LoopForSmooth++ )
	{
		if(LoopForSmooth == 1)//两次循环，先不滤波，解不出来再滤波。
		{
			printf("start Smoothing wave\n");//
			unsigned long lLowF = 0;
			unsigned long lHighF = 0;
			lLowF = (unsigned long)((float)(2000*2/(MobileType+1))*(float)(1.0/32.0 * (float)(MobileType+1)+15.0/16.0));
			lHighF = (unsigned long)((float)(15000*2/(MobileType+1))*(float)(1.0/16.0 * (float)(MobileType+1)+7.0/8.0));

			memcpy(InDataBuf,(char*)InDataBuf+lenth*2,lenth*2);
			SmoothingWave(InDataBuf,lenth, lLowF, lHighF, 44100);
			//	SmoothingWave(InDataBuf,lenth);
			*OutLenIndix = 0;
		}
		this->DisInterference(InDataBuf,lenth,MobileType);//去扰
		DemodulationResult = this->FindHead(InDataBuf,lenth,OutLenIndix,MobileType);//找同步头
		if( DemodulationResult == 1)//如果找到了，则解
		{
			DemodulationResult = this->GetAllData(OutDataBuf,InDataBuf,lenth,OutLenIndix,MobileType);//解调
		}

		if(LoopForSmooth == 0)
		{
			if(DemodulationResult == 1)//continue;//第一次解不出来，滤波后再解
			{
				printf("with no need for Smoothing wave\n");//
				return 1;//第一次就解出来了，说明是没有滤波就解出来了
			}
		}
		else if(LoopForSmooth == 1)
		{
			if(DemodulationResult == 0)
			{
				return 0;//第二次还解不出来，出错了
			}
			else
			{
				printf("need Smoothing wave\n");//
				return 2;//第二次才解出来，说明需要滤波
			}
		}

	}
	return 0;//出错了
}
