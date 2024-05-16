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

#include "Geometry.h"

bool Geometry::forPartNumber(String PN,
	Array<ElectrodeMetadata>& em,
	ProbeMetadata& pm)
{

	LOGC("Validating part number: ", PN);

	bool found_valid_part_number = true;

	if (PN.equalsIgnoreCase("NP1010")
		|| PN.equalsIgnoreCase("NP1011")
		|| PN.equalsIgnoreCase("NP1012")
		|| PN.equalsIgnoreCase("NP1013"))
		NHP2(10, true, false, em, pm); // staggered layout

	else if (PN.equalsIgnoreCase("NP1015"))
		NHP2(10, false, false, em, pm); // linear layout

	else if (PN.equalsIgnoreCase("NP1016"))
		NHP2(10, false, true, em, pm); // linear layout, Sapiens version

	else if (PN.equalsIgnoreCase("NP1020") || PN.equalsIgnoreCase("NP1021"))
		NHP2(25, true, false, em, pm); // staggered layout

	else if (PN.equalsIgnoreCase("NP1022"))
		NHP2(25, false, false, em, pm); // linear layout

	else if (PN.equalsIgnoreCase("NP1030") || PN.equalsIgnoreCase("NP1031"))
		NHP2(45, true, false, em, pm); // staggered layout

	else if (PN.equalsIgnoreCase("NP1032"))
		NHP2(45, false, false, em, pm); // linear layout

	else if (PN.equalsIgnoreCase("NP1200") || PN.equalsIgnoreCase("NP1210"))
		NHP1(em, pm);

	else if (PN.equalsIgnoreCase("PRB2_1_2_0640_0") 
		|| PN.equalsIgnoreCase("PRB2_1_4_0480_1") 
		|| PN.equalsIgnoreCase("NP2000")
		|| PN.equalsIgnoreCase("NP2003")
		|| PN.equalsIgnoreCase("NP2004"))
		NP2(1, em, pm); // single shank

	else if (PN.equalsIgnoreCase("PRB2_4_2_0640_0") 
		|| PN.equalsIgnoreCase("NP2010")
		|| PN.equalsIgnoreCase("NP2013")
		|| PN.equalsIgnoreCase("NP2014"))
		NP2(4, em, pm); // multi-shank

	else if (PN.equalsIgnoreCase("PRB_1_4_0480_1")
		|| PN.equalsIgnoreCase("PRB_1_4_0480_1_C")
		|| PN.equalsIgnoreCase("PRB_1_2_0480_2"))
		NP1(em, pm);

	else if (PN.equalsIgnoreCase("NP1100"))
		UHDPassive(8, 6, em, pm); // UHD1 - fixed, 8 cols, 6 um spacing

	else if (PN.equalsIgnoreCase("NP1120"))
		UHDPassive(2, 4.5, em, pm); // UHD3, Type 1 - fixed, 2 cols, 4.5 um spacing

	else if (PN.equalsIgnoreCase("NP1121"))
		UHDPassive(1, 3, em, pm); // UHD3, Type 2 - fixed, 1 col, 3.0 um spacing

	else if (PN.equalsIgnoreCase("NP1122"))
		UHDPassive(16, 3, em, pm); // UHD3, Type 3 - fixed, 16 cols, 3.0 um spacing

	else if (PN.equalsIgnoreCase("NP1123"))
		UHDPassive(12, 4.5, em, pm); // UHD3, Type 4 - fixed,  12 cols, 4.5 um spacing

	else if (PN.equalsIgnoreCase("NP1110"))
		UHDActive(em, pm); // UHD2 - switchable, 8 cols, 6 um spacing

	else if (PN.equalsIgnoreCase("NP2020"))
		QuadBase(em, pm);

	else
		found_valid_part_number = false;

	if (!found_valid_part_number)
		CoreServices::sendStatusMessage("Unrecognized part number: " + PN);

	LOGDD("Part #: ", PN, " Valid: ", found_valid_part_number);

	return found_valid_part_number;

}

bool Geometry::forPartNumber(String PN,
	Array<ElectrodeMetadata>& em,
	Array<EmissionSiteMetadata>& esm,
	ProbeMetadata& pm)
{

	bool found_valid_part_number = true;

	if (PN.equalsIgnoreCase("NP1300"))
		OPTO(em, esm, pm);
	else
		found_valid_part_number = false;

	if (!found_valid_part_number)
		CoreServices::sendStatusMessage("Unrecognized part number: " + PN);

	LOGDD("Part #: ", PN, " Valid: ", found_valid_part_number);

	return found_valid_part_number;

}

void Geometry::NP1(Array<ElectrodeMetadata>& electrodeMetadata,
				   ProbeMetadata& probeMetadata)
{

	probeMetadata.type = ProbeType::NP1;
	probeMetadata.name = "Neuropixels 1.0";

	Path path;
	path.startNewSubPath(27, 31);
	path.lineTo(27, 514);
	path.lineTo(27 + 5, 522);
	path.lineTo(27 + 10, 514);
	path.lineTo(27 + 10, 31);
	path.closeSubPath();

	probeMetadata.shank_count = 1;
	probeMetadata.electrodes_per_shank = 960;
	probeMetadata.rows_per_shank = 960 / 2;
	probeMetadata.columns_per_shank = 2;
	probeMetadata.shankOutline = path;
	probeMetadata.num_adcs = 32;

	probeMetadata.availableBanks = 
	{	Bank::A,
		Bank::B,
		Bank::C,
		Bank::OFF //disconnected
	};

	Array<float> xpositions = { 27.0f, 59.0f, 11.0f, 43.0f };

	for (int i = 0; i < probeMetadata.electrodes_per_shank * probeMetadata.shank_count; i++)
	{
		ElectrodeMetadata metadata;

		metadata.shank = 0;
		metadata.shank_local_index = i % probeMetadata.electrodes_per_shank;
		metadata.global_index = i;
		metadata.xpos = xpositions[i % 4];
		metadata.ypos = (i - (i % 2)) * 10.0f;
		metadata.site_width = 12;
		metadata.column_index = i % 2;
		metadata.row_index = i / 2;
		metadata.isSelected = false;
		metadata.colour = Colours::lightgrey;

		if (i < 384)
		{
			metadata.bank = Bank::A;
			metadata.channel = i;
			metadata.status = ElectrodeStatus::CONNECTED;
		}
		else if (i >= 384 && i < 768)
		{
			metadata.bank = Bank::B;
			metadata.channel = i - 384;
			metadata.status = ElectrodeStatus::DISCONNECTED;
		}
		else
		{
			metadata.bank = Bank::C;
			metadata.channel = i - 768;
			metadata.status = ElectrodeStatus::DISCONNECTED;
		}

		if (i == 191 || i == 575 || i == 959)
		{
			metadata.type = ElectrodeType::REFERENCE;
		}
		else {
			metadata.type = ElectrodeType::ELECTRODE;
		}

		electrodeMetadata.add(metadata);
	}
}


