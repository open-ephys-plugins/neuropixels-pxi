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
#include <stdio.h>
#include <inttypes.h>

#include "NeuropixAPI.h"
#include "util.h"

#define PROBE_STABILISATION_DELAY_MS    60

#define ANHP_PROBE_PN "PRB3_1_4_32_"

#define PXI_GAMAX                10

#ifdef NP_EMULATIONMODE
#pragma message("API Compiled in EMULATION mode.")
#endif

// TODO: insert minimum compatible BS HW Version
#define BS_HW_VERSION      VERSION(1,0,99)
#define BSC_HW_VERSION     VERSION(1,0,156)

#define HS_SUPPLYRAMPUPDELAY_MS  200

const char* API_timestamp(void) {return __TIMESTAMP__;}

static triggerInputline_t selectedinputline[PXI_GAMAX];
static triggerInputline_t selectedtriggersource[PXI_GAMAX];
static triggerOutputline_t selectedoutputline[PXI_GAMAX];

typedef struct probe* probehandle_t;



EXPORT NP_ErrorCode APIC scanPXI(uint32_t* availableslotmask)
{
	debug_trace(DBG_VERBOSE, "scanPXI : %d", 0);
	*availableslotmask = 0; // emulate no V1 probes present
	return SUCCESS;
}


EXPORT	NP_ErrorCode APIC openBS(uint8_t slotID)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);	
	return SUCCESS;
}

EXPORT	NP_ErrorCode APIC closeBS(unsigned char slotID)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

EXPORT	NP_ErrorCode APIC openEmulationProbe(uint8_t slotID, int8_t port)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}



static NP_ErrorCode openHS(uint8_t slotID, int8_t port,bool openprobe)
{
	debug_trace(DBG_VERBOSE, "slotID=%d, port=%d", slotID,port);
	return SUCCESS;

}

EXPORT	NP_ErrorCode APIC openProbeHSTest(uint8_t slotID, int8_t port)
{
	debug_trace(DBG_VERBOSE, "slotID=%d, port=%d", slotID, port);
	return FAILED; // mark the probe not HS tester
}

FILE* fp;

EXPORT	NP_ErrorCode APIC openProbe(uint8_t slotID, int8_t port)
{
	debug_trace(DBG_VERBOSE, "slotID=%d, port=%d", slotID, port);

	if (slotID == 0 && port == 1) { // only present 1 Probe. Probe indexes are not zero based
		fp = fopen("NP1_PXI_Formatted.bin", "rb");
		return SUCCESS;
	}else {
		return NO_LOCK;
	}
}

EXPORT	NP_ErrorCode APIC init(uint8_t slotID, int8_t port)
{
	debug_trace(DBG_VERBOSE, "slotID=%d, port=%d", slotID, port);
	return SUCCESS;

}



NP_EXPORT NP_ErrorCode NP_APIC getADCparams(unsigned char slotID, unsigned char port, int adcnr, struct ADC_Calib* data)
{
	debug_trace(DBG_VERBOSE, "slotID=%d, port=%d, adcnr=%d", slotID, port,adcnr);
	return SUCCESS;
}

NP_EXPORT NP_ErrorCode NP_APIC setADCparams(unsigned char slotID, unsigned char port, const struct ADC_Calib* data)
{
	return SUCCESS;

}

EXPORT	NP_ErrorCode APIC setADCCalibration(uint8_t slotID, int8_t port, const char * filename)
{
	return SUCCESS;

}


EXPORT NP_ErrorCode APIC getGain(uint8_t slotID, int8_t port, int channel, int* APgainselect, int* LFPgainselect)
{
	return SUCCESS;
}


EXPORT	NP_ErrorCode APIC setGainCalibration(uint8_t slotID, int8_t port, const char * filename)
{
	return SUCCESS;
}


EXPORT	NP_ErrorCode APIC close(uint8_t slotID, int8_t port)
{
	debug_trace(DBG_VERBOSE, "slotID=%d, port=%d", slotID, port);
	return SUCCESS;
}

EXPORT  NP_ErrorCode APIC close_np(uint8_t slotID, int8_t port)
{
    debug_trace(DBG_VERBOSE, "slotID=%d, port=%d", slotID, port);
    
    return close(slotID,port);
}

