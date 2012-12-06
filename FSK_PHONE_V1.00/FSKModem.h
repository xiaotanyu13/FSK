/************************************************************************/
/*   
FSKModem.h
xiaotanyu13
2012/11/12
xiaot.yu@sunyard.com

封装了FSK调制解调的函数，采用类的方法来实现                                                                   */
/************************************************************************/

#ifndef FSKMODEM_H
#define FSKMODEM_H

#ifndef BYTE
typedef unsigned char BYTE;
#endif

#ifndef DWORD
typedef unsigned long DWORD;
#endif

#define FS				44100			// 采样率

// 发送的频率被固定死了，所以都使用宏来表示
#define MODULATE_FREQUENCY	5512.5		// 调制的频率
#define MODULATE_SAMPLE  (int)(FS / MODULATE_FREQUENCY)	// 调制时候的周期点数
#define AMP_U			1024*24			// 幅度 +
#define AMP_D			-1024*24		// 幅度 -
#define BYTE_LEN		8				// 每个字节的比特数
#define ZVALUEREF		0   //参考点   过零比较
#define E_POINTS		128
#define E_SOUND_THRESHOLD	(1000*E_POINTS)
#define E_SILENCE_THRESHOD	(800*E_POINTS)
#define MAX_N_POINTS		(512*1024)  /* 2**14 */


class CFSKModem
{
private:
	// 调制
	bool m_bIobitFlag;			// if true >0 else <0 ,用来标记每个采样点的正负
	char m_cPackageCount;		// 包计数器，每次发送都会++

	float *m_fPoint;           /* pointer to time-domain samples */

protected:
	int ModulateByte(BYTE byte,short* retData);				// 添加一个字节
	int ModulateBit(int type,short* retData);				// 将一个比特的数据调制成音频数据

	BYTE* PackField(char* data,int len,int *outLen);		// 组报文
	int PackSYNC(BYTE* data);								// 添加同步域
	int PackCount(BYTE* data);								// 添加计数域
	int PackLength(int len,BYTE* data);						// 添加长度域
	int PackData(char* dataIn,int len,BYTE* data);			// 添加数据域
	int PackCRC(short crc,BYTE* data);						// 添加校验域

	void DisInterference(short *InDataBuf,
		unsigned long lenth,BYTE MobileType);				// 去扰
	void SmoothingWave(short *InDataBuf,unsigned long length, 
		unsigned long LowF, unsigned long HighF, 
		unsigned long SampleRate);							// 滤波
	BYTE GetAllData(BYTE *OutDataBuf,
		short *InDataBuf,unsigned long lenth,
		unsigned long *endlen,BYTE MobileType);				// 解出全部数据
	BYTE FindHead(short * InDataBuf,unsigned long lenth,
		unsigned long *endlen,BYTE MobileType);				// 查找同步头
	int GetValidData(short *InDataBuf,short *OutDataBuf,
		unsigned long lenth);								// 
	unsigned long GetN(unsigned long len);					//
	void FindFrame(short *pData, 
		unsigned long len, long *start, long *end);			//

	short CalculateCRC(BYTE *buf,int len);					// 计算CRC

public:
	CFSKModem();
	~CFSKModem();

public:
	short* Modulate(char* data,int len,int* outFrameLen);	// 调制数据  
	int    Demodulate(BYTE *OutDataBuf,
		short *InDataBuf,unsigned long lenth,
		unsigned long *OutLenIndix,BYTE MobileType);		// 解调数据
};


/*
	用来保存wav文件的头
*/
typedef struct _tagMsWavPcmHeader44{
	BYTE ChunkID[4]; // "RIFF"; The "RIFF" the mainchunk;
	unsigned long ChunkSize; // FileSize - 8; The size following this data
	BYTE Format[4]; // "WAVE"; The "WAVE" format consists of two subchunks: "fmt " and "data"

	BYTE SubChunk1ID[4]; // "fmt "
	unsigned long SubChunk1Size; // 16 for PCM. This is the size of the rest of the subchunk which follows this data.
	unsigned short AudioFormat; // 1 for PCM. Linear quantization
	unsigned short NumChannels; // 1->Mono, 2->stereo, etc..
	unsigned long SampleRate; // 8000, 11025, 16000, 44100, 48000, etc..
	unsigned long ByteRate; // = SampleRate * NumChannels * BitsPerSample/8
	unsigned short BlockAlign; // = NumChannels * BitsPerSample / 8
	unsigned short BitsPerSample; // 8->8bits, 16->16bits, etc..

	BYTE SubChunk2ID[4]; // "data"
	unsigned long SubChun2Size; // = NumSamples * NumChannels * BitsPerSample / 8. The size of data
} wav_pcm_header44;
#endif