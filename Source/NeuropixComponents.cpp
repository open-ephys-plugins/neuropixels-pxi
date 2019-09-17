/*
------------------------------------------------------------------

This file is part of the Open Ephys GUI
Copyright (C) 2018 Allen Institute for Brain Science and Open Ephys

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

#include "NeuropixComponents.h"

#define MAXLEN 50

np::NP_ErrorCode errorCode;

NeuropixComponent::NeuropixComponent() : serial_number(-1), part_number(""), version("")
{
}

void NeuropixAPI::getInfo()
{
	unsigned char version_major;
	unsigned char version_minor;
	np::getAPIVersion(&version_major, &version_minor);

	version = String(version_major) + "." + String(version_minor);
}

void Basestation::getInfo()
{

	unsigned char version_major;
	unsigned char version_minor;
	uint16_t version_build;

	errorCode = np::getBSBootVersion(slot, &version_major, &version_minor, &version_build);

	boot_version = String(version_major) + "." + String(version_minor);

	if (version_build != NULL)
		boot_version += ".";
		boot_version += String(version_build);

}


void BasestationConnectBoard::getInfo()
{

	unsigned char version_major;
	unsigned char version_minor;
	uint16_t version_build;

	errorCode = np::getBSCBootVersion(basestation->slot, &version_major, &version_minor, &version_build);

	boot_version = String(version_major) + "." + String(version_minor);

	if (version_build != NULL)
		boot_version += ".";
		boot_version += String(version_build);

	errorCode = np::getBSCVersion(basestation->slot, &version_major, &version_minor);

	version = String(version_major) + "." + String(version_minor);

	errorCode = np::readBSCSN(basestation->slot, &serial_number);

	char pn[MAXLEN];
	np::readBSCPN(basestation->slot, pn, MAXLEN);

	part_number = String(pn);

}

void Headstage::getInfo()
{

	unsigned char version_major;
	unsigned char version_minor;

	errorCode = np::getHSVersion(probe->basestation->slot, probe->port, &version_major, &version_minor);

	version = String(version_major) + "." + String(version_minor);

	errorCode = np::readHSSN(probe->basestation->slot, probe->port, &serial_number);

	char pn[MAXLEN];
	errorCode = np::readHSPN(probe->basestation->slot, probe->port, pn, MAXLEN);

	part_number = String(pn);

}


void Flex::getInfo()
{

	unsigned char version_major;
	unsigned char version_minor;

	errorCode = np::getFlexVersion(probe->basestation->slot, probe->port, &version_major, &version_minor);

	version = String(version_major) + "." + String(version_minor);

	char pn[MAXLEN];
	errorCode = np::readFlexPN(probe->basestation->slot, probe->port, pn, MAXLEN);

	part_number = String(pn);

}


void Probe::getInfo()
{

	errorCode = np::readId(basestation->slot, port, &serial_number);

	char pn[MAXLEN];
	errorCode = np::readProbePN(basestation->slot, port, pn, MAXLEN);

	part_number = String(pn);
}

Probe::Probe(Basestation* bs, signed char port_) : Thread("probe_" + String(port_)), basestation(bs), port(port_), fifoFillPercentage(0.0f)
{

	setStatus(ProbeStatus::DISCONNECTED);
	setSelected(false);

	flex = new Flex(this);
	headstage = new Headstage(this);

	getInfo();

	for (int i = 0; i < 384; i++)
	{
		apGains.add(3); // default = 500
		lfpGains.add(2); // default = 250
		channelMap.add(BANK_SELECT::DISCONNECTED);
	}

	gains.add(50.0f);
	gains.add(125.0f);
	gains.add(250.0f);
	gains.add(500.0f);
	gains.add(1000.0f);
	gains.add(1500.0f);
	gains.add(2000.0f);
	gains.add(3000.0f);

}

void Probe::setStatus(ProbeStatus status)
{
	this->status = status;
}

void Probe::setSelected(bool isSelected_)
{
	isSelected = isSelected_;
}

void Probe::calibrate()
{
	File baseDirectory = File::getSpecialLocation(File::currentExecutableFile).getParentDirectory();
	File calibrationDirectory = baseDirectory.getChildFile("CalibrationInfo");
	File probeDirectory = calibrationDirectory.getChildFile(String(serial_number));

	std::cout << probeDirectory.getFullPathName() << std::endl;

	if (probeDirectory.exists())
	{
		String adcFile = probeDirectory.getChildFile(String(serial_number) + "_ADCCalibration.csv").getFullPathName();
		String gainFile = probeDirectory.getChildFile(String(serial_number) + "_gainCalValues.csv").getFullPathName();
		std::cout << adcFile << std::endl;
		
		errorCode = np::setADCCalibration(basestation->slot, port, adcFile.toRawUTF8());

		if (errorCode == 0)
			std::cout << "Successful ADC calibration." << std::endl;
		else
			std::cout << "Unsuccessful ADC calibration, failed with error code: " << errorCode << std::endl;

		std::cout << gainFile << std::endl;
		
		errorCode = np::setGainCalibration(basestation->slot, port, gainFile.toRawUTF8());

		if (errorCode == 0)
			std::cout << "Successful gain calibration." << std::endl;
		else
			std::cout << "Unsuccessful gain calibration, failed with error code: " << errorCode << std::endl;

		errorCode = np::writeProbeConfiguration(basestation->slot, port, false);
	}
	else {
		// show popup notification window
		String message = "Missing calibration files for probe serial number " + String(serial_number) + ". ADC and Gain calibration files must be located in 'CalibrationInfo\\<serial_number>' folder in the directory where the Open Ephys GUI was launched. The GUI will proceed without calibration.";
		AlertWindow::showMessageBox(AlertWindow::AlertIconType::WarningIcon, "Calibration files missing", message, "OK");
	}
}

void Probe::setChannels(Array<int> channelStatus)
{

	np::NP_ErrorCode ec;

	for (int channel = 0; channel < channelMap.size(); channel++)
	{
		if (channel != 191)
		{
			ec = np::selectElectrode(basestation->slot, port, channel, BANK_SELECT::DISCONNECTED);
		}
	}

	int electrode;
	BANK_SELECT electrode_bank;

	for (int channel = 0; channel < channelMap.size(); channel++)
	{

		if (channel != 191)
		{
			if (channelStatus[channel])
			{
				electrode = channel;
				electrode_bank = BANK_SELECT::BANK_0;
			}
			else if (channelStatus[channel + 384])
			{
				electrode = channel + 384;
				electrode_bank = BANK_SELECT::BANK_1;
			}
			else if (channelStatus[channel + 2 * 384])
			{
				electrode = channel + 2 * 384;
				electrode_bank = BANK_SELECT::BANK_2;
			}
			else
			{
				electrode_bank = BANK_SELECT::DISCONNECTED;
			}

			channelMap.set(channel, electrode_bank);

			ec = np::selectElectrode(basestation->slot, port, channel, electrode_bank);

		}

	}

	std::cout << "Updating electrode settings for"
		<< " slot: " << static_cast<unsigned>(basestation->slot)
		<< " port: " << static_cast<unsigned>(port) << std::endl;

	ec = np::writeProbeConfiguration(basestation->slot, port, false);
	if (!ec == np::SUCCESS)
		std::cout << "Failed to write channel config " << std::endl;
	else
		std::cout << "Successfully wrote channel config " << std::endl;

}

void Probe::setApFilterState(bool disableHighPass)
{
	for (int channel = 0; channel < 384; channel++)
		np::setAPCornerFrequency(basestation->slot, port, channel, disableHighPass);

	errorCode = np::writeProbeConfiguration(basestation->slot, port, false);

	std::cout << "Wrote filter " << int(disableHighPass) << " with error code " << errorCode << std::endl;
}

void Probe::setGains(unsigned char apGain, unsigned char lfpGain)
{
	for (int channel = 0; channel < 384; channel++)
	{
		np::setGain(basestation->slot, port, channel, apGain, lfpGain);
		apGains.set(channel, int(apGain));
		lfpGains.set(channel, int(lfpGain));
	}
		
	errorCode = np::writeProbeConfiguration(basestation->slot, port, false);

	std::cout << "Wrote gain " << int(apGain) << ", " << int(lfpGain) << " with error code " << errorCode << std::endl;
}


void Probe::setReferences(np::channelreference_t refId, unsigned char refElectrodeBank)
{
	for (int channel = 0; channel < 384; channel++)
		np::setReference(basestation->slot, port, channel, refId, refElectrodeBank);

	errorCode = np::writeProbeConfiguration(basestation->slot, port, false);

	std::cout << "Wrote reference " << int(refId) << ", " << int(refElectrodeBank) << " with error code " << errorCode << std::endl;
}


void Probe::run()
{

	//std::cout << "Thread running." << std::endl;

	while (!threadShouldExit())
	{
		

		size_t count = SAMPLECOUNT;

		errorCode = readElectrodeData(
			basestation->slot,
			port,
			&packet[0],
			&count,
			count);

		if (errorCode == np::SUCCESS &&
			count > 0)
		{
			float apSamples[384];
			float lfpSamples[384];

			for (int packetNum = 0; packetNum < count; packetNum++)
			{
				for (int i = 0; i < 12; i++)
				{
					eventCode = packet[packetNum].Status[i] >> 6; // AUX_IO<0:13>

					uint32_t npx_timestamp = packet[packetNum].timestamp[i];

					for (int j = 0; j < 384; j++)
					{
						apSamples[j] = float(packet[packetNum].apData[i][j]) * 1.2f / 1024.0f * 1000000.0f / gains[apGains[j]]; // convert to microvolts

						if (i == 0)
							lfpSamples[j] = float(packet[packetNum].lfpData[j]) * 1.2f / 1024.0f * 1000000.0f / gains[lfpGains[j]]; // convert to microvolts
					}

					ap_timestamp += 1;

					apBuffer->addToBuffer(apSamples, &ap_timestamp, &eventCode, 1);

					if (ap_timestamp % 30000 == 0)
					{
						size_t packetsAvailable;
						size_t headroom;

						np::getElectrodeDataFifoState(
							basestation->slot,
							port,
							&packetsAvailable,
							&headroom);

						//std::cout << "Basestation " << int(basestation->slot) << ", probe " << int(port) << ", packets: " << packetsAvailable << std::endl;

						fifoFillPercentage = float(packetsAvailable) / float(packetsAvailable + headroom);
					}


				}
				lfp_timestamp += 1;

				lfpBuffer->addToBuffer(lfpSamples, &lfp_timestamp, &eventCode, 1);

			}

		}
		else if (errorCode != np::SUCCESS)
		{
			std::cout << "Error code: " << errorCode << "for Basestation " << int(basestation->slot) << ", probe " << int(port) << std::endl;
		}
	}

}

Headstage::Headstage(Probe* probe_) : probe(probe_)
{
	getInfo();
}

Flex::Flex(Probe* probe_) : probe(probe_)
{
	getInfo();
}

BasestationConnectBoard::BasestationConnectBoard(Basestation* bs) : basestation(bs)
{
	getInfo();
}


Basestation::Basestation(int slot_number) : probesInitialized(false)
{

	slot = (unsigned char)slot_number;

	errorCode = np::openBS(slot);

	if (errorCode == np::SUCCESS)
	{

		std::cout << "  Opened BS on slot " << int(slot) << std::endl;

		getInfo();
		basestationConnectBoard = new BasestationConnectBoard(this);

		savingDirectory = File();

		for (signed char port = 1; port <= 4; port++)
		{

			errorCode = np::openProbe(slot, port);

			if (errorCode == np::SUCCESS)
			{
				probes.add(new Probe(this, port));
				probes[probes.size() - 1]->setStatus(ProbeStatus::CONNECTING);
				continue;
			}
			
			errorCode = np::openProbeHSTest(slot, port);

			if (errorCode == np::SUCCESS)
			{
				ScopedPointer<HeadstageTestModule> hst = new HeadstageTestModule(this, port);
				hst->runAll();
				hst->showResults();
			}

		}
		std::cout << "Found " << String(probes.size()) << (probes.size() == 1 ? " probe." : " probes.") << std::endl;
	}

	syncFrequencies.add(1);
	syncFrequencies.add(10);
}

void Basestation::init()
{

	for (int i = 0; i < probes.size(); i++)
	{
		std::cout << "Initializing probe " << String(i + 1) << "/" << String(probes.size()) << "...";

		errorCode = np::init(this->slot, probes[i]->port);
		if (errorCode != np::SUCCESS)
			std::cout << "  FAILED!." << std::endl;
		else
		{
			setGains(this->slot, probes[i]->port, 3, 2);
			probes[i]->setStatus(ProbeStatus::CONNECTED);
		}

	}

}

Basestation::~Basestation()
{
	for (int i = 0; i < probes.size(); i++)
	{
		errorCode = np::close(slot, probes[i]->port);
	}

	errorCode = np::closeBS(slot);
}

void Basestation::setSyncAsInput()
{

	errorCode = np::setTriggerInput(slot, np::TRIGIN_SW);
	if (errorCode != np::SUCCESS)
	{
		printf("Failed to set slot %d trigger as input!\n");
		return;
	}

	errorCode = setParameter(np::NP_PARAM_SYNCMASTER, slot);
	if (errorCode != np::SUCCESS)
	{
		printf("Failed to set slot %d as sync master!\n");
		return;
	}

	errorCode = setParameter(np::NP_PARAM_SYNCSOURCE, np::TRIGIN_SMA);
	if (errorCode != np::SUCCESS)
		printf("Failed to set slot %d SMA as sync source!\n");

	errorCode = setTriggerOutput(slot, np::TRIGOUT_PXI1, np::TRIGIN_SW);
	if (errorCode != np::SUCCESS)
	{
		printf("Failed to reset sync on SMA output on slot: %d\n", slot);
	}


}

Array<int> Basestation::getSyncFrequencies()
{
	return syncFrequencies;
}

void Basestation::setSyncAsOutput(int freqIndex)
{

	errorCode = setParameter(np::NP_PARAM_SYNCMASTER, slot);
	if (errorCode != np::SUCCESS)
	{
		printf("Failed to set slot %d as sync master!\n", slot);
		return;
	} 

	errorCode = setParameter(np::NP_PARAM_SYNCSOURCE, np::TRIGIN_SYNCCLOCK);
	if (errorCode != np::SUCCESS)
	{
		printf("Failed to set slot %d internal clock as sync source!\n", slot);
		return;
	}

	int freq = syncFrequencies[freqIndex];

	printf("Setting slot %d sync frequency to %d Hz...\n", slot, freq);
	errorCode = setParameter(np::NP_PARAM_SYNCFREQUENCY_HZ, freq);
	if (errorCode != np::SUCCESS)
	{
		printf("Failed to set slot %d sync frequency to %d Hz!\n", slot, freq);
		return;
	}

	errorCode = setTriggerOutput(slot, np::TRIGOUT_SMA, np::TRIGIN_SHAREDSYNC);
	if (errorCode != np::SUCCESS)
	{
		printf("Failed to set sync on SMA output on slot: %d\n", slot);
	}

}

int Basestation::getProbeCount()
{
	return probes.size();
}

float Basestation::getFillPercentage()
{
	float perc = 0.0;

	for (int i = 0; i < getProbeCount(); i++)
	{
		//std::cout << "Percentage for probe " << i << ": " << probes[i]->fifoFillPercentage << std::endl;

		if (probes[i]->fifoFillPercentage > perc)
			perc = probes[i]->fifoFillPercentage;
	}

	return perc;
}

void Basestation::initializeProbes()
{
	if (!probesInitialized)
	{
		errorCode = np::setTriggerInput(slot, np::TRIGIN_SW);

		for (int i = 0; i < probes.size(); i++)
		{
			errorCode = np::setOPMODE(slot, probes[i]->port, np::RECORDING);
			errorCode = np::setHSLed(slot, probes[i]->port, false);

			probes[i]->calibrate();

			if (errorCode == np::SUCCESS)
			{
				std::cout << "     Probe initialized." << std::endl;
				probes[i]->ap_timestamp = 0;
				probes[i]->lfp_timestamp = 0;
				probes[i]->eventCode = 0;
				probes[i]->setStatus(ProbeStatus::CONNECTED);
			}
			else {
				std::cout << "     Failed with error code " << errorCode << std::endl;
			}

		}

		probesInitialized = true;
	}

	errorCode = np::arm(slot);

	
}

void Basestation::startAcquisition()
{
	for (int i = 0; i < probes.size(); i++)
	{
		std::cout << "Probe " << int(probes[i]->port) << " setting timestamp to 0" << std::endl;
		probes[i]->ap_timestamp = 0;
		probes[i]->lfp_timestamp = 0;
		//std::cout << "... and clearing buffers" << std::endl;
		probes[i]->apBuffer->clear();
		probes[i]->lfpBuffer->clear();
		std::cout << "  Starting thread." << std::endl;
		probes[i]->startThread();
	}

	errorCode = np::setSWTrigger(slot);

}

void Basestation::stopAcquisition()
{
	for (int i = 0; i < probes.size(); i++)
	{
		probes[i]->stopThread(1000);
	}

	errorCode = np::arm(slot);
}

void Basestation::setChannels(unsigned char slot_, signed char port, Array<int> channelMap)
{
	if (slot == slot_)
	{
		for (int i = 0; i < probes.size(); i++)
		{
			if (probes[i]->port == port)
			{
				probes[i]->setChannels(channelMap);
				std::cout << "Set electrode-channel connections " << std::endl;
			}
		}
	}
}

void Basestation::setApFilterState(unsigned char slot_, signed char port, bool disableHighPass)
{
	if (slot == slot_)
	{
		for (int i = 0; i < probes.size(); i++)
		{
			if (probes[i]->port == port)
			{
				probes[i]->setApFilterState(disableHighPass);
				std::cout << "Set all filters to " << int(disableHighPass) << std::endl;
			}
		}
	}
	
}

void Basestation::setGains(unsigned char slot_, signed char port, unsigned char apGain, unsigned char lfpGain)
{
	if (slot == slot_)
	{
		for (int i = 0; i < probes.size(); i++)
		{
			if (probes[i]->port == port)
			{
				probes[i]->setGains(apGain, lfpGain);
				std::cout << "Set all gains to " << int(apGain) << ":" << int(lfpGain) << std::endl;
			}
		}
	}
	
}

void Basestation::setReferences(unsigned char slot_, signed char port, np::channelreference_t refId, unsigned char refElectrodeBank)
{
	if (slot == slot_)
	{
		for (int i = 0; i < probes.size(); i++)
		{
			if (probes[i]->port == port)
			{
				probes[i]->setReferences(refId, refElectrodeBank);
				std::cout << "Set all references to " << refId << ":" << int(refElectrodeBank) << std::endl;
			}
		}
	}

}

void Basestation::setSavingDirectory(File directory)
{
	savingDirectory = directory;
}

File Basestation::getSavingDirectory()
{
	return savingDirectory;
}

/****************Headstage Test Module**************************/

