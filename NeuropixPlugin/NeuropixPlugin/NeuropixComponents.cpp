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

NP_ErrorCode errorCode;

NeuropixComponent::NeuropixComponent() : serial_number(-1), part_number(""), version("")
{
}

void NeuropixAPI::getInfo()
{
	unsigned char version_major;
	unsigned char version_minor;
	getAPIVersion(&version_major, &version_minor);

	version = String(version_major) + "." + String(version_minor);
}

void Basestation::getInfo()
{

	unsigned char version_major;
	unsigned char version_minor;
	uint16_t version_build;

	errorCode = getBSBootVersion(slot, &version_major, &version_minor, &version_build);

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

	errorCode = getBSCBootVersion(basestation->slot, &version_major, &version_minor, &version_build);

	boot_version = String(version_major) + "." + String(version_minor);

	if (version_build != NULL)
		boot_version += ".";
		boot_version += String(version_build);

	errorCode = getBSCVersion(basestation->slot, &version_major, &version_minor);

	version = String(version_major) + "." + String(version_minor);

	errorCode = readBSCSN(basestation->slot, &serial_number);

	char pn[MAXLEN];
	readBSCPN(basestation->slot, pn, MAXLEN);

	part_number = String(pn);

}

void Headstage::getInfo()
{

	unsigned char version_major;
	unsigned char version_minor;

	errorCode = getHSVersion(probe->basestation->slot, probe->port, &version_major, &version_minor);

	version = String(version_major) + "." + String(version_minor);

	errorCode = readHSSN(probe->basestation->slot, probe->port, &serial_number);

	char pn[MAXLEN];
	errorCode = readHSPN(probe->basestation->slot, probe->port, pn, MAXLEN);

	part_number = String(pn);

}


void Flex::getInfo()
{

	unsigned char version_major;
	unsigned char version_minor;

	errorCode = getFlexVersion(probe->basestation->slot, probe->port, &version_major, &version_minor);

	version = String(version_major) + "." + String(version_minor);

	char pn[MAXLEN];
	errorCode = readFlexPN(probe->basestation->slot, probe->port, pn, MAXLEN);

	part_number = String(pn);

}


void Probe::getInfo()
{

	errorCode = readId(basestation->slot, port, &serial_number);

	char pn[MAXLEN];
	errorCode = readProbePN(basestation->slot, port, pn, MAXLEN);

	part_number = String(pn);
}

Probe::Probe(Basestation* bs, signed char port_) : basestation(bs), port(port_), fifoFillPercentage(0.0f)
{
	getInfo();

	flex = new Flex(this);
	headstage = new Headstage(this);
}

void Probe::calibrate()
{
	File baseDirectory = File::getCurrentWorkingDirectory().getFullPathName();
	File calibrationDirectory = baseDirectory.getChildFile("CalibrationInfo");
	File probeDirectory = calibrationDirectory.getChildFile(String(serial_number));

	if (probeDirectory.exists())
	{
		String adcFile = probeDirectory.getChildFile(String(serial_number) + "_ADCCalibration.csv").getFullPathName();
		String gainFile = probeDirectory.getChildFile(String(serial_number) + "_gainCalValues.csv").getFullPathName();
		std::cout << adcFile << std::endl;
		
		errorCode = setADCCalibration(basestation->slot, port, adcFile.toRawUTF8());

		if (errorCode == 0)
			std::cout << "Successful ADC calibration." << std::endl;
		else
			std::cout << "Unsuccessful ADC calibration, failed with error code: " << errorCode << std::endl;

		std::cout << gainFile << std::endl;
		
		errorCode = setGainCalibration(basestation->slot, port, gainFile.toRawUTF8());

		if (errorCode == 0)
			std::cout << "Successful gain calibration." << std::endl;
		else
			std::cout << "Unsuccessful gain calibration, failed with error code: " << errorCode << std::endl;

		errorCode = writeProbeConfiguration(basestation->slot, port, false);
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

	errorCode = openBS(slot);

	savingDirectory = File::getCurrentWorkingDirectory();

	if (errorCode == SUCCESS)
	{
		std::cout << "  Opened BS on slot " << int(slot) << std::endl;

		getInfo();
		basestationConnectBoard = new BasestationConnectBoard(this);

		for (signed char port = 1; port < 5; port++)
		{
			errorCode = openProbe(slot, port);

			std::cout << "    Opening probe " << int(port) << ", error code : " << errorCode << std::endl;

			if (errorCode == SUCCESS)
			{
				probes.add(new Probe(this, port));
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
		errorCode = close(slot, probes[i]->port);
	}

	errorCode = closeBS(slot);
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
	errorCode = setParameter(NP_PARAM_SYNCSOURCE, TRIGIN_SMA);
	errorCode = setParameter(NP_PARAM_SYNCMASTER, slot);
}

void Basestation::initializeProbes()
{
	if (!probesInitialized)
	{
		errorCode = setTriggerInput(slot, TRIGIN_SW);

		for (int i = 0; i < probes.size(); i++)
		{
			errorCode = init(slot, probes[i]->port);
			errorCode = setOPMODE(slot, probes[i]->port, RECORDING);
			//errorCode = setHSLed(slot, probes[i]->port, false);

			//probes[i]->calibrate();

			if (errorCode == SUCCESS)
			{
				std::cout << "     Probe initialized." << std::endl;
			}
			else {
				std::cout << "     Failed with error code " << errorCode << std::endl;
			}

		}

		probesInitialized = true;
	}

	

	errorCode = arm(slot);
	
}

void Basestation::startAcquisition()
{
	for (int i = 0; i < probes.size(); i++)
	{
		std::cout << "Probe " << int(probes[i]->port) << " setting timestamp to 0" << std::endl;
		probes[i]->timestamp = 0;
		//std::cout << "... and clearing buffers" << std::endl;
		probes[i]->apBuffer->clear();
		probes[i]->lfpBuffer->clear();
	}

	errorCode = setSWTrigger(slot);
}

void Basestation::stopAcquisition()
{
	errorCode = arm(slot);
}

void Basestation::setApFilterState(bool filterState)
{
	for (int i = 0; i < probes.size(); i++)
	{
		for (int channel = 0; channel < 384; channel++)
			setAPCornerFrequency(slot, probes[i]->port, channel, filterState);

		errorCode = writeProbeConfiguration(slot, probes[i]->port, false);
	}

	std::cout << "Set all filters to " << int(filterState) << std::endl;
}

void Basestation::setGains(unsigned char apGain, unsigned char lfpGain)
{
	for (int i = 0; i < probes.size(); i++)
	{
		for (int channel = 0; channel < 384; channel++)
			setGain(slot, probes[i]->port, channel, apGain, lfpGain);

		errorCode = writeProbeConfiguration(slot, probes[i]->port, false);
	}

	std::cout << "Set all gains to " << int(apGain) << ":" << int(lfpGain) << std::endl;
}

void Basestation::setReferences(channelreference_t refId, unsigned char refElectrodeBank)
{
	for (int i = 0; i < probes.size(); i++)
	{
		for (int channel = 0; channel < 384; channel++)
			setReference(slot, probes[i]->port, channel, refId, refElectrodeBank);

		errorCode = writeProbeConfiguration(slot, probes[i]->port, false);
	}

	std::cout << "Set all references to " << refId << ":" << int(refElectrodeBank) << std::endl;
}

void Basestation::setSavingDirectory(File directory)
{
	savingDirectory = directory;
}

File Basestation::getSavingDirectory()
{
	return savingDirectory;
}