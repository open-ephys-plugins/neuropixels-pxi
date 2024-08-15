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

#include <thread>

float FirmwareUpdater::totalFirmwareBytes = 0;
FirmwareUpdater* FirmwareUpdater::currentThread = nullptr;


Probe::Probe(Basestation* bs_, Headstage* hs_, Flex* fl_, int dock_) 
	: DataSource(bs_),
	  headstage(hs_),
	  flex(fl_),
	  dock(dock_),
	  isValid(true),
	  isCalibrated(false),
      calibrationWarningShown(false)
{

	for (int i = 0; i < 12 * MAXPACKETS; i++)
		timestamp_s[i] = -1.0;

	sourceType = DataSourceType::PROBE;

	for (int i = 0; i < 384; i++)
	{
		for (int j = 0; j < 100; j++)
		{
			ap_offsets[i][j] = 0;
			lfp_offsets[i][j] = 0;
		}
	}

}

void Probe::updateOffsets(float* samples, int64 timestamp, bool isApBand)
{

	if (isApBand && timestamp > 30000 * 5) // wait for amplifiers to settle
	{
		
		if (ap_offset_counter < 99)
		{
			for (int i = 0; i < 384; i++)
			{
				ap_offsets[i][ap_offset_counter+1] = samples[i];
			}

			ap_offset_counter++;
		}
		else if (ap_offset_counter == 99)
		{
			for (int i = 0; i < 384; i++)
			{

				for (int j = 1; j < 100; j++)
				{
					ap_offsets[i][0] += ap_offsets[i][j];
				}
				
				ap_offsets[i][0] /= 99;

			}

			ap_offset_counter++;
		}

	}
	else if (!isApBand && timestamp > 2500 * 5) // wait for amplifiers to settle
	{

		if (lfp_offset_counter < 99)
		{
			for (int i = 0; i < 384; i++)
			{
				lfp_offsets[i][lfp_offset_counter+1] = samples[i];
			}

			lfp_offset_counter++;
		}
		else if (lfp_offset_counter == 99)
		{
			for (int i = 0; i < 384; i++)
			{

				for (int j = 1; j < 100; j++)
				{
					lfp_offsets[i][0] += lfp_offsets[i][j];
				}

				lfp_offsets[i][0] /= 99;

			}

			lfp_offset_counter++;
		}

	}

}

void Probe::updateNamingScheme(ProbeNameConfig::NamingScheme scheme)
{
	namingScheme = scheme;

	switch (scheme)
	{
	case ProbeNameConfig::AUTO_NAMING:
		displayName = customName.automatic;
		break;
	case ProbeNameConfig::STREAM_INDICES:
		displayName = customName.streamSpecific;
		break;
	case ProbeNameConfig::PORT_SPECIFIC_NAMING:
		displayName = basestation->getCustomPortName(headstage->port, dock);
		break;
	case ProbeNameConfig::PROBE_SPECIFIC_NAMING:
		displayName = customName.probeSpecific;
		break;
	}
}


FirmwareUpdater::FirmwareUpdater(Basestation* basestation_, File firmwareFile_, FirmwareType type)
	: ThreadWithProgressWindow("Firmware Update...", true, false),
	  basestation(basestation_),
	  firmwareType(type)
{
	FirmwareUpdater::currentThread = this;
	
	FirmwareUpdater::totalFirmwareBytes = (float) firmwareFile_.getSize();

	auto window = getAlertWindow();
	window->setColour(AlertWindow::textColourId, Colours::white);
	window->setColour(AlertWindow::backgroundColourId, Colour::fromRGB(50, 50, 50));

	firmwareFilePath = firmwareFile_.getFullPathName();

	LOGD("Firmware path: ", firmwareFilePath);

	if (firmwareType == FirmwareType::BSC_FIRMWARE)
	{
		this->setStatusMessage("Updating BSC firmware...");
	}
	else {
		this->setStatusMessage("Updating BS firmware...");
	}

	runThread();

	if (firmwareType == FirmwareType::BSC_FIRMWARE)
		AlertWindow::showMessageBoxAsync(AlertWindow::InfoIcon, "Successful firmware update",
			String("Basestation connect board firmware updated successfully. Please update the basestation firmware now."));
	else
		AlertWindow::showMessageBoxAsync(AlertWindow::InfoIcon, "Successful firmware update",
			String("Please restart your computer and power cycle the PXI chassis for the changes to take effect."));

}

void FirmwareUpdater::run()
{

	if (firmwareType == FirmwareType::BSC_FIRMWARE)
	{

		if (basestation->type == BasestationType::V1)
		{
			np::qbsc_update(basestation->slot_c,
				firmwareFilePath.getCharPointer(),
				firmwareUpdateCallback);
		}
		else if (basestation->type == BasestationType::SIMULATED)
		{
			for (int i = 0; i < 20; i++)
			{
				setProgress(0.05 * i);
				Sleep(100);
			}
		}
		else {
			Neuropixels::bsc_updateFirmware(basestation->slot,
				firmwareFilePath.getCharPointer(),
				firmwareUpdateCallback);
		}
		
	}
		
	else { // BS_FIRMWARE

		if (basestation->type == BasestationType::V1)
		{
			np::bs_update(basestation->slot_c,
				firmwareFilePath.getCharPointer(),
				firmwareUpdateCallback);
		}
		else if (basestation->type == BasestationType::SIMULATED)
		{
			for (int i = 0; i < 20; i++)
			{
				setProgress(0.05 * i);
				Sleep(100);
			}
		}
		else {
			Neuropixels::bs_updateFirmware(basestation->slot,
				firmwareFilePath.getCharPointer(),
				firmwareUpdateCallback);
		}
		
	}
}

void Basestation::updateBscFirmware(File file)
{

	std::unique_ptr<FirmwareUpdater> firmwareUpdater
		= std::make_unique<FirmwareUpdater>((Basestation*)this, file, FirmwareType::BSC_FIRMWARE);

}

void Basestation::updateBsFirmware(File file)
{
	std::unique_ptr<FirmwareUpdater> firmwareUpdater
		= std::make_unique<FirmwareUpdater>((Basestation*)this, file, FirmwareType::BS_FIRMWARE);
}