void Geometry::NP2(int shank_count,
	Array<ElectrodeMetadata>& electrodeMetadata,
	ProbeMetadata& probeMetadata)
{

	if (shank_count == 1)
	{
		probeMetadata.type = ProbeType::NP2_1;
		probeMetadata.name = "Neuropixels 2.0 - Single Shank";
	}
	else
	{
		probeMetadata.type = ProbeType::NP2_4;
		probeMetadata.name = "Neuropixels 2.0 - Multishank";
	}
		

	Path path;
	path.startNewSubPath(27, 31);
	path.lineTo(27, 514);
	path.lineTo(27 + 5, 522);
	path.lineTo(27 + 10, 514);
	path.lineTo(27 + 10, 31);
	path.closeSubPath();

	probeMetadata.shank_count = shank_count;
	probeMetadata.electrodes_per_shank = 1280;
	probeMetadata.rows_per_shank = 1280 / 2;
	probeMetadata.columns_per_shank = 2;
	probeMetadata.shankOutline = path;
	probeMetadata.num_adcs = 24;

	probeMetadata.availableBanks =
		{   Bank::A,
			Bank::B,
			Bank::C,
			Bank::D,
			Bank::OFF //disconnected
		};

	for (int i = 0; i < probeMetadata.electrodes_per_shank * probeMetadata.shank_count; i++)
	{
		ElectrodeMetadata metadata;

		metadata.global_index = i;

		metadata.shank = i / probeMetadata.electrodes_per_shank;
		metadata.shank_local_index = i % probeMetadata.electrodes_per_shank;

		metadata.xpos = i % 2 * 32.0f + 8.0f; 
		metadata.ypos = (metadata.shank_local_index - (metadata.shank_local_index % 2)) * 7.5f;
		metadata.site_width = 12;

		metadata.column_index = i % 2;
		metadata.row_index = metadata.shank_local_index / 2;

		metadata.isSelected = false;

		if (shank_count == 1)
		{

			if (i < 384)
			{
				metadata.bank = Bank::A;

				int bank_index = metadata.shank_local_index % 384;
				int block = bank_index / 32;
				int row = (bank_index % 32) / 2;

				if (i % 2 == 0) { // left side
					metadata.channel = row * 2 + block * 32;
				}
				else { // right side
					metadata.channel = row * 2 + block * 32 + 1;
				}

				metadata.status = ElectrodeStatus::CONNECTED;
			}
			else if (i >= 384 && i < 768)
			{
				metadata.bank = Bank::B;

				int bank_index = metadata.shank_local_index % 384;
				int block = bank_index / 32;
				int row = (bank_index % 32) / 2;

				if (i % 2 == 0) { // left side
					metadata.channel = ((row * 7) % 16) * 2 + block * 32;
				}
				else {
					metadata.channel = ((row * 7 + 4) % 16) * 2 + block * 32 + 1;
				}

				metadata.status = ElectrodeStatus::DISCONNECTED;
			}
			else if (i >= 768 && i < 1152)
			{
				metadata.bank = Bank::C;

				int bank_index = metadata.shank_local_index % 384;
				int block = bank_index / 32;
				int row = (bank_index % 32) / 2;

				if (i % 2 == 0) { // left side
					metadata.channel = ((row * 5) % 16) * 2 + block * 32;
				}
				else {
					metadata.channel = ((row * 5 + 8) % 16) * 2 + block * 32 + 1;
				}

				metadata.status = ElectrodeStatus::DISCONNECTED;
			}
			else {
				metadata.bank = Bank::D;

				int bank_index = metadata.shank_local_index % 384;
				int block = bank_index / 32;
				int row = (bank_index % 32) / 2;

				if (i % 2 == 0) { // left side
					metadata.channel = ((row * 3) % 16) * 2 + block * 32;
				}
				else {
					metadata.channel = ((row * 3 + 12) % 16) * 2 + block * 32 + 1;
				}

				metadata.status = ElectrodeStatus::DISCONNECTED;
			}

			metadata.type = ElectrodeType::ELECTRODE; // disable internal reference channel

		}
		else if (shank_count == 4)
		{
			

			if (i < 384)
			{
				metadata.status = ElectrodeStatus::CONNECTED;
			}
			else {
				metadata.status = ElectrodeStatus::DISCONNECTED;
			}

			if (metadata.shank_local_index < 384)
				metadata.bank = Bank::A;
			else if (metadata.shank_local_index >= 384 &&
				metadata.shank_local_index < 768)
				metadata.bank = Bank::B;
			else if (metadata.shank_local_index >= 768 &&
				metadata.shank_local_index < 1152)
				metadata.bank = Bank::C;
			else
				metadata.bank = Bank::D;

			int block = metadata.shank_local_index % 384 / 48 + 1;
			int block_index = metadata.shank_local_index % 48;

			if (metadata.shank == 0)
			{
				switch (block)
				{
					case 1:
						metadata.channel = block_index + 48 * 0; // 1-48 (Bank 0-3)
						break;
					case 2:
						metadata.channel = block_index + 48 * 2; // 96-144 (Bank 0-3)
						break;
					case 3:
						metadata.channel = block_index + 48 * 4; // 192-223 (Bank 0-3)
						break;
					case 4:
						metadata.channel = block_index + 48 * 6; // 288-336 (Bank 0-2)
						break;
					case 5:
						metadata.channel = block_index + 48 * 5; // 240-288 (Bank 0-2)
						break;
					case 6:
						metadata.channel = block_index + 48 * 7; // 336-384 (Bank 0-2)
						break;
					case 7:
						metadata.channel = block_index + 48 * 1; // 48-96 (Bank 0-2)
						break;
					case 8:
						metadata.channel = block_index + 48 * 3; // 144-192 (Bank 0-2)
						break;
					default:
						metadata.channel = -1;
				}
			} else if (metadata.shank == 1)
			{
				switch (block)
				{
				case 1:
					metadata.channel = block_index + 48 * 1; // 48-96 (Bank 0-3)
					break;
				case 2:
					metadata.channel = block_index + 48 * 3; // 144-192 (Bank 0-3)
					break;
				case 3:
					metadata.channel = block_index + 48 * 5; // 240-27
					break;
				case 4:
					metadata.channel = block_index + 48 * 7;
					break;
				case 5:
					metadata.channel = block_index + 48 * 4;
					break;
				case 6:
					metadata.channel = block_index + 48 * 6;
					break;
				case 7:
					metadata.channel = block_index + 48 * 0;
					break;
				case 8:
					metadata.channel = block_index + 48 * 2;
					break;
				default:
					metadata.channel = -1;
				}
			} if (metadata.shank == 2)
			{
				switch (block)
				{
				case 1:
					metadata.channel = block_index + 48 * 4;
					break;
				case 2:
					metadata.channel = block_index + 48 * 6;
					break;
				case 3:
					metadata.channel = block_index + 48 * 0;
					break;
				case 4:
					metadata.channel = block_index + 48 * 2;
					break;
				case 5:
					metadata.channel = block_index + 48 * 1;
					break;
				case 6:
					metadata.channel = block_index + 48 * 3;
					break;
				case 7:
					metadata.channel = block_index + 48 * 5;
					break;
				case 8:
					metadata.channel = block_index + 48 * 7;
					break;
				default:
					metadata.channel = -1;
				}
			} if (metadata.shank == 3)
			{
				switch (block)
				{
				case 1:
					metadata.channel = block_index + 48 * 5;
					break;
				case 2:
					metadata.channel = block_index + 48 * 7;
					break;
				case 3:
					metadata.channel = block_index + 48 * 1;
					break;
				case 4:
					metadata.channel = block_index + 48 * 3;
					break;
				case 5:
					metadata.channel = block_index + 48 * 0;
					break;
				case 6:
					metadata.channel = block_index + 48 * 2;
					break;
				case 7:
					metadata.channel = block_index + 48 * 4;
					break;
				case 8:
					metadata.channel = block_index + 48 * 6;
					break;
				default:
					metadata.channel = -1;
				}
			}


			metadata.type = ElectrodeType::ELECTRODE; // disable internal reference
		}

		electrodeMetadata.add(metadata);
	}
}


