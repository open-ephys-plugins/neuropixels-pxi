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

#include "OneBoxDAC.h"
#include "Geometry.h"

#include "OneBoxADC.h"

#define MAXLEN 50

void OneBoxDAC::getInfo()
{
	//errorCode = Neuropixels::readProbeSN(basestation->slot, headstage->port, dock, &info.serial_number);

	//char pn[MAXLEN];
	//errorCode = Neuropixels::readProbePN(basestation->slot_c, headstage->port_c, dock, pn, MAXLEN);

	//info.part_number = String(pn);
}

OneBoxDAC::OneBoxDAC(Basestation* bs_) : DataSource(bs), bs(bs_)
{

	getInfo();

	//setStatus(ProbeStatus::ADC);

	channel_count = 0;
	sample_rate = 30000.0f;

	errorCode = Neuropixels::NP_ErrorCode::SUCCESS;

	sourceType = DataSourceType::DAC;
	status = SourceStatus::CONNECTED;

	errorCode = Neuropixels::waveplayer_setSampleFrequency(bs->slot, 30000.0f);

}

bool OneBoxDAC::open()
{
	return true;

}

bool OneBoxDAC::close()
{
	return true;
}

void OneBoxDAC::initialize(bool signalChainIsLoading)
{

	if (open())
	{

		//setAdcInputRange(AdcInputRange::PLUSMINUS5V);

		timestamp = 0;
	}

}

void OneBoxDAC::startAcquisition()
{
	
}

void OneBoxDAC::stopAcquisition()
{
	//stopThread(1000);
}

void OneBoxDAC::playWaveform()
{
	errorCode = Neuropixels::setSWTriggerEx(bs->slot, Neuropixels::swtrigger2);

	std::cout << "setSWTriggerEx error code: " << errorCode << std::endl;
}

void OneBoxDAC::stopWaveform()
{
	//errorCode = Neuropixels::setSWTriggerEx(bs->slot, Neuropixels::swtrigger2);

	//std::cout << "setSWTriggerEx error code: " << errorCode << std::endl;
	std::cout << "NOT IMPLEMENTED." << std::endl;
}

void OneBoxDAC::setWaveform(Array<float> samples)
{
	
	Array<int16_t> samples_t;

	for (auto sample : samples)
		samples_t.add(int(sample / 5.0f * 65535));

	errorCode = Neuropixels::waveplayer_writeBuffer(bs->slot, samples_t.getRawDataPointer(), samples_t.size());

	std::cout << "waveplayer_writeBuffer error code: " << errorCode << std::endl;

	errorCode = Neuropixels::waveplayer_arm(bs->slot, true);

	std::cout << "waveplayer_arm error code: " << errorCode << std::endl;

	adc->disableInput(0);

}

void OneBoxDAC::configureDataPlayer(int DACChannel, int portID, int dockID, int channelnr, int sourceType)
{
	Neuropixels::streamsource_t sourcetype = Neuropixels::SourceAP;

	if (sourceType == 1)
		sourceType = Neuropixels::SourceAP;
	else
		sourceType = Neuropixels::SourceLFP;

	errorCode = Neuropixels::DAC_setProbeSniffer(bs->slot, DACChannel, portID, dockID, channelnr, sourcetype);

	std::cout << "DAC_setProbeSniffer error code: " << errorCode << std::endl;

	adc->disableInput(DACChannel);
}

void OneBoxDAC::disableOutput(int chan)
{
	errorCode = Neuropixels::DAC_enableOutput(bs->slot, chan, false);

	adc->enableInput(chan);
}

void OneBoxDAC::enableOutput(int chan)
{
	errorCode = Neuropixels::DAC_enableOutput(bs->slot, chan, true);

	adc->disableInput(chan);
}


void OneBoxDAC::run()
{


}
