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

#include "OneBoxADC.h"
#include "Geometry.h"

#include "../UI/OneBoxInterface.h"

OneBoxADC::OneBoxADC(Basestation* bs) : DataSource(bs)
{

	ui = nullptr;

	channel_count = NUM_ADCS;
	sample_rate = 9300.0f;

	sourceType = DataSourceType::ADC;
	status = SourceStatus::CONNECTED;

	Neuropixels::ADC_setVoltageRange(basestation->slot, Neuropixels::ADC_RANGE_5V);
	bitVolts = 5.0f / float(pow(2, 15));
	inputRange = AdcInputRange::PLUSMINUS5V;

	for (int i = 0; i < channel_count; i++)
	{
		outputChannel.add(-1);
		isOutput.add(false);
		
		Neuropixels::DAC_enableOutput(basestation->slot, i, false);
		thresholdLevels.add(AdcThresholdLevel::ONE_VOLT);
		waveplayerTrigger.add(false);
	}

}


void OneBoxADC::initialize(bool signalChainIsLoading)
{
	LOGD("Initializing OneBoxADC");

}



void OneBoxADC::startAcquisition()
{
	sample_number = 0;
	apBuffer->clear();

	LOGD("  Starting thread.");
	startThread();
}

void OneBoxADC::stopAcquisition()
{
	stopThread(1000);
}

void OneBoxADC::setAsOutput(int selectedOutput, int channel)
{

	if (channel < 0 || channel >= channel_count)
		return;

	if (outputChannel[channel] != -1)
	{
		isOutput.set(outputChannel[channel], false);
	}

	isOutput.set(selectedOutput, true);
	outputChannel.set(channel, selectedOutput);

	
	// TODO: actually set the output

}

int OneBoxADC::getOutputChannel(int channel)
{
	if (channel < 0 || channel >= channel_count)
		return false;

	return outputChannel[channel];
}

Array<int> OneBoxADC::getAvailableChannels(int sourceChannel)
{
	Array<int> availableChannels;

	for (int i = 0; i < channel_count; i++)
	{
		if (!isOutput[i] && i != sourceChannel)
		{
			availableChannels.add(i);
		}
	}

	return availableChannels;
}

void OneBoxADC::setAdcInputRange(AdcInputRange range)
{


	switch (range)
	{
	case AdcInputRange::PLUSMINUS2PT5V:
		Neuropixels::ADC_setVoltageRange(basestation->slot, Neuropixels::ADC_RANGE_2_5V);
		bitVolts =  2.5f / float(pow(2, 15));
		inputRange = AdcInputRange::PLUSMINUS2PT5V;
		break;

	case AdcInputRange::PLUSMINUS5V:
		Neuropixels::ADC_setVoltageRange(basestation->slot, Neuropixels::ADC_RANGE_5V);
		bitVolts = 5.0f / float(pow(2, 15));
		inputRange = AdcInputRange::PLUSMINUS5V;
		break;

	case AdcInputRange::PLUSMINUS10V:
		Neuropixels::ADC_setVoltageRange(basestation->slot, Neuropixels::ADC_RANGE_10V);
		bitVolts = 10.0f / float(pow(2, 15));
		inputRange = AdcInputRange::PLUSMINUS10V;
		break;

	default:
		Neuropixels::ADC_setVoltageRange(basestation->slot, Neuropixels::ADC_RANGE_5V);
		bitVolts = 5.0f / float(pow(2, 15));
		inputRange = AdcInputRange::PLUSMINUS5V;
		break;

	}
}

AdcInputRange OneBoxADC::getAdcInputRange()
{
	return inputRange;
}

float OneBoxADC::getChannelGain(int channel)
{
	if (channel < 0 || channel >= channel_count)
		return -1;


	return bitVolts;
}

void OneBoxADC::setAdcThresholdLevel(AdcThresholdLevel level, int channel)
{
	if (channel < 0 || channel >= channel_count)
		return;

	switch (level)
	{ 
	case AdcThresholdLevel::ONE_VOLT:
		Neuropixels::ADC_setComparatorThreshold(basestation->slot, 
			channel, 0.5f, 1.0f);
		thresholdLevels.set(channel, AdcThresholdLevel::ONE_VOLT);
		break;
	case AdcThresholdLevel::THREE_VOLTS:
		Neuropixels::ADC_setComparatorThreshold(basestation->slot, 
						channel, 1.5f, 3.0f);
		thresholdLevels.set(channel, AdcThresholdLevel::THREE_VOLTS);
		break;
	default:
		Neuropixels::ADC_setComparatorThreshold(basestation->slot, 
						channel, 0.5f, 1.0f);
		thresholdLevels.set(channel, AdcThresholdLevel::ONE_VOLT);
	}

}


AdcThresholdLevel OneBoxADC::getAdcThresholdLevel(int channel)
{
	if (channel < 0 || channel >= channel_count)
		return AdcThresholdLevel::ONE_VOLT;

	return thresholdLevels[channel];
}


void OneBoxADC::setTriggersWaveplayer(bool shouldTrigger, int channel)
{
	if (channel < 0 || channel >= channel_count)
		return;

	waveplayerTrigger.set(channel, shouldTrigger);

	LOGD("Setting channel ", channel, " to trigger waveplayer: ", shouldTrigger);

	// configure trigger
}


bool OneBoxADC::getTriggersWaveplayer(int channel)
{
	if (channel < 0 || channel >= channel_count)
		return false;

	return waveplayerTrigger[channel];
}

void OneBoxADC::run()
{

	int16_t data[SAMPLECOUNT * NUM_ADCS];

	double ts_s;

	Neuropixels::PacketInfo packetInfo[SAMPLECOUNT];

	while (!threadShouldExit())
	{
		int count = SAMPLECOUNT;

		errorCode = Neuropixels::ADC_readPackets(basestation->slot, 
			&packetInfo[0],
			&data[0], 
			NUM_ADCS,
			count,
			&count);

		if (errorCode == Neuropixels::SUCCESS && count > 0)
		{
			float adcSamples[NUM_ADCS];

			for (int packetNum = 0; packetNum < count; packetNum++)
			{

				uint64 eventCode = packetInfo[packetNum].Status >> 6; 
				uint32_t adcThresholdStates;

				uint32_t npx_timestamp = packetInfo[packetNum].Timestamp;

				for (int j = 0; j < NUM_ADCS; j++)
				{
					
					adcSamples[j] = float(data[packetNum * NUM_ADCS + j]) * bitVolts; // convert to volts

				}

				Neuropixels::ADC_readComparators(basestation->slot, &adcThresholdStates);

				//eventCode = eventCode | (adcThresholdStates << 1);

				sample_number += 1;

				apBuffer->addToBuffer(adcSamples, &sample_number, &ts_s, &eventCode, 1);

				/*if (ap_timestamp % 30000 == 0)
				{
					int packetsAvailable;
					int headroom;

					Neuropixels::getElectrodeDataFifoState(
						basestation->slot,
						headstage->port,
						dock,
						&packetsAvailable,
						&headroom);

					//std::cout << "Basestation " << int(basestation->slot) << ", probe " << int(port) << ", packets: " << packetsAvailable << std::endl;

					fifoFillPercentage = float(packetsAvailable) / float(packetsAvailable + headroom);
				}*/

			}

		}
		else if (errorCode != Neuropixels::SUCCESS)
		{
			LOGD("readPackets error code: ", errorCode, " for ADCs");
		}
	}

}