void Geometry::NHP1(Array<ElectrodeMetadata>& electrodeMetadata,
	ProbeMetadata& probeMetadata)
{

	probeMetadata.type = ProbeType::NHP1;
	probeMetadata.name = "Neuropixels NHP - Passive";

	Path path;
	path.startNewSubPath(27, 31);
	path.lineTo(27, 514);
	path.lineTo(27 + 5, 522);
	path.lineTo(27 + 10, 514);
	path.lineTo(27 + 10, 31);
	path.closeSubPath();

	probeMetadata.shank_count = 1;
	probeMetadata.electrodes_per_shank = 128;
	probeMetadata.rows_per_shank = 128 / 2;
	probeMetadata.columns_per_shank = 2;
	probeMetadata.shankOutline = path;
	probeMetadata.num_adcs = 32;

	probeMetadata.availableBanks =
		{ Bank::A
		};

	Array<int> channel_map = { 6,10,14,18,22,26,30,34,38,42,50,2,60,62,64,
		54,58,103,56,115,107,46,119,111,52,123,4,127,8,12,16,20,
		24,28,32,36,40,44,48,121,105,93,125,101,89,99,97,85,95,109,
		81,87,113,77,83,117,73,91,71,69,79,67,65,75,63,61,47,59,57,
		51,55,53,43,9,49,35,13,45,39,17,41,31,29,37,1,25,33,5,21,84,
		88,92,96,100,104,108,112,116,120,124,3,128,7,80,19,11,82,23,15,
		76,27,70,74,68,66,72,126,78,86,90,94,98,102,106,110,114,118,122 };

	Array<float> xpositions = { 27.0f, 59.0f, 11.0f, 43.0f };

	for (int i = 0; i < probeMetadata.electrodes_per_shank * probeMetadata.shank_count; i++)
	{
		ElectrodeMetadata metadata;

		metadata.global_index = i;

		metadata.shank = 0;
		metadata.shank_local_index = i;

		metadata.xpos = xpositions[i % 4]; 
		metadata.ypos = (metadata.shank_local_index - (metadata.shank_local_index % 2)) * 10.0f;
		metadata.site_width = 12;

		metadata.column_index = i % 2;
		metadata.row_index = metadata.shank_local_index / 2;

		metadata.bank = Bank::A;
		metadata.channel = channel_map[i];
		metadata.status = ElectrodeStatus::CONNECTED;

		metadata.isSelected = false;

		electrodeMetadata.add(metadata);
	}
}



void Geometry::NHP2(int length, 
	bool siteLayout,
	bool sapiensVersion,
	Array<ElectrodeMetadata>& electrodeMetadata,
	ProbeMetadata& probeMetadata)
{

	if (length == 10)
	{
		probeMetadata.type = ProbeType::NHP10;
		if (!sapiensVersion)
			probeMetadata.name = "Neuropixels NHP - Active (10 mm)";
		else
			probeMetadata.name = "Neuropixels 1.0 - Sapiens";
	}
	else if (length == 25)
	{
		probeMetadata.type = ProbeType::NHP25;
		probeMetadata.name = "Neuropixels NHP - Active (25 mm)";
	} else if (length == 45)
	{
		probeMetadata.type = ProbeType::NHP45;
		probeMetadata.name = "Neuropixels NHP - Active (45 mm)";
	}

	Path path;
	path.startNewSubPath(27, 31);
	path.lineTo(27, 514);
	path.lineTo(27 + 5, 522);
	path.lineTo(27 + 10, 514);
	path.lineTo(27 + 10, 31);
	path.closeSubPath();

	if (length == 10)
		probeMetadata.electrodes_per_shank = 960;
	else if (length == 25)
		probeMetadata.electrodes_per_shank = 2496;
	else if (length == 45)
		probeMetadata.electrodes_per_shank = 4416;

	probeMetadata.shank_count = 1;
	probeMetadata.rows_per_shank = probeMetadata.electrodes_per_shank / 2;
	probeMetadata.columns_per_shank = 2;
	probeMetadata.shankOutline = path;
	probeMetadata.num_adcs = 32;

	probeMetadata.availableBanks = { 
		Bank::A,
		Bank::B,
		Bank::C,
		Bank::D,
		Bank::E,
		Bank::F,
		Bank::G,
		Bank::H,
		Bank::I,
		Bank::J,
		Bank::K,
		Bank::L
	};

	Array<float> xpositions;
	
	if (siteLayout) // staggered
		xpositions = { 27.0f, 59.0f, 11.0f, 43.0f };
	else // straight
	{
		if (length == 10)
			xpositions = { 27.0f, 59.0f, 27.0f, 59.0f };
		else
			xpositions = { 11.0f, 114.0f, 11.0f, 114.0f };
	}

	for (int i = 0; i < probeMetadata.electrodes_per_shank; i++)
	{
		ElectrodeMetadata metadata;

		metadata.global_index = i;

		metadata.shank = 0;
		metadata.shank_local_index = i;

		metadata.xpos = xpositions[i % 4]; 
		metadata.ypos = (metadata.shank_local_index - (metadata.shank_local_index % 2)) * 10.0f;
		metadata.site_width = 12;

		metadata.column_index = i % 2;
		metadata.row_index = metadata.shank_local_index / 2;

		int bank_index = i / 384;

		metadata.bank = probeMetadata.availableBanks[bank_index];
		metadata.channel = i % 384;

		metadata.isSelected = false;

		if (i < 384)
			metadata.status = ElectrodeStatus::CONNECTED;
		else
			metadata.status = ElectrodeStatus::DISCONNECTED;

		if (metadata.channel == 191)
		{
			metadata.type = ElectrodeType::REFERENCE;
		}
		else {
			metadata.type = ElectrodeType::ELECTRODE;
		}

		electrodeMetadata.add(metadata);
	}
}


void Geometry::UHDPassive(int numColumns,
	float siteSpacing, Array<ElectrodeMetadata>& electrodeMetadata,
	ProbeMetadata& probeMetadata)
{
	
	probeMetadata.name = "Neuropixels Ultra";

	probeMetadata.type = ProbeType::UHD1;

	if (numColumns == 8 && siteSpacing == 6.0f)
		probeMetadata.name = "Neuropixels Ultra (Phase 1)";
	if (numColumns == 2 && siteSpacing == 4.5f)
		probeMetadata.name = "Neuropixels Ultra (Phase 3, Type 1)";
	if (numColumns == 1 && siteSpacing == 3.0f)
		probeMetadata.name = "Neuropixels Ultra (Phase 3, Type 2)";
	if (numColumns == 16 && siteSpacing == 3.0f)
		probeMetadata.name = "Neuropixels Ultra (Phase 3, Type 3)";
	if (numColumns == 12 && siteSpacing == 4.5f)
		probeMetadata.name = "Neuropixels Ultra (Phase 3, Type 4)";
	
	probeMetadata.switchable = false;

	Path path;
	path.startNewSubPath(27, 31);
	path.lineTo(27, 514);
	path.lineTo(27 + 10, 542);
	path.lineTo(27 + 20, 514);
	path.lineTo(27 + 20, 31);
	path.closeSubPath();

	probeMetadata.shank_count = 1;
	probeMetadata.electrodes_per_shank = 384;
	probeMetadata.rows_per_shank = 384 / numColumns;
	probeMetadata.columns_per_shank = numColumns;
	probeMetadata.shankOutline = path;
	probeMetadata.num_adcs = 32;

	probeMetadata.availableBanks =
		{ Bank::A
		};


	for (int i = 0; i < probeMetadata.electrodes_per_shank; i++)
	{
		ElectrodeMetadata metadata;

		metadata.shank = 0;
		metadata.shank_local_index = i;
		metadata.global_index = i;
		metadata.xpos = i % numColumns * siteSpacing + 2*siteSpacing;
		metadata.ypos = (i - (i % numColumns)) * siteSpacing / numColumns;
		metadata.column_index = i % numColumns;
		metadata.row_index = i / numColumns;
		metadata.site_width = siteSpacing - 1;

		metadata.channel = i;
		metadata.bank = Bank::A;
		metadata.status = ElectrodeStatus::CONNECTED;
		metadata.type = ElectrodeType::ELECTRODE;

		metadata.isSelected = false;

		electrodeMetadata.add(metadata);
	}
}