NP_ErrorCode probe_setlog(probehandle_t ph, int enable) 
{
	debug_trace(DBG_VERBOSE, "enable=%d", enable);

	return SUCCESS;
}

EXPORT	NP_ErrorCode APIC setLog(uint8_t slotID, int8_t port, bool enable)
{
	debug_trace(DBG_VERBOSE, "slotID=%d, port=%d, enable=%d", slotID, port,enable);
	return SUCCESS;
}

EXPORT	NP_ErrorCode APIC writeBSCMM(uint8_t slotID, uint32_t address, uint32_t data)
{
	debug_trace(DBG_VERBOSE, "slotID=%d, address=%d", slotID, address);
	return SUCCESS;
}

EXPORT	NP_ErrorCode APIC readBSCMM(uint8_t slotID, uint32_t address, uint32_t* data)
{
	debug_trace(DBG_VERBOSE, "slotID=%d, address=%d", slotID, address);
	return SUCCESS;
}

EXPORT	NP_ErrorCode APIC writeI2C(uint8_t slotID, int8_t port, uint8_t device, uint8_t address, uint8_t data)
{
	debug_trace(DBG_VERBOSE, "slotID=%d, port=%d,device=%d,adres=%d", slotID, port,device,address);
	return SUCCESS;
}

EXPORT	NP_ErrorCode APIC readI2C(uint8_t slotID, int8_t port, uint8_t device, uint8_t address, uint8_t* data)
{
	debug_trace(DBG_VERBOSE, "slotID=%d, port=%d,device=%d,adres=%d", slotID, port, device, address);
	return SUCCESS;
}

EXPORT	void APIC getAPIVersion(uint8_t* version_major, uint8_t* version_minor)
{
	debug_trace(DBG_VERBOSE, "API_VERSION_MAJOR = %d",1);
	//*version_major = API_VERSION_MAJOR;
	//*version_minor = API_VERSION_MINOR;
	*version_major = 1;
	*version_minor = 0;
}

EXPORT	NP_ErrorCode APIC getBSCBootVersion(uint8_t slotID, uint8_t* version_major, uint8_t* version_minor, uint16_t* version_build)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

EXPORT	NP_ErrorCode APIC getBSBootVersion(uint8_t slotID, uint8_t* version_major, uint8_t* version_minor, uint16_t* version_build)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}



EXPORT	NP_ErrorCode APIC getBSCVersion(uint8_t slotID, uint8_t* version_major, uint8_t* version_minor)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);

	*version_major = 1;
	*version_minor = 0;
	return SUCCESS;
}

EXPORT	NP_ErrorCode APIC setBSCVersion(uint8_t slotID, uint8_t version_major, uint8_t version_minor)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}


EXPORT	NP_ErrorCode APIC readId(uint8_t slotID, int8_t port, uint64_t* id)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

EXPORT	NP_ErrorCode APIC writeId(uint8_t slotID, int8_t port, uint64_t id)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

#ifdef _APPLE_
#include <string.h>

static void strncpy_s(char *dst,unsigned dummy, const char *src, unsigned max_len){
    strlcpy(dst,src,max_len);
}

static void strcpy_s(char *dst, unsigned max_len, const char *src){
    strlcpy(dst,src,max_len);
}

#elif __linux__

//#warning "Linux defined"

#include <string.h>

static void strncpy_s(char *dst,unsigned dummy, const char *src, unsigned max_len){
    strncpy(dst,src,max_len);
}

static void strcpy_s(char *dst, unsigned max_len, const char *src){
    strncpy(dst,src,max_len);
}

#endif



EXPORT	NP_ErrorCode APIC readProbePN(unsigned char slotID, signed char port, char* pn, size_t maxlen) 
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	const char* e_probe_pn = "PRB_1_4_0480_1";


	strcpy_s(pn, maxlen, e_probe_pn);
	return SUCCESS;
}

EXPORT	NP_ErrorCode APIC writeProbePN(unsigned char slotID, signed char port, const char* pn) 
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

EXPORT	NP_ErrorCode APIC getFlexVersion(unsigned char slotID, signed char port, unsigned char* version_major, unsigned char* version_minor) {
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);

	*version_major = 0;
	*version_minor = 0;
	return SUCCESS;

}

EXPORT	NP_ErrorCode APIC setFlexVersion(unsigned char slotID, signed char port, unsigned char version_major, unsigned char version_minor) {
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;

}



