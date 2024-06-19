/*
------------------------------------------------------------------

This file is part of the Open Ephys GUI
Copyright (C) 2024 Allen Institute for Brain Science and Open Ephys

------------------------------------------------------------------

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef __NEUROPIXP2C_H_2C4C2D67__
#define __NEUROPIXP2C_H_2C4C2D67__

#include "../NeuropixComponents.h"

# define MAXPACKETS 64 * 12 * 4

class AcquisitionThread : public Thread
{
public:
	AcquisitionThread(int slot,
		int port,
		int dock,
		int shank,
		DataBuffer* buffer, 
		Probe* probe);

	/** Acquires data from the probe */
	void run() override; // acquire data

private:

	Neuropixels::NP_ErrorCode errorCode;

	int SKIP;

	float apSamples[(384 * 4 + 1) * MAXPACKETS];
	int64 ap_timestamps[MAXPACKETS];
	uint64 event_codes[MAXPACKETS];

	float ap_sample_rate;
	int64 ap_timestamp;

	float ap_offsets[384][100];
	float lfp_offsets[384][100];

	double timestamp_s[12 * MAXPACKETS];

	uint32_t last_npx_timestamp;
	bool passedOneSecond;
	bool sendSync;

	int16_t data[MAXPACKETS * 384];
	uint64 eventCode;

	Neuropixels::streamsource_t source = Neuropixels::SourceAP;

	Neuropixels::PacketInfo packetInfo[MAXPACKETS];

	Neuropixels::streamsource_t stream_source;
	int shank;
	int slot;
	int port;
	int dock;
	DataBuffer* buffer;
	Probe* probe;
	bool invertSyncLine = false;
};

/**

	Acquires data from a Neuropixels QuadBase probe,
	using IMEC's v3 API.

*/
class Neuropixels_QuadBase : public Probe
{
public:

	/** Constructor */
	Neuropixels_QuadBase(Basestation*, 
		Headstage*, 
		Flex*,
		int dock);

	/** Reads probe part number and serial number */
	void getInfo() override;

	/** Opens the connection to the probe (fast) */
	bool open() override;

	/** Closes the connection to the probe (fast) */
	bool close() override;

	/** Call init, setOPMODE, and setHSLED (slow) */
	void initialize(bool signalChainIsLoading) override;

	/** Selects active electrodes based on settings.selectedChannel */
	void selectElectrodes() override;

	/** Select a preset electrode configuration */
	Array<int> selectElectrodeConfiguration(String config);

	/** Sets reference for all channels based on settings.referenceIndex */
	void setAllReferences() override;

	/** Sets gains for all channels based on settings.apGainIndex and settings.lfpGainIndex */
	void setAllGains() override;

	/** Sets AP filter cut based on settings.apFilterState */
	void setApFilterState() override;

	/** Writes latest settings to the probe (slow) */
	void writeConfiguration() override;

	/** Resets timestamps, clears buffers, and starts the thread*/
	void startAcquisition() override;

	/** Stops the thread */
	void stopAcquisition() override;

	/** Runs a built-in self test. */
	bool runBist(BIST bistType) override;

	/** Uploads ADC and gain calibration files */
	void calibrate() override;

	/** Signals that this probe DOES NOT have an LFP data stream*/
	bool generatesLfpData() { return false; }

	/** Signals that this probe DOES NOT have AP filter switch*/
	bool hasApFilterSwitch() { return false; }

	/** Acquisition happens in sub-threads -- this one is not used */
	void run() override { } // not used

private:

	Neuropixels::NP_ErrorCode errorCode;

	OwnedArray<AcquisitionThread> acquisitionThreads;


};


#endif  // __NEUROPIXP2C_H_2C4C2D67__