void Geometry::UHDActive(Array<ElectrodeMetadata>& electrodeMetadata,
	ProbeMetadata& probeMetadata)
{

	probeMetadata.name = "Neuropixels Ultra (Switchable)";
	probeMetadata.type = ProbeType::UHD2;

	probeMetadata.switchable = false;

	Path path;
	path.startNewSubPath(27, 23);
	path.lineTo(27, 514);
	path.lineTo(27 + 10, 542);
	path.lineTo(27 + 20, 514);
	path.lineTo(27 + 20, 23);
	path.closeSubPath();

	int numRows = 768;
	int numColumns = 8;

	probeMetadata.shank_count = 1;
	probeMetadata.electrodes_per_shank = 6144;
	probeMetadata.rows_per_shank = numRows;
	probeMetadata.columns_per_shank = numColumns;
	probeMetadata.shankOutline = path;
	probeMetadata.num_adcs = 32;

	int siteSpacing = 6;

	probeMetadata.availableBanks =
	{ Bank::A
	};

	Array<int> column_order = { 0, 7, 1, 6, 2, 5, 3, 4 };
	Array<int> channel_order = {
			0,2,32,34,63,61,31,29,4,6,36,38,59,57,27,25,8,10,40,42,55,53,23,21,
			12,14,44,46,51,49,19,17,16,18,48,50,47,45,15,13,20,22,52,54,43,41,
			11,9,24,26,56,58,39,37,7,5,28,30,60,62,35,33,3,1,64,66,96,98,127,
			125,95,93,68,70,100,102,123,121,91,89,72,74,104,106,119,117,87,85,
			76,78,108,110,115,113,83,81,80,82,112,114,111,109,79,77,84,86,116,
			118,107,105,75,73,88,90,120,122,103,101,71,69,92,94,124,126,99,97,
			67,65,128,130,160,162,191,189,159,157,132,134,164,166,187,185,155,
			153,136,138,168,170,183,181,151,149,140,142,172,174,179,177,147,145,
			144,146,176,178,175,173,143,141,148,150,180,182,171,169,139,137,152,
			154,184,186,167,165,135,133,156,158,188,190,163,161,131,129,192,194,
			224,226,255,253,223,221,196,198,228,230,251,249,219,217,200,202,232,
			234,247,245,215,213,204,206,236,238,243,241,211,209,208,210,240,242,
			239,237,207,205,212,214,244,246,235,233,203,201,216,218,248,250,231,
			229,199,197,220,222,252,254,227,225,195,193,256,258,288,290,319,317,
			287,285,260,262,292,294,315,313,283,281,264,266,296,298,311,309,279,
			277,268,270,300,302,307,305,275,273,272,274,304,306,303,301,271,269,
			276,278,308,310,299,297,267,265,280,282,312,314,295,293,263,261,284,
			286,316,318,291,289,259,257,320,322,352,354,383,381,351,349,324,326,
			356,358,379,377,347,345,328,330,360,362,375,373,343,341,332,334,364,
			366,371,369,339,337,336,338,368,370,367,365,335,333,340,342,372,374,
			363,361,331,329,344,346,376,378,359,357,327,325,348,350,380,382,355,
			353,323,321,32,34,0,2,31,29,63,61,36,38,4,6,27,25,59,57,40,42,8,10,
			23,21,55,53,44,46,12,14,19,17,51,49,48,50,16,18,15,13,47,45,52,54,20,
			22,11,9,43,41,56,58,24,26,7,5,39,37,60,62,28,30,3,1,35,33,96,98,64,66,
			95,93,127,125,100,102,68,70,91,89,123,121,104,106,72,74,87,85,119,117,
			108,110,76,78,83,81,115,113,112,114,80,82,79,77,111,109,116,118,84,86,
			75,73,107,105,120,122,88,90,71,69,103,101,124,126,92,94,67,65,99,97,160,
			162,128,130,159,157,191,189,164,166,132,134,155,153,187,185,168,170,136,
			138,151,149,183,181,172,174,140,142,147,145,179,177,176,178,144,146,143,
			141,175,173,180,182,148,150,139,137,171,169,184,186,152,154,135,133,167,
			165,188,190,156,158,131,129,163,161,224,226,192,194,223,221,255,253,228,
			230,196,198,219,217,251,249,232,234,200,202,215,213,247,245,236,238,204,
			206,211,209,243,241,240,242,208,210,207,205,239,237,244,246,212,214,203,
			201,235,233,248,250,216,218,199,197,231,229,252,254,220,222,195,193,227,
			225,288,290,256,258,287,285,319,317,292,294,260,262,283,281,315,313,296,
			298,264,266,279,277,311,309,300,302,268,270,275,273,307,305,304,306,272,
			274,271,269,303,301,308,310,276,278,267,265,299,297,312,314,280,282,263,
			261,295,293,316,318,284,286,259,257,291,289,352,354,320,322,351,349,383,
			381,356,358,324,326,347,345,379,377,360,362,328,330,343,341,375,373,364,
			366,332,334,339,337,371,369,368,370,336,338,335,333,367,365,372,374,340,
			342,331,329,363,361,376,378,344,346,327,325,359,357,380,382,348,350,323,
			321,355,353,0,2,32,34,63,61,31,29,4,6,36,38,59,57,27,25,8,10,40,42,55,
			53,23,21,12,14,44,46,51,49,19,17,16,18,48,50,47,45,15,13,20,22,52,54,43,
			41,11,9,24,26,56,58,39,37,7,5,28,30,60,62,35,33,3,1,64,66,96,98,127,125,
			95,93,68,70,100,102,123,121,91,89,72,74,104,106,119,117,87,85,76,78,108,
			110,115,113,83,81,80,82,112,114,111,109,79,77,84,86,116,118,107,105,75,73,
			88,90,120,122,103,101,71,69,92,94,124,126,99,97,67,65,128,130,160,162,191,
			189,159,157,132,134,164,166,187,185,155,153,136,138,168,170,183,181,151,149,
			140,142,172,174,179,177,147,145,144,146,176,178,175,173,143,141,148,150,180,
			182,171,169,139,137,152,154,184,186,167,165,135,133,156,158,188,190,163,161,
			131,129,192,194,224,226,255,253,223,221,196,198,228,230,251,249,219,217,200,
			202,232,234,247,245,215,213,204,206,236,238,243,241,211,209,208,210,240,242,
			239,237,207,205,212,214,244,246,235,233,203,201,216,218,248,250,231,229,199,
			197,220,222,252,254,227,225,195,193,256,258,288,290,319,317,287,285,260,262,
			292,294,315,313,283,281,264,266,296,298,311,309,279,277,268,270,300,302,307,
			305,275,273,272,274,304,306,303,301,271,269,276,278,308,310,299,297,267,265,
			280,282,312,314,295,293,263,261,284,286,316,318,291,289,259,257,320,322,352,
			354,383,381,351,349,324,326,356,358,379,377,347,345,328,330,360,362,375,373,
			343,341,332,334,364,366,371,369,339,337,336,338,368,370,367,365,335,333,340,
			342,372,374,363,361,331,329,344,346,376,378,359,357,327,325,348,350,380,382,
			355,353,323,321,32,34,0,2,31,29,63,61,36,38,4,6,27,25,59,57,40,42,8,10,23,21,
			55,53,44,46,12,14,19,17,51,49,48,50,16,18,15,13,47,45,52,54,20,22,11,9,43,41,
			56,58,24,26,7,5,39,37,60,62,28,30,3,1,35,33,96,98,64,66,95,93,127,125,100,102,
			68,70,91,89,123,121,104,106,72,74,87,85,119,117,108,110,76,78,83,81,115,113,112,
			114,80,82,79,77,111,109,116,118,84,86,75,73,107,105,120,122,88,90,71,69,103,101,
			124,126,92,94,67,65,99,97,160,162,128,130,159,157,191,189,164,166,132,134,155,153,
			187,185,168,170,136,138,151,149,183,181,172,174,140,142,147,145,179,177,176,178,
			144,146,143,141,175,173,180,182,148,150,139,137,171,169,184,186,152,154,135,133,
			167,165,188,190,156,158,131,129,163,161,224,226,192,194,223,221,255,253,228,230,
			196,198,219,217,251,249,232,234,200,202,215,213,247,245,236,238,204,206,211,209,
			243,241,240,242,208,210,207,205,239,237,244,246,212,214,203,201,235,233,248,250,
			216,218,199,197,231,229,252,254,220,222,195,193,227,225,288,290,256,258,287,285,
			319,317,292,294,260,262,283,281,315,313,296,298,264,266,279,277,311,309,300,302,
			268,270,275,273,307,305,304,306,272,274,271,269,303,301,308,310,276,278,267,265,
			299,297,312,314,280,282,263,261,295,293,316,318,284,286,259,257,291,289,352,354,
			320,322,351,349,383,381,356,358,324,326,347,345,379,377,360,362,328,330,343,341,
			375,373,364,366,332,334,339,337,371,369,368,370,336,338,335,333,367,365,372,374,
			340,342,331,329,363,361,376,378,344,346,327,325,359,357,380,382,348,350,323,321,
			355,353,2,0,34,32,61,63,29,31,6,4,38,36,57,59,25,27,10,8,42,40,53,55,21,23,14,12,
			46,44,49,51,17,19,18,16,50,48,45,47,13,15,22,20,54,52,41,43,9,11,26,24,58,56,37,39,
			5,7,30,28,62,60,33,35,1,3,66,64,98,96,125,127,93,95,70,68,102,100,121,123,89,91,74,
			72,106,104,117,119,85,87,78,76,110,108,113,115,81,83,82,80,114,112,109,111,77,79,86,
			84,118,116,105,107,73,75,90,88,122,120,101,103,69,71,94,92,126,124,97,99,65,67,130,
			128,162,160,189,191,157,159,134,132,166,164,185,187,153,155,138,136,170,168,181,183,
			149,151,142,140,174,172,177,179,145,147,146,144,178,176,173,175,141,143,150,148,182,
			180,169,171,137,139,154,152,186,184,165,167,133,135,158,156,190,188,161,163,129,131,
			194,192,226,224,253,255,221,223,198,196,230,228,249,251,217,219,202,200,234,232,245,
			247,213,215,206,204,238,236,241,243,209,211,210,208,242,240,237,239,205,207,214,212,
			246,244,233,235,201,203,218,216,250,248,229,231,197,199,222,220,254,252,225,227,193,
			195,258,256,290,288,317,319,285,287,262,260,294,292,313,315,281,283,266,264,298,296,
			309,311,277,279,270,268,302,300,305,307,273,275,274,272,306,304,301,303,269,271,278,
			276,310,308,297,299,265,267,282,280,314,312,293,295,261,263,286,284,318,316,289,291,
			257,259,322,320,354,352,381,383,349,351,326,324,358,356,377,379,345,347,330,328,362,360,
			373,375,341,343,334,332,366,364,369,371,337,339,338,336,370,368,365,367,333,335,342,340,
			374,372,361,363,329,331,346,344,378,376,357,359,325,327,350,348,382,380,353,355,321,323,
			34,32,2,0,29,31,61,63,38,36,6,4,25,27,57,59,42,40,10,8,21,23,53,55,46,44,14,12,17,19,49,
			51,50,48,18,16,13,15,45,47,54,52,22,20,9,11,41,43,58,56,26,24,5,7,37,39,62,60,30,28,1,3,
			33,35,98,96,66,64,93,95,125,127,102,100,70,68,89,91,121,123,106,104,74,72,85,87,117,119,
			110,108,78,76,81,83,113,115,114,112,82,80,77,79,109,111,118,116,86,84,73,75,105,107,122,
			120,90,88,69,71,101,103,126,124,94,92,65,67,97,99,162,160,130,128,157,159,189,191,166,164,
			134,132,153,155,185,187,170,168,138,136,149,151,181,183,174,172,142,140,145,147,177,179,178,
			176,146,144,141,143,173,175,182,180,150,148,137,139,169,171,186,184,154,152,133,135,165,167,
			190,188,158,156,129,131,161,163,226,224,194,192,221,223,253,255,230,228,198,196,217,219,249,251,234,232,202,200,213,215,245,247,238,236,206,204,209,211,241,243,242,240,210,208,205,207,237,239,246,244,214,212,201,203,233,235,250,248,218,216,197,199,229,231,254,252,222,220,193,195,225,227,290,288,258,256,285,287,317,319,294,292,262,260,281,283,313,315,298,296,266,264,277,279,309,311,302,300,270,268,273,275,305,307,306,304,274,272,269,271,301,303,310,308,278,276,265,267,297,299,314,312,282,280,261,263,293,295,318,316,286,284,257,259,289,291,354,352,322,320,349,351,381,383,358,356,326,324,345,347,377,379,362,360,330,328,341,343,373,375,366,364,334,332,337,339,369,371,370,368,338,336,333,335,365,367,374,372,342,340,329,331,361,363,378,376,346,344,325,327,357,359,382,380,350,348,321,323,353,355,2,0,34,32,61,63,29,31,6,4,38,36,57,59,25,27,10,8,42,40,53,55,21,23,14,12,46,44,49,51,17,19,18,16,50,48,45,47,13,15,22,20,54,52,41,43,9,11,26,24,58,56,37,39,5,7,30,28,62,60,33,35,1,3,66,64,98,96,125,127,93,95,70,68,102,100,121,123,89,91,74,72,106,104,117,119,85,87,78,76,110,108,113,115,81,83,82,80,114,112,109,111,77,79,86,84,118,116,105,107,73,75,90,88,122,120,101,103,69,71,94,92,126,124,97,99,65,67,130,128,162,160,189,191,157,159,134,132,166,164,185,187,153,155,138,136,170,168,181,183,149,151,142,140,174,172,177,179,145,147,146,144,178,176,173,175,141,143,150,148,182,180,169,171,137,139,154,152,186,184,165,167,133,135,158,156,190,188,161,163,129,131,194,192,226,224,253,255,221,223,198,196,230,228,249,251,217,219,202,200,234,232,245,247,213,215,206,204,238,236,241,243,209,211,210,208,242,240,237,239,205,207,214,212,246,244,233,235,201,203,218,216,250,248,229,231,197,199,222,220,254,252,225,227,193,195,258,256,290,288,317,319,285,287,262,260,294,292,313,315,281,283,266,264,298,296,309,311,277,279,270,268,302,300,305,307,273,275,274,272,306,304,301,303,269,271,278,276,310,308,297,299,265,267,282,280,314,312,293,295,261,263,286,284,318,316,289,291,257,259,322,320,354,352,381,383,349,351,326,324,358,356,377,379,345,347,330,328,362,360,373,375,341,343,334,332,366,364,369,371,337,339,338,336,370,368,365,367,333,335,342,340,374,372,361,363,329,331,346,344,378,376,357,359,325,327,350,348,382,380,353,355,321,323,34,32,2,0,29,31,61,63,38,36,6,4,25,27,57,59,42,40,10,8,21,23,53,55,46,44,14,12,17,19,49,51,50,48,18,16,13,15,45,47,54,52,22,20,9,11,41,43,58,56,26,24,5,7,37,39,62,60,30,28,1,3,33,35,98,96,66,64,93,95,125,127,102,100,70,68,89,91,121,123,106,104,74,72,85,87,117,119,110,108,78,76,81,83,113,115,114,112,82,80,77,79,109,111,118,116,86,84,73,75,105,107,122,120,90,88,69,71,101,103,126,124,94,92,65,67,97,99,162,160,130,128,157,159,189,191,166,164,134,132,153,155,185,187,170,168,138,136,149,151,181,183,174,172,142,140,145,147,177,179,178,176,146,144,141,143,173,175,182,180,150,148,137,139,169,171,186,184,154,152,133,135,165,167,190,188,158,156,129,131,161,163,226,224,194,192,221,223,253,255,230,228,198,196,217,219,249,251,234,232,202,200,213,215,245,247,238,236,206,204,209,211,241,243,242,240,210,208,205,207,237,239,246,244,214,212,201,203,233,235,250,248,218,216,197,199,229,231,254,252,222,220,193,195,225,227,290,288,258,256,285,287,317,319,294,292,262,260,281,283,313,315,298,296,266,264,277,279,309,311,302,300,270,268,273,275,305,307,306,304,274,272,269,271,301,303,310,308,278,276,265,267,297,299,314,312,282,280,261,263,293,295,318,316,286,284,257,259,289,291,354,352,322,320,349,351,381,383,358,356,326,324,345,347,377,379,362,360,330,328,341,343,373,375,366,364,334,332,337,339,369,371,370,368,338,336,333,335,365,367,374,372,342,340,329,331,361,363,378,376,346,344,325,327,357,359,382,380,350,348,321,323,353,355,0,2,32,34,63,61,31,29,4,6,36,38,59,57,27,25,8,10,40,42,55,53,23,21,12,14,44,46,51,49,19,17,16,18,48,50,47,45,15,13,20,22,52,54,43,41,11,9,24,26,56,58,39,37,7,5,28,30,60,62,35,33,3,1,64,66,96,98,127,125,95,93,68,70,100,102,123,121,91,89,72,74,104,106,119,117,87,85,76,78,108,110,115,113,83,81,80,82,112,114,111,109,79,77,84,86,116,118,107,105,75,73,88,90,120,122,103,101,71,69,92,94,124,126,99,97,67,65,128,130,160,162,191,189,159,157,132,134,164,166,187,185,155,153,136,138,168,170,183,181,151,149,140,142,172,174,179,177,147,145,144,146,176,178,175,173,143,141,148,150,180,182,171,169,139,137,152,154,184,186,167,165,135,133,156,158,188,190,163,161,131,129,192,194,224,226,255,253,223,221,196,198,228,230,251,249,219,217,200,202,232,234,247,245,215,213,204,206,236,238,243,241,211,209,208,210,240,242,239,237,207,205,212,214,244,246,235,233,203,201,216,218,248,250,231,229,199,197,220,222,252,254,227,225,195,193,256,258,288,290,319,317,287,285,260,262,292,294,315,313,283,281,264,266,296,298,311,309,279,277,268,270,300,302,307,305,275,273,272,274,304,306,303,301,271,269,276,278,308,310,299,297,267,265,280,282,312,314,295,293,263,261,284,286,316,318,291,289,259,257,320,322,352,354,383,381,351,349,324,326,356,358,379,377,347,345,328,330,360,362,375,373,343,341,332,334,364,366,371,369,339,337,336,338,368,370,367,365,335,333,340,342,372,374,363,361,331,329,344,346,376,378,359,357,327,325,348,350,380,382,355,353,323,321,32,34,0,2,31,29,63,61,36,38,4,6,27,25,59,57,40,42,8,10,23,21,55,53,44,46,12,14,19,17,51,49,48,50,16,18,15,13,47,45,52,54,20,22,11,9,43,41,56,58,24,26,7,5,39,37,60,62,28,30,3,1,35,33,96,98,64,66,95,93,127,125,100,102,68,70,91,89,123,121,104,106,72,74,87,85,119,117,108,110,76,78,83,81,115,113,112,114,80,82,79,77,111,109,116,118,84,86,75,73,107,105,120,122,88,90,71,69,103,101,124,126,92,94,67,65,99,97,160,162,128,130,159,157,191,189,164,166,132,134,155,153,187,185,168,170,136,138,151,149,183,181,172,174,140,142,147,145,179,177,176,178,144,146,143,141,175,173,180,182,148,150,139,137,171,169,184,186,152,154,135,133,167,165,188,190,156,158,131,129,163,161,224,226,192,194,223,221,255,253,228,230,196,198,219,217,251,249,232,234,200,202,215,213,247,245,236,238,204,206,211,209,243,241,240,242,208,210,207,205,239,237,244,246,212,214,203,201,235,233,248,250,216,218,199,197,231,229,252,254,220,222,195,193,227,225,288,290,256,258,287,285,319,317,292,294,260,262,283,281,315,313,296,298,264,266,279,277,311,309,300,302,268,270,275,273,307,305,304,306,272,274,271,269,303,301,308,310,276,278,267,265,299,297,312,314,280,282,263,261,295,293,316,318,284,286,259,257,291,289,352,354,320,322,351,349,383,381,356,358,324,326,347,345,379,377,360,362,328,330,343,341,375,373,364,366,332,334,339,337,371,369,368,370,336,338,335,333,367,365,372,374,340,342,331,329,363,361,376,378,344,346,327,325,359,357,380,382,348,350,323,321,355,353,0,2,32,34,63,61,31,29,4,6,36,38,59,57,27,25,8,10,40,42,55,53,23,21,12,14,44,46,51,49,19,17,16,18,48,50,47,45,15,13,20,22,52,54,43,41,11,9,24,26,56,58,39,37,7,5,28,30,60,62,35,33,3,1,64,66,96,98,127,125,95,93,68,70,100,102,123,121,91,89,72,74,104,106,119,117,87,85,76,78,108,110,115,113,83,81,80,82,112,114,111,109,79,77,84,86,116,118,107,105,75,73,88,90,120,122,103,101,71,69,92,94,124,126,99,97,67,65,128,130,160,162,191,189,159,157,132,134,164,166,187,185,155,153,136,138,168,170,183,181,151,149,140,142,172,174,179,177,147,145,144,146,176,178,175,173,143,141,148,150,180,182,171,169,139,137,152,154,184,186,167,165,135,133,156,158,188,190,163,161,131,129,192,194,224,226,255,253,223,221,196,198,228,230,251,249,219,217,200,202,232,234,247,245,215,213,204,206,236,238,243,241,211,209,208,210,240,242,239,237,207,205,212,214,244,246,235,233,203,201,216,218,248,250,231,229,199,197,220,222,252,254,227,225,195,193,256,258,288,290,319,317,287,285,260,262,292,294,315,313,283,281,264,266,296,298,311,309,279,277,268,270,300,302,307,305,275,273,272,274,304,306,303,301,271,269,276,278,308,310,299,297,267,265,280,282,312,314,295,293,263,261,284,286,316,318,291,289,259,257,320,322,352,354,383,381,351,349,324,326,356,358,379,377,347,345,328,330,360,362,375,373,343,341,332,334,364,366,371,369,339,337,336,338,368,370,367,365,335,333,340,342,372,374,363,361,331,329,344,346,376,378,359,357,327,325,348,350,380,382,355,353,323,321,32,34,0,2,31,29,63,61,36,38,4,6,27,25,59,57,40,42,8,10,23,21,55,53,44,46,12,14,19,17,51,49,48,50,16,18,15,13,47,45,52,54,20,22,11,9,43,41,56,58,24,26,7,5,39,37,60,62,28,30,3,1,35,33,96,98,64,66,95,93,127,125,100,102,68,70,91,89,123,121,104,106,72,74,87,85,119,117,108,110,76,78,83,81,115,113,112,114,80,82,79,77,111,109,116,118,84,86,75,73,107,105,120,122,88,90,71,69,103,101,124,126,92,94,67,65,99,97,160,162,128,130,159,157,191,189,164,166,132,134,155,153,187,185,168,170,136,138,151,149,183,181,172,174,140,142,147,145,179,177,176,178,144,146,143,141,175,173,180,182,148,150,139,137,171,169,184,186,152,154,135,133,167,165,188,190,156,158,131,129,163,161,224,226,192,194,223,221,255,253,228,230,196,198,219,217,251,249,232,234,200,202,215,213,247,245,236,238,204,206,211,209,243,241,240,242,208,210,207,205,239,237,244,246,212,214,203,201,235,233,248,250,216,218,199,197,231,229,252,254,220,222,195,193,227,225,288,290,256,258,287,285,319,317,292,294,260,262,283,281,315,313,296,298,264,266,279,277,311,309,300,302,268,270,275,273,307,305,304,306,272,274,271,269,303,301,308,310,276,278,267,265,299,297,312,314,280,282,263,261,295,293,316,318,284,286,259,257,291,289,352,354,320,322,351,349,383,381,356,358,324,326,347,345,379,377,360,362,328,330,343,341,375,373,364,366,332,334,339,337,371,369,368,370,336,338,335,333,367,365,372,374,340,342,331,329,363,361,376,378,344,346,327,325,359,357,380,382,348,350,323,321,355,353,2,0,34,32,61,63,29,31,6,4,38,36,57,59,25,27,10,8,42,40,53,55,21,23,14,12,46,44,49,51,17,19,18,16,50,48,45,47,13,15,22,20,54,52,41,43,9,11,26,24,58,56,37,39,5,7,30,28,62,60,33,35,1,3,66,64,98,96,125,127,93,95,70,68,102,100,121,123,89,91,74,72,106,104,117,119,85,87,78,76,110,108,113,115,81,83,82,80,114,112,109,111,77,79,86,84,118,116,105,107,73,75,90,88,122,120,101,103,69,71,94,92,126,124,97,99,65,67,130,128,162,160,189,191,157,159,134,132,166,164,185,187,153,155,138,136,170,168,181,183,149,151,142,140,174,172,177,179,145,147,146,144,178,176,173,175,141,143,150,148,182,180,169,171,137,139,154,152,186,184,165,167,133,135,158,156,190,188,161,163,129,131,194,192,226,224,253,255,221,223,198,196,230,228,249,251,217,219,202,200,234,232,245,247,213,215,206,204,238,236,241,243,209,211,210,208,242,240,237,239,205,207,214,212,246,244,233,235,201,203,218,216,250,248,229,231,197,199,222,220,254,252,225,227,193,195,258,256,290,288,317,319,285,287,262,260,294,292,313,315,281,283,266,264,298,296,309,311,277,279,270,268,302,300,305,307,273,275,274,272,306,304,301,303,269,271,278,276,310,308,297,299,265,267,282,280,314,312,293,295,261,263,286,284,318,316,289,291,257,259,322,320,354,352,381,383,349,351,326,324,358,356,377,379,345,347,330,328,362,360,373,375,341,343,334,332,366,364,369,371,337,339,338,336,370,368,365,367,333,335,342,340,374,372,361,363,329,331,346,344,378,376,357,359,325,327,350,348,382,380,353,355,321,323,34,32,2,0,29,31,61,63,38,36,6,4,25,27,57,59,42,40,10,8,21,23,53,55,46,44,14,12,17,19,49,51,50,48,18,16,13,15,45,47,54,52,22,20,9,11,41,43,58,56,26,24,5,7,37,39,62,60,30,28,1,3,33,35,98,96,66,64,93,95,125,127,102,100,70,68,89,91,121,123,106,104,74,72,85,87,117,119,110,108,78,76,81,83,113,115,114,112,82,80,77,79,109,111,118,116,86,84,73,75,105,107,122,120,90,88,69,71,101,103,126,124,94,92,65,67,97,99,162,160,130,128,157,159,189,191,166,164,134,132,153,155,185,187,170,168,138,136,149,151,181,183,174,172,142,140,145,147,177,179,178,176,146,144,141,143,173,175,182,180,150,148,137,139,169,171,186,184,154,152,133,135,165,167,190,188,158,156,129,131,161,163,226,224,194,192,221,223,253,255,230,228,198,196,217,219,249,251,234,232,202,200,213,215,245,247,238,236,206,204,209,211,241,243,242,240,210,208,205,207,237,239,246,244,214,212,201,203,233,235,250,248,218,216,197,199,229,231,254,252,222,220,193,195,225,227,290,288,258,256,285,287,317,319,294,292,262,260,281,283,313,315,298,296,266,264,277,279,309,311,302,300,270,268,273,275,305,307,306,304,274,272,269,271,301,303,310,308,278,276,265,267,297,299,314,312,282,280,261,263,293,295,318,316,286,284,257,259,289,291,354,352,322,320,349,351,381,383,358,356,326,324,345,347,377,379,362,360,330,328,341,343,373,375,366,364,334,332,337,339,369,371,370,368,338,336,333,335,365,367,374,372,342,340,329,331,361,363,378,376,346,344,325,327,357,359,382,380,350,348,321,323,353,355,2,0,34,32,61,63,29,31,6,4,38,36,57,59,25,27,10,8,42,40,53,55,21,23,14,12,46,44,49,51,17,19,18,16,50,48,45,47,13,15,22,20,54,52,41,43,9,11,26,24,58,56,37,39,5,7,30,28,62,60,33,35,1,3,66,64,98,96,125,127,93,95,70,68,102,100,121,123,89,91,74,72,106,104,117,119,85,87,78,76,110,108,113,115,81,83,82,80,114,112,109,111,77,79,86,84,118,116,105,107,73,75,90,88,122,120,101,103,69,71,94,92,126,124,97,99,65,67,130,128,162,160,189,191,157,159,134,132,166,164,185,187,153,155,138,136,170,168,181,183,149,151,142,140,174,172,177,179,145,147,146,144,178,176,173,175,141,143,150,148,182,180,169,171,137,139,154,152,186,184,165,167,133,135,158,156,190,188,161,163,129,131,194,192,226,224,253,255,221,223,198,196,230,228,249,251,217,219,202,200,234,232,245,247,213,215,206,204,238,236,241,243,209,211,210,208,242,240,237,239,205,207,214,212,246,244,233,235,201,203,218,216,250,248,229,231,197,199,222,220,254,252,225,227,193,195,258,256,290,288,317,319,285,287,262,260,294,292,313,315,281,283,266,264,298,296,309,311,277,279,270,268,302,300,305,307,273,275,274,272,306,304,301,303,269,271,278,276,310,308,297,299,265,267,282,280,314,312,293,295,261,263,286,284,318,316,289,291,257,259,322,320,354,352,381,383,349,351,326,324,358,356,377,379,345,347,330,328,362,360,373,375,341,343,334,332,366,364,369,371,337,339,338,336,370,368,365,367,333,335,342,340,374,372,361,363,329,331,346,344,378,376,357,359,325,327,350,348,382,380,353,355,321,323,34,32,2,0,29,31,61,63,38,36,6,4,25,27,57,59,42,40,10,8,21,23,53,55,46,44,14,12,17,19,49,51,50,48,18,16,13,15,45,47,54,52,22,20,9,11,41,43,58,56,26,24,5,7,37,39,62,60,30,28,1,3,33,35,98,96,66,64,93,95,125,127,102,100,70,68,89,91,121,123,106,104,74,72,85,87,117,119,110,108,78,76,81,83,113,115,114,112,82,80,77,79,109,111,118,116,86,84,73,75,105,107,122,120,90,88,69,71,101,103,126,124,94,92,65,67,97,99,162,160,130,128,157,159,189,191,166,164,134,132,153,155,185,187,170,168,138,136,149,151,181,183,174,172,142,140,145,147,177,179,178,176,146,144,141,143,173,175,182,180,150,148,137,139,169,171,186,184,154,152,133,135,165,167,190,188,158,156,129,131,161,163,226,224,194,192,221,223,253,255,230,228,198,196,217,219,249,251,234,232,202,200,213,215,245,247,238,236,206,204,209,211,241,243,242,240,210,208,205,207,237,239,246,244,214,212,201,203,233,235,250,248,218,216,197,199,229,231,254,252,222,220,193,195,225,227,290,288,258,256,285,287,317,319,294,292,262,260,281,283,313,315,298,296,266,264,277,279,309,311,302,300,270,268,273,275,305,307,306,304,274,272,269,271,301,303,310,308,278,276,265,267,297,299,314,312,282,280,261,263,293,295,318,316,286,284,257,259,289,291,354,352,322,320,349,351,381,383,358,356,326,324,345,347,377,379,362,360,330,328,341,343,373,375,366,364,334,332,337,339,369,371,370,368,338,336,333,335,365,367,374,372,342,340,329,331,361,363,378,376,346,344,325,327,357,359,382,380,350,348,321,323,353,355,
	};


	for (int i = 0; i < probeMetadata.electrodes_per_shank; i++)
	{
		ElectrodeMetadata metadata;

		metadata.shank = 0;
		metadata.shank_local_index = i;
		metadata.global_index = i;
		metadata.column_index = column_order[i % numColumns];
		metadata.row_index = i / numColumns;
		metadata.xpos = metadata.column_index * siteSpacing;
		metadata.ypos = (i - (i % numColumns)) * siteSpacing / numColumns;
		metadata.site_width = siteSpacing - 1;

		metadata.channel = channel_order[i];
		metadata.bank = Bank::A;
		if (i < 384)
			metadata.status = ElectrodeStatus::CONNECTED;
		else
			metadata.status = ElectrodeStatus::DISCONNECTED;

		metadata.type = ElectrodeType::ELECTRODE;

		metadata.isSelected = false;

		electrodeMetadata.add(metadata);
	}

}



