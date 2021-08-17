/*
------------------------------------------------------------------

This file is part of the Open Ephys GUI
Copyright (C) 2021 Allen Institute for Brain Science and Open Ephys

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

#ifndef __ONEBOX_H_2C4C2D67__
#define __ONEBOX_H_2C4C2D67__

#include "../API/v3/NeuropixAPI.h"
#include "../NeuropixComponents.h"

# define SAMPLECOUNT 64

class OneBoxADC;
class OneBoxDAC;

class OneBox : public Basestation

{
public:
	OneBox(int ID);
	~OneBox();

	int slot;

	bool open() override;
	void close() override;
	void initialize() override;

	int getProbeCount() override;

	//float getTemperature() override;

	void getInfo() override;

	void setSyncAsInput() override;
	void setSyncAsOutput(int freqIndex) override;

	Array<int> getSyncFrequencies() override;

	void startAcquisition() override;
	void stopAcquisition() override;

	float getFillPercentage() override;

	void updateBsFirmware(File file) override;
	void updateBscFirmware(File file) override;

	void run() override;

	Array<DataSource*> getAdditionalDataSources() override;

	void triggerWaveplayer(bool shouldStart);

	//Neuropixels::bistElectrodeStats stats[960];

	Neuropixels::NP_ErrorCode errorCode;

	static int box_count;

	const int first_available_slot = 16;

	ScopedPointer<OneBoxADC> adcSource;
	ScopedPointer<OneBoxDAC> dacSource;

};


#endif  // __ONEBOX_H_2C4C2D67__