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

#ifndef __GEOMETRY_H_2C4C2D67__
#define __GEOMETRY_H_2C4C2D67__

#include "../NeuropixComponents.h"


class Geometry
{
public:
	Geometry() { }

	/** For probes without emission sites*/
	static bool forPartNumber(String pn,
		Array<ElectrodeMetadata>& em,
		ProbeMetadata& pm);

	/** For probes with emission sites*/
	static bool forPartNumber(String pn,
		Array<ElectrodeMetadata>& em,
		Array<EmissionSiteMetadata>& esm,
		ProbeMetadata& pm);

	/** Non-human primate Phase 1 (Passive) */
	static void NHP1(Array<ElectrodeMetadata>& em,
		ProbeMetadata& pm);

	/** Non-human primate Phase 2 (Active)
	*
		Available in 3 lengths: 10 mm, 25 mm, 45 mm

		Available in 2 site layouts: staggered (true) or straight (false)

		Available in Sapiens version (P/N = NP1016)
	*/
	static void NHP2(int length, bool siteLayout, bool sapiens, Array<ElectrodeMetadata>& em,
		ProbeMetadata& pm);

	/** Neuropixels 1.0
	*/
	static void NP1(Array<ElectrodeMetadata>& em,
		ProbeMetadata& pm);

	/** Neuropixels 2.0
	*
		Available with 1 shank or 4 shanks
	*/
	static void NP2(int shank_count, int adc_bits,
		Array<ElectrodeMetadata>& em,
		ProbeMetadata& pm);

	/** Neuropixels UHD Passive
	*
		Fixed site layout

		Site spacing: 6 um, 4.5 um or 3 um
	*/
	static void UHDPassive(int numColumns, 
		float siteSpacing, 
		Array<ElectrodeMetadata>& em,
		ProbeMetadata& pm);

	/** Neuropixels UHD Active
	*
		Switchable site layout

		Site spacing: 6 um
	*/
	static void UHDActive(Array<ElectrodeMetadata>& em,
		ProbeMetadata& pm);

	/** Neuropixels Opto
	*/
	static void OPTO(Array<ElectrodeMetadata>& em, 
		Array<EmissionSiteMetadata>& esm,
		ProbeMetadata& pm);

	/** Neuropixels QuadBase
	*/
	static void QuadBase(Array<ElectrodeMetadata>& em,
		ProbeMetadata& pm);

};

#endif //__GEOMETRY_H_2C4C2D67__