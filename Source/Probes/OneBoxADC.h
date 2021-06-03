/*
------------------------------------------------------------------

This file is part of the Open Ephys GUI
Copyright (C) 2020 Allen Institute for Brain Science and Open Ephys

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

#ifndef __ONEBOX_ADC_H_2C4C2D67__
#define __ONEBOX_ADC_H_2C4C2D67__

#include "../NeuropixComponents.h"

#include "../API/v3/NeuropixAPI.h"

# define SAMPLECOUNT 64


class OneBoxADC : public DataSource
{
public:
	OneBoxADC(Basestation* bs);

	void getInfo() override;

	bool open() override;
	bool close() override;

	void initialize() override;

	void setAdcInputRange(AdcInputRange range);

	void startAcquisition() override;
	void stopAcquisition() override;

	void run() override; // acquire data

	Neuropixels::NP_ErrorCode errorCode;

	float bitVolts;

	int64 timestamp;

	DataBuffer* sampleBuffer;

};


#endif  // __ONEBOX_ADC_H_2C4C2D67__