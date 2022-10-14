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

	bool found_valid_part_number = true;

	if (PN.equalsIgnoreCase("NP1010"))
		NHP2(10, em, pm);

	else if (PN.equalsIgnoreCase("NP1020") || PN.equalsIgnoreCase("NP1021"))
		NHP2(25, em, pm);

	else if (PN.equalsIgnoreCase("NP1030") || PN.equalsIgnoreCase("NP1031"))
		NHP2(45, em, pm);

	else if (PN.equalsIgnoreCase("NP1200") || PN.equalsIgnoreCase("NP1210"))
		NHP1(em, pm);

	else if (PN.equalsIgnoreCase("PRB2_1_2_0640_0") || PN.equalsIgnoreCase("PRB2_1_4_0480_1") || PN.equalsIgnoreCase("NP2000"))
		NP2(1, em, pm);

	else if (PN.equalsIgnoreCase("PRB2_4_2_0640_0") || PN.equalsIgnoreCase("NP2010"))
		NP2(4, em, pm);

	else if (PN.equalsIgnoreCase("PRB_1_4_0480_1")
		|| PN.equalsIgnoreCase("PRB_1_4_0480_1_C")
		|| PN.equalsIgnoreCase("PRB_1_2_0480_2"))
		NP1(em, pm);

	else if (PN.equalsIgnoreCase("NP1100"))
		UHD(false, em, pm);

	else if (PN.equalsIgnoreCase("NP1110"))
		UHD(true, em, pm);

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
	Array<ElectrodeMetadata>& electrodeMetadata,
	ProbeMetadata& probeMetadata)
{

	if (length == 10)
	{
		probeMetadata.type = ProbeType::NHP10;
		probeMetadata.name = "Neuropixels NHP - Active (10 mm)";
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

	Array<float> xpositions = { 27.0f, 59.0f, 11.0f, 43.0f };

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


void Geometry::UHD(bool switchable, Array<ElectrodeMetadata>& electrodeMetadata,
	ProbeMetadata& probeMetadata)
{
	// need to implement switchable case
	probeMetadata.type = ProbeType::UHD1;

	if (switchable)
	{
		probeMetadata.name = "Neuropixels Ultra (Switchable)";
	}
	else {
		probeMetadata.name = "Neuropixels Ultra";
	}
	
	probeMetadata.switchable = switchable;

	Path path;
	path.startNewSubPath(27, 31);
	path.lineTo(27, 514);
	path.lineTo(27 + 10, 542);
	path.lineTo(27 + 20, 514);
	path.lineTo(27 + 20, 31);
	path.closeSubPath();

	probeMetadata.shank_count = 1;
	probeMetadata.electrodes_per_shank = 384;
	probeMetadata.rows_per_shank = 384 / 8;
	probeMetadata.columns_per_shank = 8;
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
		metadata.xpos = i % 8 * 6.0f + 12.0f; 
		metadata.ypos = (i - (i % 8)) * 6.0f; 
		metadata.column_index = i % 8;
		metadata.row_index = i / 8;
		metadata.site_width = 5;

		metadata.channel = i;
		metadata.bank = Bank::A;
		metadata.status = ElectrodeStatus::CONNECTED;
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