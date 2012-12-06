/*
	auther:xiaotanyu13
	这个版本的函数采用c标准来编写，用于在ipos上面使用
*/

#ifndef FSK_MODEM_IPOS_H
#define FSK_MODEM_IPOS_H

#ifndef BYTE
typedef unsigned char BYTE;
#endif

#ifndef DWORD
typedef unsigned long DWORD;
#endif

#define FS				44100			// 采样率

#define AMP_U			1024*24			// 幅度 +
#define AMP_D			-1024*24		// 幅度 -
#define BYTE_LEN		8				// 每个字节的比特数
#define ZVALUEREF		0   //参考点   过零比较
#define E_POINTS		128
#define E_SOUND_THRESHOLD	(1000*E_POINTS)
#define E_SILENCE_THRESHOD	(800*E_POINTS)
#define MAX_N_POINTS		(512*1024)  /* 2**14 */


short* Modulate(char* data,int len,int* outFrameLen,BYTE MobileType);		// 调制数据  
int    Demodulate(BYTE *OutDataBuf,
				  short *InDataBuf,unsigned long lenth,
				  unsigned long *OutLenIndix);								// 解调数据
void  SetSampleLevel(int level);


/*
	wav格式的文件的头44字节数据
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