EXPORT	NP_ErrorCode APIC readFlexPN(unsigned char slotID, signed char port, char* pn, size_t maxlen)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	strncpy_s(pn, maxlen, "emulFlexPN", maxlen);
	return SUCCESS;

}

EXPORT	NP_ErrorCode APIC writeFlexPN(unsigned char slotID, signed char port, const char* pn) 
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

EXPORT	NP_ErrorCode APIC readHSPN(unsigned char slotID, signed char port, char* pn,size_t maxlen) 
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	strncpy_s(pn, maxlen, "emulHSPN", maxlen);
	return SUCCESS;

}

EXPORT	NP_ErrorCode APIC writeHSPN(unsigned char slotID, signed char port, const char* pn) 
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

EXPORT	NP_ErrorCode APIC readHSSN(uint8_t slotID, int8_t port, uint64_t* sn)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	*sn = 1;
	return SUCCESS;

}

EXPORT	NP_ErrorCode APIC writeHSSN(uint8_t slotID, int8_t port, uint64_t sn)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

EXPORT	NP_ErrorCode APIC getHSVersion(uint8_t slotID, int8_t port, uint8_t* version_major, uint8_t* version_minor)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	*version_major = 1;
	*version_minor = 1;
	return SUCCESS;

}

EXPORT	NP_ErrorCode APIC setHSVersion(uint8_t slotID, int8_t port, uint8_t version_major, uint8_t version_minor)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}




EXPORT	NP_ErrorCode APIC readBSCSN(uint8_t slotID, uint64_t* sn)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	struct BSCID_Layout data;
	
	//data.partnr = "123";
	data.Revision = 1;
	data.Serial = 1;
	data.Version = 1;

	*sn = data.Serial;
	return SUCCESS;
}

EXPORT	NP_ErrorCode APIC writeBSCSN(uint8_t slotID, uint64_t sn)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

EXPORT	NP_ErrorCode APIC readBSCPN(unsigned char slotID, char* pn,size_t maxlen)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	strncpy_s(pn, maxlen, "emulBSCPN", maxlen);
	return SUCCESS;
}

EXPORT	NP_ErrorCode APIC writeBSCPN(unsigned char slotID, const char* pn)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

EXPORT	NP_ErrorCode APIC setHSLed(uint8_t slotID, int8_t port, bool enable)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

EXPORT	NP_ErrorCode APIC setDataMode(uint8_t slotID, bool mode)
{	
	debug_trace(DBG_VERBOSE, "slotID(%i) set datamode to %i.", slotID, mode);
	return SUCCESS;
}

EXPORT	NP_ErrorCode APIC getDataMode(uint8_t slotID, bool* mode)
{	
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

EXPORT	NP_ErrorCode APIC getBSTemperature(uint8_t slotID, float* temperature)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

EXPORT	NP_ErrorCode APIC getBSCTemperature(uint8_t slotID, float* temperature)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

EXPORT	NP_ErrorCode APIC setTestSignal(uint8_t slotID, int8_t port, bool enable)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

EXPORT	NP_ErrorCode APIC setOPMODE(uint8_t slotID, int8_t port, probe_opmode_t mode)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

EXPORT	NP_ErrorCode APIC setCALMODE(uint8_t slotID, int8_t port, testinputmode_t mode)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

EXPORT	NP_ErrorCode APIC setREC_NRESET(uint8_t slotID, int8_t port, bool state)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}



EXPORT	NP_ErrorCode APIC setStdb(uint8_t slotID, int8_t port, unsigned int channel, bool standby)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}


EXPORT	NP_ErrorCode APIC writeProbeConfiguration(uint8_t slotID, int8_t port, bool readCheck)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

EXPORT	NP_ErrorCode APIC arm(uint8_t slotID)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}


EXPORT	NP_ErrorCode APIC getTriggerInput(uint8_t slotID, triggerInputline_t* input)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

EXPORT	NP_ErrorCode APIC setTriggerInput(uint8_t slotID, triggerInputline_t input)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}



EXPORT  NP_ErrorCode APIC setTriggerEdge(unsigned char slotID, bool rising)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

EXPORT	NP_ErrorCode APIC setTriggerBinding(uint8_t slotID, signalline_t outputlines, signalline_t inputlines)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}
EXPORT	NP_ErrorCode APIC getTriggerBinding(uint8_t slotID, signalline_t outputlines, signalline_t* inputlines)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