void Geometry::OPTO(Array<ElectrodeMetadata>& electrodeMetadata,
	Array<EmissionSiteMetadata>& emissionSiteMetadata,
	ProbeMetadata& probeMetadata)
{

	probeMetadata.type = ProbeType::OPTO;
	probeMetadata.name = "Neuropixels Opto";

	Path path;
	path.startNewSubPath(27, 31);
	path.lineTo(27, 514);
	path.lineTo(27 + 5, 522);
	path.lineTo(27 + 10, 514);
	path.lineTo(27 + 10, 31);
	path.closeSubPath();

	probeMetadata.shank_count = 1;
	probeMetadata.electrodes_per_shank = 960;
	probeMetadata.rows_per_shank = 960 / 2;
	probeMetadata.columns_per_shank = 2;
	probeMetadata.shankOutline = path;
	probeMetadata.num_adcs = 32;

	probeMetadata.availableBanks =
	{ Bank::A,
		Bank::B,
		Bank::C,
		Bank::OFF //disconnected
	};

	Array<float> xpositions = { 11.0f, 59.0f, 11.0f, 59.0f };

	for (int i = 0; i < probeMetadata.electrodes_per_shank * probeMetadata.shank_count; i++)
	{
		ElectrodeMetadata metadata;

		metadata.shank = 0;
		metadata.shank_local_index = i % probeMetadata.electrodes_per_shank;
		metadata.global_index = i;
		metadata.xpos = xpositions[i % 4];
		metadata.ypos = (i - (i % 2)) * 10.0f;
		metadata.site_width = 12;
		metadata.column_index = i % 2;
		metadata.row_index = i / 2;
		metadata.isSelected = false;
		metadata.colour = Colours::lightgrey;

		if (i < 384)
		{
			metadata.bank = Bank::A;
			metadata.channel = i;
			metadata.status = ElectrodeStatus::CONNECTED;
		}
		else if (i >= 384 && i < 768)
		{
			metadata.bank = Bank::B;
			metadata.channel = i - 384;
			metadata.status = ElectrodeStatus::DISCONNECTED;
		}
		else
		{
			metadata.bank = Bank::C;
			metadata.channel = i - 768;
			metadata.status = ElectrodeStatus::DISCONNECTED;
		}

		if (i == 191 || i == 575 || i == 959)
		{
			metadata.type = ElectrodeType::REFERENCE;
		}
		else {
			metadata.type = ElectrodeType::ELECTRODE;
		}

		electrodeMetadata.add(metadata);
	}

	for (int i = 0; i < 14; i++)
	{

		Array<float> wavelengths = { 450.0f, 638.0f };

		for (auto wv : wavelengths)
		{
			EmissionSiteMetadata metadata;

			metadata.global_index = i;
			metadata.shank_index = 0;
			metadata.xpos = 35.0f;
			metadata.ypos = 60.0f + i * 100.0f;
			metadata.isSelected = false;
			metadata.wavelength_nm = wv;

			emissionSiteMetadata.add(metadata);
		}

	}
}

