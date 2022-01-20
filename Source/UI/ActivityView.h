/*
------------------------------------------------------------------

This file is part of the Open Ephys GUI
Copyright (C) 2019 Allen Institute for Brain Science and Open Ephys

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

#ifndef __ACTIVITYVIEW_H__
#define __ACTIVITYVIEW_H__

#include <VisualizerEditorHeaders.h>

enum ActivityToView {
	APVIEW = 0,
	LFPVIEW = 1
};

/**

Helper class for viewing real-time activity across the probe.

*/


class ActivityView
{
public:
	ActivityView(int numChannels, int updateInterval_)
	{
		for (int i = 0; i < numChannels; i++)
			peakToPeakValues.add(0);

		updateInterval = updateInterval_;

		reset();
	}

	const float* getPeakToPeakValues() {


		if (counter > 100)
		{
			for (int i = 0; i < peakToPeakValues.size(); i++)
			{
				peakToPeakValues.set(i, maxChannelValues[i] - minChannelValues[i]);
			}
		}

		return peakToPeakValues.getRawDataPointer();
	}

	void addSample(float sample, int channel)
	{
		if (channel == 0)
		{
			if (counter == updateInterval)
				reset();

			counter++;
		}

		if (counter % 10 == 0)
		{
			if (sample < minChannelValues[channel])
			{
				minChannelValues.set(channel, sample);
				return;
			}

			if (sample > maxChannelValues[channel])
			{
				maxChannelValues.set(channel, sample);
			}
		}
		
	}

	void reset()
	{

		for (int i = 0; i < peakToPeakValues.size(); i++)
		{
			minChannelValues.set(i, 999999.9f);
			maxChannelValues.set(i, -999999.9f);
		}

		counter = 0;
	}

private:

	Array<float, CriticalSection> minChannelValues;
	Array<float, CriticalSection> maxChannelValues;
	Array<float, CriticalSection> peakToPeakValues;

	int counter;
	int updateInterval;

};

	


#endif  // __ACTIVITYVIEW_H__