HeadstageTestModule::HeadstageTestModule(Basestation* bs, signed char port) : basestation(bs)
{

	slot = basestation->slot;

	this->port = port;

	tests = {
		"VDDA1V2",
		"VDDA1V8",
		"VDDD1V2",
		"VDDD1V8",
		"MCLK", 
		"PCLK",
		"PSB",
		"I2C",
		"NRST",
		"REC_NRESET",
		"SIGNAL"
	};

}

void HeadstageTestModule::getInfo()
{
	//TODO?
}

void HeadstageTestModule::runAll()
{

	status = new HST_Status();

	status->VDD_A1V2 	= test_VDD_A1V2();
	status->VDD_A1V8 	= test_VDD_A1V8();
	status->VDD_D1V2 	= test_VDD_D1V2();
	status->VDD_D1V8 	= test_VDD_D1V8();
	status->MCLK 		= test_MCLK();
	status->PCLK 		= test_PCLK();
	status->PSB 		= test_PSB();
	status->I2C 		= test_I2C();
	status->NRST 		= test_NRST();
	status->REC_NRESET 	= test_REC_NRESET();
	status->SIGNAL 		= test_SIGNAL();

}

void HeadstageTestModule::showResults()
{

	int numTests = sizeof(struct HST_Status)/sizeof(np::NP_ErrorCode);
	int resultLineTextLength = 30;
	String message = "Test results from HST module on slot: " + String(slot) + " port: " + String(port) + "\n\n"; 

	np::NP_ErrorCode *results = (np::NP_ErrorCode*)(&status->VDD_A1V2);
	for (int i = 0; i < numTests; i++)
	{
		message+=String(tests[i]);
		for (int ii = 0; ii < resultLineTextLength - tests[i].length(); ii++) message+="-"; 
		message+=results[i] == np::SUCCESS ? "PASSED" : "FAILED w/ error code: " + String(results[i]);
		message+= "\n";
	}

	AlertWindow::showMessageBox(AlertWindow::AlertIconType::InfoIcon, "HST Module Detected!", message, "OK");

}