void Geometry::QuadBase(Array<ElectrodeMetadata>& electrodeMetadata,
	ProbeMetadata& probeMetadata)
{

	int shank_count = 4;

	probeMetadata.type = ProbeType::QUAD_BASE;
	probeMetadata.name = " Neuropixels 2.0 QuadBase";


	Path path;
	path.startNewSubPath(27, 31);
	path.lineTo(27, 514);
	path.lineTo(27 + 5, 522);
	path.lineTo(27 + 10, 514);
	path.lineTo(27 + 10, 31);
	path.closeSubPath();

	probeMetadata.shank_count = shank_count;
	probeMetadata.electrodes_per_shank = 1280;
	probeMetadata.rows_per_shank = 1280 / 2;
	probeMetadata.columns_per_shank = 2;
	probeMetadata.shankOutline = path;
	probeMetadata.num_adcs = 96;

	probeMetadata.availableBanks =
		{   Bank::A,
			Bank::B,
			Bank::C,
			Bank::D,
			Bank::A2,
			Bank::B2,
			Bank::C2,
			Bank::D2,
			Bank::A3,
			Bank::B3,
			Bank::C3,
			Bank::D3,
			Bank::A4,
			Bank::B4,
			Bank::C4,
			Bank::D4,
			Bank::OFF //disconnected
		};

	for (int i = 0; i < probeMetadata.electrodes_per_shank * probeMetadata.shank_count; i++)
	{
		ElectrodeMetadata metadata;

		metadata.global_index = i;

		metadata.shank = i / probeMetadata.electrodes_per_shank;
		metadata.shank_local_index = i % probeMetadata.electrodes_per_shank;

		metadata.xpos = i % 2 * 32.0f + 8.0f;
		metadata.ypos = (metadata.shank_local_index - (metadata.shank_local_index % 2)) * 7.5f;
		metadata.site_width = 12;

		metadata.column_index = i % 2;
		metadata.row_index = metadata.shank_local_index / 2;

		metadata.isSelected = false;

		if (metadata.shank_local_index < 384)
		{
			metadata.status = ElectrodeStatus::CONNECTED;
		}
		else {
			metadata.status = ElectrodeStatus::DISCONNECTED;
		}

		if (metadata.shank_local_index < 384)
		{
			if (metadata.shank == 0)
			{
				metadata.bank = Bank::A;
			}
			else if (metadata.shank == 1)
			{
				metadata.bank = Bank::A;
			}
			else if (metadata.shank == 2)
			{
				metadata.bank = Bank::A;
			}
			else if (metadata.shank == 3)
			{
				metadata.bank = Bank::A;
			}
		}
		else if (metadata.shank_local_index >= 384 &&
			metadata.shank_local_index < 768)
		{
			if (metadata.shank == 0)
			{
				metadata.bank = Bank::B;
			}
			else if (metadata.shank == 1)
			{
				metadata.bank = Bank::B;
			}
			else if (metadata.shank == 2)
			{
				metadata.bank = Bank::B;
			}
			else if (metadata.shank == 3)
			{
				metadata.bank = Bank::B;
			}
		}
		else if (metadata.shank_local_index >= 768 &&
			metadata.shank_local_index < 1152)
		{
			if (metadata.shank == 0)
			{
				metadata.bank = Bank::C;
			}
			else if (metadata.shank == 1)
			{
				metadata.bank = Bank::C;
			}
			else if (metadata.shank == 2)
			{
				metadata.bank = Bank::C;
			}
			else if (metadata.shank == 3)
			{
				metadata.bank = Bank::C;
			}
		}
		else {
			if (metadata.shank == 0)
			{
				metadata.bank = Bank::D;
			}
			else if (metadata.shank == 1)
			{
				metadata.bank = Bank::D;
			}
			else if (metadata.shank == 2)
			{
				metadata.bank = Bank::D;
			}
			else if (metadata.shank == 3)
			{
				metadata.bank = Bank::D;
			}
		}

		metadata.channel = metadata.shank_local_index % 384;

		metadata.type = ElectrodeType::ELECTRODE; // disable internal reference

		electrodeMetadata.add(metadata);
	}
}