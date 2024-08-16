/*
 * This file is part of the Neuropixels System Developer Distribution (https://github.com/imec-Neuropixels/NP_SYSTEM ).
* Copyright (c) 2020 IMEC vzw, Kapeldreef 75, 3001 Leuven, Belgium.
*
 * This program is distributed under a free license agreement: BY USING OR REDISTRIBUTING THE LICENSED TECHNOLOGY,
* LICENSEE IS AGREEING TO THE TERMS AND CONDITIONS OF THIS LICENSE AGREEMENT.
* IF LICENSEE DOES NOT AGREE WITH THE TERMS AND CONDITIONS OF THE LICENSE AGREEMENT,
* LICENSEE MAY NOT USE OR REDISTRIBUTE THE LICENSED TECHNOLOGY.
*
* You, the licensee, should have received a copy of the Neuropixels System Developer License
 * along with this program. If not, see < https://github.com/imec-Neuropixels/NP_SYSTEM/blob/master/LICENSE/20200608_Neuropixels_System_Developer_License.pdf >.
*/
#ifndef _NEUROPIXAPI_H_
#define _NEUROPIXAPI_H_

#ifdef __cplusplus
extern "C" {
#endif	

#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
#include <Windows.h>
#define NP_EXPORT __declspec(dllexport)
#define NP_APIC __stdcall

#define EXPORT __declspec(dllexport)
#define APIC __stdcall
#else
#define NP_EXPORT
#define NP_APIC
#define EXPORT
#define APIC
#endif




#define PROBE_ELECTRODE_COUNT 960
#define PROBE_CHANNEL_COUNT   384
#define PROBE_SUPERFRAMESIZE  12


#define ELECTRODEPACKET_STATUS_TRIGGER    (1<<0)
#define ELECTRODEPACKET_STATUS_SYNC       (1<<6)

#define ELECTRODEPACKET_STATUS_LFP        (1<<1)
#define ELECTRODEPACKET_STATUS_ERR_COUNT  (1<<2)
#define ELECTRODEPACKET_STATUS_ERR_SERDES (1<<3)
#define ELECTRODEPACKET_STATUS_ERR_LOCK   (1<<4)
#define ELECTRODEPACKET_STATUS_ERR_POP    (1<<5)
#define ELECTRODEPACKET_STATUS_ERR_SYNC   (1<<7)

/*
	Debug std_err output levels
	These values can be set using dbg_setlevel()
*/ 
/* output errors only */
#define DBG_ERROR    4
/* output warnings */
#define DBG_WARNING  3
/* output messages (such as BIST information) */
#define DBG_MESSAGE  2
/* output more detailed background information */
#define DBG_VERBOSE  1
/* output register transactions */
#define DBG_PARANOID 0

struct electrodePacket {
	uint32_t timestamp[PROBE_SUPERFRAMESIZE];
	int16_t apData[PROBE_SUPERFRAMESIZE][PROBE_CHANNEL_COUNT];
	int16_t lfpData[PROBE_CHANNEL_COUNT];
	uint16_t Status[PROBE_SUPERFRAMESIZE];
	//uint16_t SYNC[PROBE_SUPERFRAMESIZE];
};

struct ADC_Calib {
	int ADCnr;
	int CompP;
	int CompN;
	int Slope;
	int Coarse;
	int Fine;
	int Cfix;
	int offset;
	int threshold;
};

/**
* Neuropix API error codes
*/
typedef enum {
	
	SUCCESS = 0,/**< The function returned sucessfully */
	FAILED = 1, /**< Unspecified failure */
	ALREADY_OPEN = 2,/**< A board was already open */
	NOT_OPEN = 3,/**< The function cannot execute, because the board or port is not open */
	IIC_ERROR = 4,/**< An error occurred while accessing devices on the BS i2c bus */
	VERSION_MISMATCH = 5,/**< FPGA firmware version mismatch */
	PARAMETER_INVALID = 6,/**< A parameter had an illegal value or out of range */
	UART_ACK_ERROR = 7,/**< uart communication on the serdes link failed to receive an acknowledgement */
	TIMEOUT = 8,/**< the function did not complete within a restricted period of time */
	WRONG_CHANNEL = 9,/**< illegal channel number */
	WRONG_BANK = 10,/**< illegal electrode bank number */
	WRONG_REF = 11,/**< a reference number outside the valid range was specified */
	WRONG_INTREF = 12,/**< an internal reference number outside the valid range was specified */
	CSV_READ_ERROR = 13,/**< an parsing error occurred while reading a malformed CSV file. */
	BIST_ERROR = 14,/**< a BIST operation has failed */
	FILE_OPEN_ERROR = 15,/**< The file could not be opened */
	READBACK_ERROR = 16,/**< a BIST readback verification failed */
	READBACK_ERROR_FLEX = 17,/**< a BIST Flex EEPROM readback verification failed */
	READBACK_ERROR_HS = 18,/**< a BIST HS EEPROM readback verification failed */
	READBACK_ERROR_BSC = 19,/**< a BIST HS EEPROM readback verification failed */
	TIMESTAMPNOTFOUND = 20,/**< the specified timestamp could not be found in the stream */
	FILE_IO_ERR = 21,/**< a file IO operation failed */
	OUTOFMEMORY = 22,/**< the operation could not complete due to insufficient process memory */
	LINK_IO_ERROR = 23,/**< serdes link IO error */
	NO_LOCK = 24,/**< missing serializer clock. Probably bad cable or connection */
	WRONG_AP = 25,/**< AP gain number out of range */
	WRONG_LFP = 26,/**< LFP gain number out of range */
	ERROR_SR_CHAIN_1 = 27,/**< Validation of SRChain1 data upload failed */
	ERROR_SR_CHAIN_2 = 28,/**< Validation of SRChain2 data upload failed */
	ERROR_SR_CHAIN_3 = 29,/**< Validation of SRChain3 data upload failed */
	PCIE_IO_ERROR = 30,/**< a PCIe data stream IO error occurred. */
	NO_SLOT = 31,/**< no Neuropix board found at the specified slot number */
	WRONG_SLOT = 32,/**<  the specified slot is out of bound */
	WRONG_PORT = 33,/**<  the specified port is out of bound */
	STREAM_EOF = 34,
	HDRERR_MAGIC = 35,
	HDRERR_CRC = 36,
	WRONG_PROBESN = 37,
	WRONG_TRIGGERLINE = 38,
	PROGRAMMINGABORTED = 39, /**<  the flash programming was aborted */
	VALUE_INVALID = 40, /**<  The parameter value is invalid */
	NOTSUPPORTED = 0xFE,/**<  the function is not supported */
	NOTIMPLEMENTED = 0xFF/**<  the function is not implemented */
}NP_ErrorCode;

/**
* Operating mode of the probe
*/
typedef enum {
	RECORDING = 0, /**< Recording mode: (default) pixels connected to channels */
	CALIBRATION = 1, /**< Calibration mode: test signal input connected to pixel, channel or ADC input */
	DIGITAL_TEST = 2, /**< Digital test mode: data transmitted over the PSB bus is a fixed data pattern */
}probe_opmode_t;

/**
* Test input mode
*/
typedef enum {
	PIXEL_MODE = 0, /**< HS test signal is connected to the pixel inputs */
	CHANNEL_MODE = 1, /**< HS test signal is connected to channel inputs */
	NO_TEST_MODE = 2, /**< no test mode */
	ADC_MODE = 3, /**< HS test signal is connected to the ADC inputs */
}testinputmode_t;

typedef enum {
	EXT_REF = 0,  /**< External electrode */
	TIP_REF = 1,  /**< Tip electrode */
	INT_REF = 2   /**< Internal electrode */
}channelreference_t;

typedef enum {
	EMUL_OFF = 0, /**< No emulation data is generated */
	EMUL_STATIC = 1, /**< static data per channel: value = channel number */
	EMUL_LINEAR = 2, /**< a linear ramp is generated per channel (1 sample shift between channels) */
}emulatormode_t;

typedef enum {
	SIGNALLINE_NONE           = 0,
	SIGNALLINE_PXI0           = (1 << 0),
	SIGNALLINE_PXI1           = (1 << 1),
	SIGNALLINE_PXI2           = (1 << 2),
	SIGNALLINE_PXI3           = (1 << 3),
	SIGNALLINE_PXI4           = (1 << 4),
	SIGNALLINE_PXI5           = (1 << 5),
	SIGNALLINE_PXI6           = (1 << 6),
	SIGNALLINE_SHAREDSYNC     = (1 << 7),
	SIGNALLINE_LOCALTRIGGER   = (1 << 8),
	SIGNALLINE_LOCALSYNC      = (1 << 9),
	SIGNALLINE_SMA            = (1 << 10),
	SIGNALLINE_SW             = (1 << 11),
	SIGNALLINE_LOCALSYNCCLOCK = (1 << 12)

}signalline_t;

typedef enum {
	TRIGOUT_NONE = 0,
	TRIGOUT_SMA  = 1, /**< PXI SMA trigger output */
	TRIGOUT_PXI0 = 2, /**< PXI signal line 0 */
	TRIGOUT_PXI1 = 3, /**< PXI signal line 1 */
	TRIGOUT_PXI2 = 4, /**< PXI signal line 2 */
	TRIGOUT_PXI3 = 5, /**< PXI signal line 3 */
	TRIGOUT_PXI4 = 6, /**< PXI signal line 4 */
	TRIGOUT_PXI5 = 7, /**< PXI signal line 5 */
	TRIGOUT_PXI6 = 8, /**< PXI signal line 6 */
}triggerOutputline_t;

typedef enum {
	TRIGIN_SW   = 0, /**< Software trigger selected as input */
	TRIGIN_SMA  = 1, /**< PXI SMA line selected as input */

	TRIGIN_PXI0 = 2, /**< PXI signal line 0 selected as input */
	TRIGIN_PXI1 = 3, /**< PXI signal line 1 selected as input */
	TRIGIN_PXI2 = 4, /**< PXI signal line 2 selected as input */
	TRIGIN_PXI3 = 5, /**< PXI signal line 3 selected as input */
	TRIGIN_PXI4 = 6, /**< PXI signal line 4 selected as input */
	TRIGIN_PXI5 = 7, /**< PXI signal line 5 selected as input */
	TRIGIN_PXI6 = 8, /**< PXI signal line 6 selected as input */
	TRIGIN_SHAREDSYNC = 9, /**< shared sync line selected as input */
	
	TRIGIN_SYNCCLOCK  = 10, /**< Internal SYNC clock */

	TRIGIN_USER1 = 11, /**< User trigger 1 (FUTURE) */
	TRIGIN_USER2 = 12, /**< User trigger 2 (FUTURE) */
	TRIGIN_USER3 = 13, /**< User trigger 3 (FUTURE) */
	TRIGIN_USER4 = 14, /**< User trigger 4 (FUTURE) */
	TRIGIN_USER5 = 15, /**< User trigger 5 (FUTURE) */
	TRIGIN_USER6 = 16, /**< User trigger 6 (FUTURE) */
	TRIGIN_USER7 = 17, /**< User trigger 7 (FUTURE) */
	TRIGIN_USER8 = 18, /**< User trigger 8 (FUTURE) */
	TRIGIN_USER9 = 19, /**< User trigger 9 (FUTURE) */

	TRIGIN_NONE = -1 /**< No trigger input selected */
}triggerInputline_t;


typedef void* np_streamhandle_t;

struct np_sourcestats {
	uint32_t timestamp;
	uint32_t packetcount;
	uint32_t samplecount;
	uint32_t fifooverflow;
};

struct np_diagstats {
	uint64_t totalbytes;         /**< total amount of bytes received */
	uint32_t packetcount;        /**< Amount of packets received */
	uint32_t triggers;           /**< Amount of triggers received */
	uint32_t err_badmagic;		 /**< amount of packet header bad MAGIC markers */
	uint32_t err_badcrc;		 /**< amount of packet header CRC errors */
	uint32_t err_droppedframes;	 /**< amount of dropped frames in the stream */
	uint32_t err_count;			 /**< Every psb frame has an incrementing count index. If the received frame count value is not as expected possible data loss has occured and this flag is raised. */
	uint32_t err_serdes;		 /**< incremented if a deserializer error (hardware pin) occured during receiption of this frame this flag is raised */
	uint32_t err_lock;			 /**< incremented if a deserializer loss of lock (hardware pin) occured during receiption of this frame this flag is raised */
	uint32_t err_pop;			 /**< incremented whenever the ‘next blocknummer’ round-robin FiFo is flagged empty during request of the next value (for debug purpose only, irrelevant for end-user software) */
	uint32_t err_sync;			 /**< Front-end receivers are out of sync. => frame is invalid. */
};

typedef struct {
	uint32_t MAGIC; // includes 'Type' field as lower 4 bits

	uint16_t samplecount;
	uint8_t seqnr;
	uint8_t format;

	uint32_t timestamp;

	uint8_t status;
	uint8_t sourceid;
	uint16_t crc;

}pckhdr_t;


/********************* Parameter configuration functions ****************************/
#define MINSTREAMBUFFERSIZE (1024*32)
#define MAXSTREAMBUFFERSIZE (1024*1024*32)
#define MINSTREAMBUFFERCOUNT (2)
#define MAXSTREAMBUFFERCOUNT (1024)
typedef enum {
	NP_PARAM_BUFFERSIZE       = 1,
	NP_PARAM_BUFFERCOUNT      = 2,
	NP_PARAM_SYNCMASTER       = 3,
	NP_PARAM_SYNCFREQUENCY_HZ = 4,
	NP_PARAM_SYNCPERIOD_MS    = 5,
	NP_PARAM_SYNCSOURCE       = 6,
	NP_PARAM_SIGNALINVERT     = 7,
}np_parameter_t;

NP_EXPORT NP_ErrorCode NP_APIC setParameter(np_parameter_t paramid, int value);
NP_EXPORT NP_ErrorCode NP_APIC getParameter(np_parameter_t paramid, int* value);
NP_EXPORT NP_ErrorCode NP_APIC setParameter_double(np_parameter_t paramid, double value);
NP_EXPORT NP_ErrorCode NP_APIC getParameter_double(np_parameter_t paramid, double* value);

/********************* Opening and initialization functions ****************************/


NP_EXPORT NP_ErrorCode NP_APIC scanPXI(uint32_t* availableslotmask);


NP_EXPORT	NP_ErrorCode NP_APIC openBS(unsigned char slotID);


NP_EXPORT	NP_ErrorCode NP_APIC closeBS(unsigned char slotID);


NP_EXPORT	NP_ErrorCode NP_APIC openProbe(unsigned char slotID, signed char port);

NP_EXPORT	NP_ErrorCode NP_APIC openProbeHSTest(unsigned char slotID, signed char port);

NP_EXPORT	NP_ErrorCode NP_APIC init(unsigned char slotID, signed char port);

NP_EXPORT	NP_ErrorCode NP_APIC setADCCalibration(unsigned char slotID, signed char port, const char* filename);

NP_EXPORT	NP_ErrorCode NP_APIC setGainCalibration(unsigned char slotID, signed char port, const char* filename);

NP_EXPORT	NP_ErrorCode NP_APIC close(unsigned char slotID, signed char port);

NP_EXPORT   NP_ErrorCode NP_APIC close_np(unsigned char slotID, signed char port); // workaround for naming issues of close

NP_EXPORT	NP_ErrorCode NP_APIC setLog(unsigned char slotID, signed char port, bool enable);

NP_EXPORT	NP_ErrorCode NP_APIC writeBSCMM(unsigned char slotID, uint32_t address, uint32_t data);

NP_EXPORT	NP_ErrorCode NP_APIC readBSCMM(unsigned char slotID, uint32_t address, uint32_t* data);

NP_EXPORT	NP_ErrorCode NP_APIC writeI2C(unsigned char slotID, signed char port, unsigned char device, unsigned char address, unsigned char data);

NP_EXPORT	NP_ErrorCode NP_APIC readI2C(unsigned char slotID, signed char port, unsigned char device, unsigned char address, unsigned char* data);

NP_EXPORT	NP_ErrorCode NP_APIC getHSVersion(unsigned char slotID, signed char port, unsigned char* version_major, unsigned char* version_minor);

NP_EXPORT	void  NP_APIC getAPIVersion(unsigned char* version_major, unsigned char* version_minor);

NP_EXPORT	NP_ErrorCode NP_APIC getBSCBootVersion(unsigned char slotID, unsigned char* version_major, unsigned char* version_minor, uint16_t* version_build);

NP_EXPORT	NP_ErrorCode NP_APIC getBSBootVersion(unsigned char slotID, unsigned char* version_major, unsigned char* version_minor, uint16_t* version_build);

NP_EXPORT	NP_ErrorCode NP_APIC getBSCVersion(unsigned char slotID, unsigned char* version_major, unsigned char* version_minor);

NP_EXPORT size_t NP_APIC getLastErrorMessage(char* bufStart, size_t bufsize);
/********************* Serial Numbers ****************************/

NP_EXPORT	NP_ErrorCode NP_APIC readId(unsigned char slotID, signed char port, uint64_t* id);



NP_EXPORT	NP_ErrorCode NP_APIC readProbePN(unsigned char slotID, signed char port, char* pn,size_t len);
NP_EXPORT	NP_ErrorCode NP_APIC getFlexVersion(unsigned char slotID, signed char port, unsigned char* version_major, unsigned char* version_minor);
NP_EXPORT	NP_ErrorCode NP_APIC readFlexPN(unsigned char slotID, signed char port, char* pn, size_t len);

NP_EXPORT	NP_ErrorCode NP_APIC readHSSN(unsigned char slotID, signed char port, uint64_t* sn);




NP_EXPORT	NP_ErrorCode NP_APIC readHSPN(unsigned char slotID, signed char port, char* pn,size_t maxlen);

NP_EXPORT	NP_ErrorCode NP_APIC readBSCSN(unsigned char slotID, uint64_t* sn);

NP_EXPORT	NP_ErrorCode NP_APIC writeBSCSN(unsigned char slotID, uint64_t sn);

NP_EXPORT	NP_ErrorCode NP_APIC readBSCPN(unsigned char slotID,  char* pn, size_t len);
NP_EXPORT	NP_ErrorCode NP_APIC writeBSCPN(unsigned char slotID, const char* pn);

/********************* System Configuration ****************************/


NP_EXPORT	NP_ErrorCode NP_APIC setHSLed(unsigned char slotID, signed char port, bool enable);

NP_EXPORT	NP_ErrorCode NP_APIC setDataMode(unsigned char slotID, bool mode);
NP_EXPORT	NP_ErrorCode NP_APIC getDataMode(unsigned char slotID, bool* mode);

NP_EXPORT	NP_ErrorCode NP_APIC getTemperature(unsigned char slotID, float* temperature);

NP_EXPORT	NP_ErrorCode NP_APIC setTestSignal(unsigned char slotID, signed char port, bool enable);

/********************* Probe Configuration ****************************/

NP_EXPORT	NP_ErrorCode NP_APIC setOPMODE(unsigned char slotID, signed char port, probe_opmode_t mode);

NP_EXPORT	NP_ErrorCode NP_APIC setCALMODE(unsigned char slotID, signed char port,	testinputmode_t mode);

NP_EXPORT	NP_ErrorCode NP_APIC setREC_NRESET(unsigned char slotID, signed char port, bool value);

NP_EXPORT	NP_ErrorCode NP_APIC selectElectrode(unsigned char slotID, signed char port, uint32_t channel, uint8_t electrode_bank);

NP_EXPORT	NP_ErrorCode NP_APIC setReference(unsigned char slotID, signed char port, unsigned int channel, channelreference_t reference, uint8_t intRefElectrodeBank);

NP_EXPORT	NP_ErrorCode NP_APIC setGain(unsigned char slotID, signed char port, unsigned int channel, unsigned char ap_gain, unsigned char lfp_gain);

NP_EXPORT  NP_ErrorCode NP_APIC getGain(uint8_t slotID, int8_t port, int channel, int* APgainselect, int* LFPgainselect);

NP_EXPORT	NP_ErrorCode NP_APIC setAPCornerFrequency(unsigned char slotID, signed char port, unsigned int channel,	bool disableHighPass);

NP_EXPORT	NP_ErrorCode NP_APIC setStdb(unsigned char slotID, signed char port, unsigned int channel, bool standby);

NP_EXPORT	NP_ErrorCode NP_APIC writeProbeConfiguration(unsigned char slotID, signed char port, bool readCheck);

/********************* Trigger Configuration ****************************/

NP_EXPORT	NP_ErrorCode NP_APIC arm(unsigned char slotID);


NP_EXPORT	NP_ErrorCode NP_APIC setTriggerBinding(uint8_t slotID, signalline_t outputline, signalline_t inputlines);
NP_EXPORT	NP_ErrorCode NP_APIC getTriggerBinding(uint8_t slotID, signalline_t outputline, signalline_t* inputlines);

NP_EXPORT	NP_ErrorCode NP_APIC setTriggerInput(uint8_t slotID, triggerInputline_t inputline);
NP_EXPORT	NP_ErrorCode NP_APIC getTriggerInput(uint8_t slotID, triggerInputline_t* input);

NP_EXPORT	NP_ErrorCode NP_APIC setTriggerOutput(uint8_t slotID, triggerOutputline_t line, triggerInputline_t inputline);
NP_EXPORT	NP_ErrorCode NP_APIC getTriggerOutput(uint8_t slotID, triggerOutputline_t* output, triggerInputline_t* source);
//NP_EXPORT	NP_ErrorCode NP_APIC getTriggerOutput(uint8_t slotID, triggerOutputline_t line, triggerInputline_t* inputline);

NP_EXPORT  NP_ErrorCode NP_APIC setTriggerEdge(unsigned char slotID, bool rising);
NP_EXPORT	NP_ErrorCode NP_APIC setSWTrigger(unsigned char slotID);


/********************* Built In Self Test ****************************/

NP_EXPORT	NP_ErrorCode NP_APIC bistBS(unsigned char slotID);
NP_EXPORT	NP_ErrorCode NP_APIC bistHB(unsigned char slotID, signed char port);
NP_EXPORT	NP_ErrorCode NP_APIC bistStartPRBS(unsigned char slotID, signed char port);
NP_EXPORT	NP_ErrorCode NP_APIC bistStopPRBS(unsigned char slotID, signed char port, unsigned char* prbs_err);
NP_EXPORT	NP_ErrorCode NP_APIC bistI2CMM(unsigned char slotID, signed char port);
NP_EXPORT	NP_ErrorCode NP_APIC bistEEPROM(unsigned char slotID, signed char port);
NP_EXPORT	NP_ErrorCode NP_APIC bistSR(unsigned char slotID, signed char port);
NP_EXPORT	NP_ErrorCode NP_APIC bistPSB(unsigned char slotID, signed char port);
NP_EXPORT	NP_ErrorCode NP_APIC bistNoise(unsigned char slotID, signed char port);

struct bistElectrodeStats {
	double peakfreq_Hz;
	double min;
	double max;
	double avg;
};

NP_EXPORT	NP_ErrorCode NP_APIC bistSignal(uint8_t slotID, int8_t port, bool* pass, struct bistElectrodeStats* stats);
/********************* Data Acquisition ****************************/
NP_EXPORT	NP_ErrorCode NP_APIC streamOpenFile(const char* filename, int8_t port, bool lfp, np_streamhandle_t* pstream);
NP_EXPORT	NP_ErrorCode NP_APIC streamClose(np_streamhandle_t stream);
NP_EXPORT	NP_ErrorCode NP_APIC streamSeek(np_streamhandle_t stream, uint64_t filepos, uint32_t* actualtimestamp);
NP_EXPORT	NP_ErrorCode NP_APIC streamSetPos(np_streamhandle_t sh, uint64_t filepos);
NP_EXPORT	uint64_t NP_APIC streamTell(np_streamhandle_t stream);

NP_EXPORT int NP_APIC streamRead(
	np_streamhandle_t sh,
	uint32_t* timestamps,
	int16_t* data,
	int samplecount);

NP_EXPORT NP_ErrorCode NP_APIC streamReadPacket(
	np_streamhandle_t sh,
	pckhdr_t* header,
	int16_t* data,
	size_t elementstoread, 
	size_t* elementread);

NP_EXPORT size_t NP_APIC readAPFifo(uint8_t slotid, uint8_t port, uint32_t* timestamps, int16_t* data, int samplecount);
NP_EXPORT size_t NP_APIC readLFPFifo(unsigned char slotid, signed char port, uint32_t* timestamps, int16_t* data, int samplecount);
NP_EXPORT size_t NP_APIC readADCFifo(unsigned char slotid, signed char port, uint32_t* timestamps, int16_t* data, int samplecount);

NP_EXPORT NP_ErrorCode NP_APIC readElectrodeData(
										unsigned char slotid, 
										signed char port, 
										struct electrodePacket* packets, 
										size_t* actualAmount, 
										size_t requestedAmount);



NP_EXPORT NP_ErrorCode NP_APIC getElectrodeDataFifoState(
										unsigned char slotid, 
										signed char port, 
										size_t* packetsavailable, 
										size_t* headroom);

/*** TEST MODULE FUNCTIONS *****/
NP_EXPORT NP_ErrorCode NP_APIC HSTestVDDA1V2(uint8_t slotID, int8_t port);
NP_EXPORT NP_ErrorCode NP_APIC HSTestVDDD1V2(uint8_t slotID, int8_t port);
NP_EXPORT NP_ErrorCode NP_APIC HSTestVDDA1V8(uint8_t slotID, int8_t port);
NP_EXPORT NP_ErrorCode NP_APIC HSTestVDDD1V8(uint8_t slotID, int8_t port);
NP_EXPORT NP_ErrorCode NP_APIC HSTestOscillator(uint8_t slotID, int8_t port);
NP_EXPORT NP_ErrorCode NP_APIC HSTestMCLK(uint8_t slotID, int8_t port);
NP_EXPORT NP_ErrorCode NP_APIC HSTestPCLK(uint8_t slotID, int8_t port);
NP_EXPORT NP_ErrorCode NP_APIC HSTestPSB(uint8_t slotID, int8_t port);
NP_EXPORT NP_ErrorCode NP_APIC HSTestI2C(uint8_t slotID, int8_t port);
NP_EXPORT NP_ErrorCode NP_APIC HSTestNRST(uint8_t slotID, int8_t port);
NP_EXPORT NP_ErrorCode NP_APIC HSTestREC_NRESET(uint8_t slotID, int8_t port);
NP_EXPORT NP_ErrorCode NP_APIC HSTestPSBShort(uint8_t slotID, int8_t port, int timeout_ms);

/********************* debug output control ****************************/
NP_EXPORT void         NP_APIC dbg_setlevel(int level);
NP_EXPORT int          NP_APIC dbg_getlevel(void);

/********************* Firmware update functions ****************************/
NP_EXPORT NP_ErrorCode NP_APIC qbsc_update(unsigned char  slotID, const char* filename, int(*callback)(size_t byteswritten));
NP_EXPORT NP_ErrorCode NP_APIC bs_update(unsigned char  slotID, const char* filename, int(*callback)(size_t byteswritten));

/********************* Stream API ****************************/
NP_EXPORT NP_ErrorCode NP_APIC setFileStream(unsigned char  slotID, const char* filename);
NP_EXPORT NP_ErrorCode NP_APIC enableFileStream(unsigned char  slotID, bool enable);

NP_EXPORT NP_ErrorCode NP_APIC getADCparams(unsigned char slotID, unsigned char port, int adcnr, struct ADC_Calib* data);
NP_EXPORT NP_ErrorCode NP_APIC setADCparams(unsigned char slotID, unsigned char port, const struct ADC_Calib* data);

NP_EXPORT const char* np_GetErrorMessage(NP_ErrorCode code);

NP_EXPORT void NP_APIC np_dbg_setlevel(int level);

#ifdef __cplusplus
}
#endif	

#define QBSC_PARTNR_CHARLEN 20

struct BSCID_Layout {
	uint8_t Version;
	uint8_t Revision;
	uint64_t Serial;
	char partnr[QBSC_PARTNR_CHARLEN];
};


#endif // _NEUROPIXAPI_H_