np::NP_ErrorCode HeadstageTestModule::test_VDD_A1V2()
{
	return np::HSTestVDDA1V2(slot, port);
}

np::NP_ErrorCode HeadstageTestModule::test_VDD_A1V8()
{
	return np::HSTestVDDA1V8(slot, port);
}

np::NP_ErrorCode HeadstageTestModule::test_VDD_D1V2()
{
	return np::HSTestVDDD1V2(slot, port);
}

np::NP_ErrorCode HeadstageTestModule::test_VDD_D1V8()
{
	return np::HSTestVDDD1V8(slot, port);
}

np::NP_ErrorCode HeadstageTestModule::test_MCLK()
{
	return np::HSTestMCLK(slot, port);
}

np::NP_ErrorCode HeadstageTestModule::test_PCLK()
{
	return np::HSTestPCLK(slot, port);
}

np::NP_ErrorCode HeadstageTestModule::test_PSB()
{
	return np::HSTestPSB(slot, port);
}

np::NP_ErrorCode HeadstageTestModule::test_I2C()
{
	return np::HSTestI2C(slot, port);
}

np::NP_ErrorCode HeadstageTestModule::test_NRST()
{
	return np::HSTestNRST(slot, port);
}

np::NP_ErrorCode HeadstageTestModule::test_REC_NRESET()
{
	return np::HSTestREC_NRESET(slot, port);
}

np::NP_ErrorCode HeadstageTestModule::test_SIGNAL()
{
	return np::HSTestOscillator(slot, port);
}