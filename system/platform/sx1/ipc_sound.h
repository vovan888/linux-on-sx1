
// defines for ipc_audio

#define IPC_DEST_OMAP		1

// AUD_Group
#define IPC_GROUP_DAC		0
#define IPC_GROUP_PCM		1
// cmd for IPC_GROUP_DAC
#define DAC_VOLUME_UPDATE	0
#define DAC_SETAUDIODEVICE	1
#define DAC_OPEN_RING		2
#define DAC_OPEN_DEFAULT	3
#define DAC_CLOSE			4
#define DAC_FMRADIO_OPEN	5
#define DAC_FMRADIO_CLOSE	6
#define DAC_PLAYTONE		7
// cmd for IPC_GROUP_PCM
#define PCM_PLAY			0
#define PCM_RECORD		1
#define PCM_CLOSE			2

// frequencies for MdaDacOpenDefaultL,  MdaDacOpenRingL
#define FRQ_8000	0
#define FRQ_11025	1
#define FRQ_12000	2
#define FRQ_16000	3
#define FRQ_22050	4
#define FRQ_24000	5
#define FRQ_32000	6
#define FRQ_44100	7
#define FRQ_48000	8