EXPORT	NP_ErrorCode APIC setTriggerOutput(uint8_t slotID, triggerOutputline_t output, triggerInputline_t input)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

EXPORT	NP_ErrorCode APIC getTriggerOutput(uint8_t slotID, triggerOutputline_t* output, triggerInputline_t* source)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

EXPORT	NP_ErrorCode APIC setSWTrigger(uint8_t slotID)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

struct electrodePacket ep;
EXPORT NP_ErrorCode APIC readElectrodeData(uint8_t slotid, int8_t portid, struct electrodePacket* packets, size_t* actualAmount, size_t requestedAmount)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotid);
	

	if (fp != NULL) {
		*actualAmount = fread(packets, sizeof(struct electrodePacket), 1, fp);
	}else {
		*packets = ep;
		*actualAmount = 1;
	}

	
	

	return SUCCESS;
}

EXPORT NP_ErrorCode APIC getElectrodeDataFifoState(uint8_t slotid, int8_t portid, size_t* packetsavailable, size_t* headroom)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotid);

	*packetsavailable = 1;
	*headroom = 2;
	return SUCCESS;
}

EXPORT size_t APIC readAPFifo(uint8_t slotid, uint8_t portid, uint32_t* timestamps, int16_t* data, int samplecount)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotid);
	return 0;

}

EXPORT size_t APIC readLFPFifo(uint8_t slotid, int8_t portid, uint32_t* timestamps, int16_t* data, int samplecount)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotid);
	return 0;

}

EXPORT size_t APIC readADCFifo(uint8_t slotid, int8_t portid, uint32_t* timestamps, int16_t* data, int samplecount)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotid);
	return 0;
}

/*** File stream API ******/

EXPORT NP_ErrorCode APIC setFileStream(uint8_t slotID, const char* filename)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

