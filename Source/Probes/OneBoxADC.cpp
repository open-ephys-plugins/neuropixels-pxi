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
#include "../Utils.h"

#include "../UI/OneBoxInterface.h"

#define MAXLEN 50

void OneBoxADC::getInfo()
{
	//errorCode = Neuropixels::readProbeSN(basestation->slot, headstage->port, dock, &info.serial_number);

	//char pn[MAXLEN];
	//errorCode = Neuropixels::readProbePN(basestation->slot_c, headstage->port_c, dock, pn, MAXLEN);

	//info.part_number = String(pn);
}

OneBoxADC::OneBoxADC(Basestation* bs) : DataSource(bs)
{

	ui = nullptr;

	getInfo();

	//setStatus(ProbeStatus::ADC);

	channel_count = 12;
	sample_rate = 30000.0f;

	errorCode = Neuropixels::NP_ErrorCode::SUCCESS;

	sourceType = DataSourceType::ADC;
	status = SourceStatus::CONNECTED;

	for (int i = 0; i < channel_count; i++)
		channelGains.add(1.0f);

	bitVolts = 5.0f / float(pow(2, 15));

}

bool OneBoxADC::open()
{
	return true;

}

bool OneBoxADC::close()
{
	return true;
}

void OneBoxADC::initialize()
{

	if (open())
	{

		setAdcInputRange(AdcInputRange::PLUSMINUS5V);

		timestamp = 0;

		for (int i = 0; i < 12; i++)
		{
			std::cout << "Initializing ADC" << i << " on slot " << basestation->slot << std::endl;
			Neuropixels::DAC_enableOutput(basestation->slot, i, false);
			Neuropixels::ADC_setVoltageRange(basestation->slot, 5.0);
		}
			
	}

}

void OneBoxADC::enableInput(int chan)
{
	if (ui != nullptr)
	{
		ui->enableInput(chan);
	}
}

void OneBoxADC::disableInput(int chan)
{
	if (ui != nullptr)
	{
		ui->disableInput(chan);
	}
}

void OneBoxADC::startAcquisition()
{
	timestamp = 0;
	apBuffer->clear();

	LOGD("  Starting thread.");
	startThread();
}

void OneBoxADC::stopAcquisition()
{
	stopThread(1000);
}

void OneBoxADC::setAdcInputRange(AdcInputRange range)
{
	switch (range)
	{
	case AdcInputRange::PLUSMINUS2PT5V:
		Neuropixels::ADC_setVoltageRange(basestation->slot, 2.5f);
		bitVolts = 2.5f / float(pow(2, 15));
		break;

	case AdcInputRange::PLUSMINUS5V:
		Neuropixels::ADC_setVoltageRange(basestation->slot, 5.0f);
		bitVolts = 5.0f / float(pow(2, 15));
		break;

	case AdcInputRange::PLUSMINUS10V:
		Neuropixels::ADC_setVoltageRange(basestation->slot, 10.0f);
		bitVolts = 10.0f / float(pow(2, 15));
		break;

	default:
		Neuropixels::ADC_setVoltageRange(basestation->slot, 5.0f);
		bitVolts = 5.0f / float(pow(2, 15));
		break;

	}
}

float OneBoxADC::getChannelGain(int chan)
{
	if (chan < channelGains.size())
	{
		return bitVolts; // channelGains[chan];
	}
	else {
		return bitVolts; // 1.0f;
	}
}

void OneBoxADC::run()
{

	int16_t data[SAMPLECOUNT * 12];

	Neuropixels::PacketInfo packetInfo[SAMPLECOUNT];

	while (!threadShouldExit())
	{
		int count = SAMPLECOUNT;

		errorCode = Neuropixels::ADC_readPackets(basestation->slot, 
			&packetInfo[0],
			&data[0], 
			12,
			count,
			&count);

		if (errorCode == Neuropixels::SUCCESS && count > 0)
		{
			float adcSamples[12];

			for (int packetNum = 0; packetNum < count; packetNum++)
			{

				uint64 eventCode = packetInfo[packetNum].Status >> 6; 

				uint32_t npx_timestamp = packetInfo[packetNum].Timestamp;

				for (int j = 0; j < 12; j++)
				{
					
					adcSamples[j] = (float(data[packetNum * 12 + j]) - 16384.0f) * bitVolts; // convert to volts

					//if (packetNum == 0 && j == 0)
					//{
					//	std::cout << float(data[packetNum * 12 + j]) << " : " << bitVolts << " : " << adcSamples[j] << std::endl;
					//}
				}

				timestamp += 1;

				apBuffer->addToBuffer(adcSamples, &timestamp, &eventCode, 1);

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
