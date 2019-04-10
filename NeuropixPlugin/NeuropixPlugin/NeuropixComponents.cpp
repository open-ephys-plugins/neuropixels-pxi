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
	getInfo();

	flex = new Flex(this);
	headstage = new Headstage(this);

	for (int i = 0; i < 384; i++)
	{
		apGains.add(3); // default = 500
		lfpGains.add(2); // default = 250
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

void Probe::setApFilterState(bool filterState)
{
	for (int channel = 0; channel < 384; channel++)
		np::setAPCornerFrequency(basestation->slot, port, channel, filterState);

	errorCode = np::writeProbeConfiguration(basestation->slot, port, false);

	std::cout << "Wrote filter " << int(filterState) << " with error code " << errorCode << std::endl;
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

	savingDirectory = File();

	if (errorCode == np::SUCCESS)
	{
		std::cout << "  Opened BS on slot " << int(slot) << std::endl;

		getInfo();
		basestationConnectBoard = new BasestationConnectBoard(this);

		for (signed char port = 1; port < 5; port++)
		{
			errorCode = np::openProbe(slot, port);

			if (errorCode == np::SUCCESS)
			{
				std::cout << "    Opening probe " << int(port) << ", error code : " << errorCode << std::endl;

				probes.add(new Probe(this, port));
				errorCode = np::init(slot, port);
				setGains(slot, port, 3, 2); // set defaults
				std::cout << "  Success." << std::endl;
			}
		}

	}
	else {
		std::cout << "  Opening BS on slot " << int(slot) << " failed with error code : " << errorCode << std::endl;
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


void Basestation::makeSyncMaster()
{
	errorCode = setParameter(np::NP_PARAM_SYNCSOURCE, np::TRIGIN_SMA);
	errorCode = setParameter(np::NP_PARAM_SYNCMASTER, slot);
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

void Basestation::setApFilterState(unsigned char slot_, signed char port, bool filterState)
{
	if (slot == slot_)
	{
		for (int i = 0; i < probes.size(); i++)
		{
			if (probes[i]->port == port)
			{
				probes[i]->setApFilterState(filterState);
				std::cout << "Set all filters to " << int(filterState) << std::endl;
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