EXPORT NP_ErrorCode APIC enableFileStream(uint8_t slotID, bool enable)
{
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

/********************* Parameter configuration functions ****************************/
NP_EXPORT NP_ErrorCode NP_APIC setParameter(np_parameter_t paramid, int value)
{
	debug_trace(DBG_VERBOSE, "paramid=%d", paramid);
	return SUCCESS;
		
}
NP_EXPORT NP_ErrorCode NP_APIC getParameter(np_parameter_t paramid, int* value)
{
	debug_trace(DBG_VERBOSE, "paramid=%d", paramid);
	return SUCCESS;
}
NP_EXPORT NP_ErrorCode NP_APIC setParameter_double(np_parameter_t paramid, double value)
{
	debug_trace(DBG_VERBOSE, "paramid=%d", paramid);
	return SUCCESS;
}
NP_EXPORT NP_ErrorCode NP_APIC getParameter_double(np_parameter_t paramid, double* value)
{
	debug_trace(DBG_VERBOSE, "paramid=%d", paramid);
	return SUCCESS;
}

NP_EXPORT void NP_APIC np_dbg_setlevel(int level) {
	debug_trace(DBG_VERBOSE, "level=%d", level);
}
//------------------------


NP_ErrorCode probe_setAPCornerFrequency(probehandle_t ph, unsigned int channel, bool disableHighPass) {
	debug_trace(DBG_VERBOSE, "channel=%d, disableHighPass : %d", channel, disableHighPass);
	return SUCCESS;
}

NP_EXPORT	NP_ErrorCode NP_APIC setGain(unsigned char slotID, signed char port, unsigned int channel, unsigned char ap_gain, unsigned char lfp_gain) {
	
	static uint32_t prev_channel = -2;

	if (prev_channel + 1 != channel) {
		debug_trace(DBG_VERBOSE, "channel=%d, ap_gain=%d, lfp_gain=%d", channel, ap_gain, lfp_gain);
	}

	prev_channel = channel;
	return SUCCESS;

}

NP_EXPORT	NP_ErrorCode NP_APIC setReference(unsigned char slotID, signed char port, unsigned int channel, channelreference_t reference, uint8_t intRefElectrodeBank) {

	static uint32_t prev_channel = -2;

	if (prev_channel + 1 != channel) {
		debug_trace(DBG_VERBOSE, "channel=%d,reference=%d,bank=%d", channel, reference, intRefElectrodeBank);
	}

	prev_channel = channel;
	return SUCCESS;
}

NP_EXPORT	NP_ErrorCode NP_APIC selectElectrode(unsigned char slotID, signed char port, uint32_t channel, uint8_t electrode_bank){
	
	static uint32_t prev_channel = -2;

	if (prev_channel + 1 != channel) {
		debug_trace(DBG_VERBOSE, "channel=%d, bank=%d", channel, electrode_bank);
	}

	prev_channel = channel;
	return SUCCESS;
}

/********************* Firmware update functions ****************************/
NP_EXPORT NP_ErrorCode NP_APIC qbsc_update(unsigned char  slotID, const char* filename, int(*callback)(size_t byteswritten)) {
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}
NP_EXPORT NP_ErrorCode NP_APIC bs_update(unsigned char  slotID, const char* filename, int(*callback)(size_t byteswritten)) {
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

/*** TEST MODULE FUNCTIONS *****/
/*



*/
NP_EXPORT NP_ErrorCode NP_APIC HSTestREC_NRESET(uint8_t slotID, int8_t port) {
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

NP_EXPORT NP_ErrorCode NP_APIC HSTestNRST(uint8_t slotID, int8_t port) {
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

NP_EXPORT NP_ErrorCode NP_APIC HSTestI2C(uint8_t slotID, int8_t port) {
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

NP_EXPORT NP_ErrorCode NP_APIC HSTestVDDA1V2(uint8_t slotID, int8_t port) {
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}
NP_EXPORT NP_ErrorCode NP_APIC HSTestVDDD1V2(uint8_t slotID, int8_t port) {
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}
NP_EXPORT NP_ErrorCode NP_APIC HSTestVDDA1V8(uint8_t slotID, int8_t port) {
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}
NP_EXPORT NP_ErrorCode NP_APIC HSTestVDDD1V8(uint8_t slotID, int8_t port) {
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}
NP_EXPORT NP_ErrorCode NP_APIC HSTestOscillator(uint8_t slotID, int8_t port) {
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}
NP_EXPORT NP_ErrorCode NP_APIC HSTestMCLK(uint8_t slotID, int8_t port) {
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}
NP_EXPORT NP_ErrorCode NP_APIC HSTestPCLK(uint8_t slotID, int8_t port) {
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}
NP_EXPORT NP_ErrorCode NP_APIC HSTestPSB(uint8_t slotID, int8_t port) {
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

/********************* Built In Self Test ****************************/


NP_EXPORT	NP_ErrorCode NP_APIC bistBS(unsigned char slotID) {
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

NP_EXPORT	NP_ErrorCode NP_APIC bistHB(unsigned char slotID, signed char port) {
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

NP_EXPORT	NP_ErrorCode NP_APIC bistStartPRBS(unsigned char slotID, signed char port) {
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

NP_EXPORT	NP_ErrorCode NP_APIC bistStopPRBS(unsigned char slotID, signed char port, unsigned char* prbs_err) {
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

NP_EXPORT	NP_ErrorCode NP_APIC bistI2CMM(unsigned char slotID, signed char port) {
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

NP_EXPORT	NP_ErrorCode NP_APIC bistEEPROM(unsigned char slotID, signed char port) {
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

NP_EXPORT	NP_ErrorCode NP_APIC bistSR(unsigned char slotID, signed char port) {
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

NP_EXPORT	NP_ErrorCode NP_APIC bistPSB(unsigned char slotID, signed char port) {
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}
NP_EXPORT	NP_ErrorCode NP_APIC bistNoise(unsigned char slotID, signed char port) {
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

NP_EXPORT	NP_ErrorCode NP_APIC bistSignal(uint8_t slotID, int8_t port, bool* pass, struct bistElectrodeStats* stats) {
	debug_trace(DBG_VERBOSE, "slotID=%d", slotID);
	return SUCCESS;
}

//************************

NP_EXPORT	NP_ErrorCode NP_APIC setAPCornerFrequency(unsigned char slotID, signed char port, unsigned int channel, bool disableHighPass) {
	static uint32_t prev_channel = -2;

	if (prev_channel + 1 != channel) {
		debug_trace(DBG_VERBOSE, "channel=%d, disableHighPass : %d", channel, disableHighPass);
	}

	prev_channel = channel;
	return SUCCESS;
}
