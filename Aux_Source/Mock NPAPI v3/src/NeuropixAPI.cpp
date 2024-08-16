#include "NeuropixAPI.hpp"

#include <stdio.h>
#include <string.h>
#include "../../Mock NPAPI/src/debugutil.h"

namespace Neuropixels {

	static void fill_basestation_id(basestationID* info) {
		info[0].ID = 1;
		info[0].platformid = NPPlatform_PXI;
	}

	NP_EXPORT void getAPIVersion(int* version_major, int* version_minor){
		debug_trace(DBG_VERBOSE, "");
		*version_major = 1;
		*version_minor = 0;
		
	}
	NP_EXPORT int getDeviceList(basestationID* list, int count)	{
		
		debug_trace(DBG_VERBOSE, "count : %d", count);
	
		fill_basestation_id(list);

		return 1;
	}
	NP_EXPORT NP_ErrorCode getDeviceInfo(int slotID, basestationID* info){
		debug_trace(DBG_VERBOSE, "slotID : %d", slotID);

		fill_basestation_id(info);
		return SUCCESS;
	}
	NP_EXPORT bool tryGetSlotID(const basestationID* bsid, int* slotID)
	{
		debug_trace(DBG_VERBOSE, "");
				
		*slotID = 0;
		return true;
	}
	NP_EXPORT NP_ErrorCode Neuropixels::scanBS(void)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode mapBS(int serialnr, int slot)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode bs_getFirmwareInfo(int slotID, firmware_Info* info)
	{
		debug_trace(DBG_VERBOSE, "");

		info->major = 2;
		info->minor = 0;
		info->build = 1;
		
		strcpy_s(info->name, "XDAQ 1.0");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode bs_updateFirmware(int slotID, const char* filename, int(*callback)(size_t byteswritten))
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode bsc_getFirmwareInfo(int slotID, firmware_Info* info)
	{
		debug_trace(DBG_VERBOSE, "slotID : %d",slotID);

		info->major = 2;
		info->minor = 0;
		info->build = 1;

		strcpy_s(info->name, "XDAQ Firmware 1.0");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode bsc_updateFirmware(int slotID, const char* filename, int(*callback)(size_t byteswritten))
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode getHSSupportedProbeCount(int slotID, int portID, int* count)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode openPort(int slotID, int portID)
	{
		debug_trace(DBG_VERBOSE, "slotID : %d, portID : %d",slotID,portID);
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode closePort(int slotID, int portID)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode detectHeadStage(int slotID, int portID, bool* detected)
	{
		debug_trace(DBG_VERBOSE, "slotID : %d, portID : %d",slotID,portID);

		if (slotID == 0 && portID == 1) { // emulate 1 sensor
			*detected = true;
		} else {
			*detected = false;
		}
		
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode detectFlex(int slotID, int portID, int dockID, bool* detected)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode setHSLed(int slotID, int portID, bool enable)
	{
		debug_trace(DBG_VERBOSE, "enable : %d, slotID : %d, portID : %d",enable,slotID,portID);
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode getFlexVersion(int slotID, int portID, int dockID, int* version_major, int* version_minor)
	{
		debug_trace(DBG_VERBOSE, "");
		*version_major = 1;
		*version_minor = 0;
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode readFlexPN(int slotID, int portID, int dockID, char* pn, size_t maxlen)
	{
		debug_trace(DBG_VERBOSE, "");

		strcpy_s(pn, maxlen, "XDAQ Flex");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode bistNoise(int slotID, int portID, int dockID)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode bistSignal(int slotID, int portID, int dockID)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode HST_GetVersion(int slotID, int portID, int* vmaj, int* vmin)	{
		debug_trace(DBG_VERBOSE, "slotID : %d, portID : %d",slotID,portID);
		return FAILED; // not a test module
	}

	NP_EXPORT NP_ErrorCode HSTestVDDA1V2(int slotID, int portID)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode HSTestVDDD1V2(int slotID, int portID)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode HSTestVDDA1V8(int slotID, int portID)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode HSTestVDDD1V8(int slotID, int portID)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode HSTestOscillator(int slotID, int portID)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode HSTestMCLK(int slotID, int portID)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode HSTestPCLK(int slotID, int portID)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode HSTestPSB(int slotID, int portID)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode HSTestI2C(int slotID, int portID)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode HSTestNRST(int slotID, int portID)
	{
		debug_trace(DBG_VERBOSE, "");
		return  SUCCESS;
	}

	NP_EXPORT NP_ErrorCode HSTestREC_NRESET(int slotID, int portID)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode NP_APIC np_setHSLed(int slotID, int portID, bool enable)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode NP_APIC np_getFlexVersion(int slotID, int portID, int dockID, int* version_major, int* version_minor)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode NP_APIC np_readFlexPN(int slotID, int portID, int dockID, char* pn, size_t maxlen)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode NP_APIC np_getHSVersion(int slotID, int portID, int* version_major, int* version_minor)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode NP_APIC np_readHSSN(int slotID, int portID, uint64_t* sn)
	{
		debug_trace(DBG_VERBOSE, "");
		 return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode NP_APIC np_readProbePN(int slotID, int portID, int dockID, char* pn, size_t maxlen)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode setEmissionSite(int slotID, int portID, int dockID, wavelength_t wavelength, int site)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode getEmissionSite(int slotID, int portID, int dockID, wavelength_t wavelength, int* site)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode openProbe(int slotID, int portID, int dockID)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode closeProbe(int slotID, int portID, int dockID)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode init(int slotID, int portID, int dockID){
		debug_trace(DBG_VERBOSE, "slotID : %d, portID : %d, dockID : %d", slotID, portID, dockID);

		// xdaq lib init 
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode writeProbeConfiguration(int slotID, int portID, int dockID, bool readCheck)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode setADCCalibration(int slotID, int portID, const char* filename)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode setGainCalibration(int slotID, int portID, int dockID, const char* filename)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode readBSCPN(int slotID, char* pn, size_t len)
	{
		debug_trace(DBG_VERBOSE, "slotID : %d",slotID);

		strcpy_s(pn, len, "XDAQ NA");

		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode readBSCSN(int slotID, uint64_t* sn)
	{
		debug_trace(DBG_VERBOSE, "slotID : %d",slotID);

		*sn = 12345;
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode getBSCVersion(int slotID, int* version_major, int* version_minor)
	{
		debug_trace(DBG_VERBOSE, "slotID : %d",slotID);

		*version_major = 1;
		*version_minor = 0;
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode getHSVersion(int slotID, int portID, int* version_major, int* version_minor)
	{
		debug_trace(DBG_VERBOSE, "");

		*version_major = 1;
		*version_minor = 0;

		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode readHSPN(int slotID, int portID, char* pn, size_t maxlen)
	{
		debug_trace(DBG_VERBOSE, "slotID : %d, portID : %d",slotID,portID);

		const char* hs_pn = "NP2_HS_30";
		strcpy_s(pn, maxlen, hs_pn);

		debug_trace(DBG_VERBOSE, "Reporting HS part number : %s", hs_pn);

		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode readHSSN(int slotID, int portID, uint64_t* sn)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode readProbeSN(int slotID, int portID, int dockID, uint64_t* id)
	{
		debug_trace(DBG_VERBOSE, "12345678");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode readProbePN(int slotID, int portID, int dockID, char* pn, size_t maxlen)	{
		debug_trace(DBG_VERBOSE, "slotID : %d, portID : %d, dockID : %d",slotID,portID,dockID);

		const char* probe_pn = "PRB_1_4_0480_1";		
		strcpy_s(pn, maxlen, probe_pn);
		
		debug_trace(DBG_VERBOSE, "Reporting probe part number : %s", probe_pn);
		
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode setParameter(np_parameter_t paramid, int value)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode openBS(int slotID)
	{
		debug_trace(DBG_VERBOSE, "slotID : %d",slotID);
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode closeBS(int slotID)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode arm(int slotID){

		debug_trace(DBG_VERBOSE, "slotID : %d", slotID);
		
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode setSWTrigger(int slotID)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode setSWTriggerEx(int slotID, swtriggerflags_t triggerflags)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode switchmatrix_set(int slotID, switchmatrixoutput_t output, switchmatrixinput_t inputline, bool connect)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT void NP_APIC np_dbg_setlevel(int level)
	{
		debug_trace(DBG_VERBOSE, "");
	}

	NP_EXPORT NP_ErrorCode readElectrodeData(int slotID, int portID, int dockID, electrodePacket* packets, int* actualAmount, int requestedAmount){
		
		debug_trace(DBG_VERBOSE, "slotID : %d, portID : %d, dockID : %d", slotID, portID, dockID);
		
		*actualAmount = 0;
		
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode getElectrodeDataFifoState(int slotID, int portID, int dockID, int* packetsavailable, int* headroom){
		debug_trace(DBG_VERBOSE, "slotID : %d, portID : %d, dockID : %d",slotID,portID,dockID);

		*packetsavailable = 1;
		*headroom = 2;

		return SUCCESS;

		
	}

	NP_EXPORT NP_ErrorCode setOPMODE(int slotID, int portID, int dockID, probe_opmode_t mode){
		

		char *op_mode_desc = "?";
		switch (mode) {
		case RECORDING:
			op_mode_desc = "RECORDING";
			break;
		case CALIBRATION:
			op_mode_desc = "CALIBRATION";
			break;
		case DIGITAL_TEST:
			op_mode_desc = "DIGITAL_TEST";
			break;
		default:
			break;
		}

		debug_trace(DBG_VERBOSE, "opmode : %s, slotID : %d, portID : %d, dockID : %d", op_mode_desc, slotID, portID, dockID);

		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode setGain(int slotID, int portID, int dockID, int channel, int ap_gain, int lfp_gain){
		static uint32_t prev_channel = -2;

		if (prev_channel + 1 != channel) {
			debug_trace(DBG_VERBOSE, "channel=%d, ap_gain=%d, lfp_gain=%d", channel, ap_gain, lfp_gain);
		}

		prev_channel = channel;
		return SUCCESS;
		
	}

	NP_EXPORT NP_ErrorCode getGain(int slotID, int portID, int dockID, int channel, int* APgainselect, int* LFPgainselect)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode selectElectrode(int slotID, int portID, int dockID, int channel, int shank, int bank){
		static uint32_t prev_channel = -2;

		if (prev_channel + 1 != channel) {
			debug_trace(DBG_VERBOSE, "channel=%d, bank=%d", channel, bank);
		}

		prev_channel = channel;
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode setReference(int slotID, int portID, int dockID, int channel, int shank, channelreference_t reference, int intRefElectrodeBank){
		static uint32_t prev_channel = -2;

		if (prev_channel + 1 != channel) {
			debug_trace(DBG_VERBOSE, "channel=%d,reference=%d,bank=%d", channel, reference, intRefElectrodeBank);
		}

		prev_channel = channel;
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode setAPCornerFrequency(int slotID, int portID, int dockID, int channel, bool disableHighPass){
		static uint32_t prev_channel = -2;

		if (prev_channel + 1 != channel) {
			debug_trace(DBG_VERBOSE, "channel=%d, disableHighPass : %d", channel, disableHighPass);
		}

		prev_channel = channel;
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode selectColumnPattern(int slotID, int portID, int dockID, columnpattern_t pattern)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode selectElectrodeGroup(int slotID, int portID, int dockID, int channelgroup, int bank)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode selectElectrodeGroupMask(int slotID, int portID, int dockID, int channelgroup, electrodebanks_t mask)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode waveplayer_writeBuffer(int slotID, const int16_t* data, int len)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode waveplayer_arm(int slotID, bool singleshot)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode waveplayer_setSampleFrequency(int slotID, double frequency_Hz)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode ADC_readPackets(int slotID, PacketInfo* pckinfo, int16_t* data, int channelcount, int packetcount, int* packetsread)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode bistBS(int slotID)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode bistHB(int slotID, int portID, int dockID)
	{
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode bistStartPRBS(int slotID, int portID)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode bistStopPRBS(int slotID, int portID, int* prbs_err)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode bistI2CMM(int slotID, int portID, int dockID)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode bistEEPROM(int slotID, int portID)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode bistSR(int slotID, int portID, int dockID)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode bistPSB(int slotID, int portID, int dockID)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode readPackets(int slotID, int portID, int dockID, streamsource_t source, PacketInfo* pckinfo, int16_t* data, int channelcount, int packetcount, int* packetsread)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode getPacketFifoStatus(int slotID, int portID, int dockID, streamsource_t source, int* packetsavailable, int* headroom)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode NP_APIC np_setOpticalCalibration(int slotID, int portID, int dockID, const char* filename)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode NP_APIC np_setEmissionSite(int slotID, int portID, int dockID, wavelength_t wavelength, int site)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode ADC_setVoltageRange(int slotID, ADCrange_t range)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode DAC_enableOutput(int slotID, int DACChannel, bool state)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

	NP_EXPORT NP_ErrorCode DAC_setProbeSniffer(int slotID, int DACChannel, int portID, int dockID, int channelnr, streamsource_t sourcetype)
	{
		debug_trace(DBG_VERBOSE, "");
		return SUCCESS;
	}

}

