/*
    ------------------------------------------------------------------

    This file is part of the Open Ephys GUI
    Copyright (C) 2024 Open Ephys

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

#ifndef __IMRO_H__
#define __IMRO_H_

#include "../NeuropixComponents.h"

class IMRO
{
public:
    static bool writeSettingsToImro (File& file, ProbeSettings& settings)
    {
        if (file.existsAsFile())
        {
            file.deleteFile();
        }

        const Result result = file.create();

        if (! result.wasOk())
        {
            LOGC ("Could not create file: ", file.getFullPathName());
        }

        String imroPartId;
        String partNumber = settings.probe->info.part_number;

        if (partNumber.startsWith ("NP"))
        {
            imroPartId = partNumber.substring (2);
        }
        else
        {
            if (partNumber.equalsIgnoreCase ("PRB2_1_2_0640_0"))
                imroPartId = "21";
            else if (partNumber.equalsIgnoreCase ("PRB2_4_2_0640_0"))
                imroPartId = "24";
            else if (partNumber.equalsIgnoreCase ("PRB_1_4_0480_1") || partNumber.equalsIgnoreCase ("PRB_1_4_0480_1_C") || partNumber.equalsIgnoreCase ("PRB_1_2_0480_2"))
            {
                imroPartId = "0";
            }
            else
            {
                LOGC ("Unknown part number: ", partNumber);
                return false;
            }
        }

        String imroChannelCount;

        if (settings.probeType == ProbeType::NHP1)
        {
            imroChannelCount = "128";
        }
        else if (settings.probeType == ProbeType::QUAD_BASE)
        {
            imroChannelCount = "1536";
        }
        else
        {
            imroChannelCount = "384";
        }

        if (settings.probeType == ProbeType::UHD2)
        {
            // not yet implemented
            writeUHDFile (file, settings);
            return true;
        }
        else
            file.appendText ("(" + imroPartId + "," + imroChannelCount + ")");

        for (int i = 0; i < settings.selectedChannel.size(); i++)
        {
            String channelInfo = "(" + String (settings.selectedChannel[i]); // channel

            if (settings.probeType == ProbeType::NP2_4 || settings.probeType == ProbeType::QUAD_BASE)
                channelInfo += " " + String (settings.selectedShank[i]); // shank

            if (settings.probeType == ProbeType::NP2_1)
                channelInfo += " " + String (pow (2, int (settings.selectedBank[i]))); // bank
            else
                channelInfo += " " + String (int (settings.selectedBank[i]));

            channelInfo += " " + String (settings.referenceIndex); // reference

            if (settings.probeType == ProbeType::QUAD_BASE
                || settings.probeType == ProbeType::NP2_4)
            {
                channelInfo += " " + String (settings.selectedElectrode[i] - 1280 * settings.selectedShank[i]); // electrode
            }
            else if (settings.probeType == ProbeType::NP2_1)
            {
                channelInfo += " " + String (settings.selectedElectrode[i]); // electrode
            }

            else
            {
                channelInfo += " " + String (int (settings.availableApGains[settings.apGainIndex]));
                channelInfo += " " + String (int (settings.availableLfpGains[settings.lfpGainIndex]));
                channelInfo += " " + String (int (settings.apFilterState));
            }

            channelInfo += ")";

            file.appendText (channelInfo);
        }

        return true;
    }

    static bool readSettingsFromImro (File& file, ProbeSettings& settings)
    {
        String imro = file.loadFileAsString();

        bool foundHeader = false;
        int lastOpeningParen = 0;

        LOGD ("IMRO length: ", imro.length());

        for (int i = 0; i < imro.length(); i++)
        {
            if (imro.substring (i, i + 1) == "(")
            {
                lastOpeningParen = i;
            }

            else if (imro.substring (i, i + 1) == ")")
            {
                //std::cout << imro.substring(lastOpeningParen + 1, i) << std::endl;

                if (! foundHeader)
                {
                    String substring = imro.substring (lastOpeningParen + 1, i);
                    int commaLoc = substring.indexOf (",");

                    int value = substring.substring (0, commaLoc).getIntValue();

                    if (value == 0)
                    {
                        LOGC ("Neuropixels 1.0 probe detected.");
                        settings.probeType = ProbeType::NP1;
                    }
                    else if (value >= 1010 && value <= 1016)
                    {
                        LOGC ("Neuropixels NHP probe 10 mm probe detected.");
                        settings.probeType = ProbeType::NHP10;
                    }
                    else if (value >= 1020 && value <= 1022)
                    {
                        LOGC ("Neuropixels NHP probe 25 mm probe detected.");
                        settings.probeType = ProbeType::NHP25;
                    }
                    else if (value >= 1030 && value <= 1032)
                    {
                        LOGC ("Neuropixels NHP probe 45 mm probe detected.");
                        settings.probeType = ProbeType::NHP45;
                    }
                    else if (value == 1200 || value == 1210)
                    {
                        LOGC ("Neuropixels NHP passive probe detected.");
                        settings.probeType = ProbeType::NHP1;
                    }
                    else if (value == 21 || value == 2000 || value == 2003 || value == 2004)
                    {
                        LOGC ("Neuropixels 2.0 single shank probe detected.");
                        settings.probeType = ProbeType::NP2_1;
                    }
                    else if (value == 24 || value == 2010 || value == 2013 || value == 2014)
                    {
                        LOGC ("Neuropixels 2.0 multi-shank probe detected.");
                        settings.probeType = ProbeType::NP2_4;
                    }
                    else if (value == 2020 || value == 2021)
                    {
                        LOGC ("Neuropixels 2.0 quad base probe detected.");
                        settings.probeType = ProbeType::QUAD_BASE;
                    }
                    else if (value == 1100 || value == 1120 || value == 1121 || value == 1122 || value == 1123)
                    {
                        LOGC ("Neuropixels UHD passive probe detected.");
                        settings.probeType = ProbeType::UHD1;
                    }
                    else if (value == 1110)
                    {
                        LOGC ("Neuropixels UHD active probe detected.");
                        settings.probeType = ProbeType::UHD2;
                    }
                    else
                    {
                        LOGC ("Could not load IMRO, unknown probe part number: ", value);
						return false;
                    }

                    foundHeader = true;
                }
                else
                {
                    StringArray strvals = StringArray::fromTokens (imro.substring (lastOpeningParen + 1, i), " ");

                    Array<int> values;

                    for (int j = 0; j < strvals.size(); j++)
                    {
                        // std::cout << strvals[j] << " ";
                        values.add (strvals[j].getIntValue());
                    }

                    // std::cout << std::endl;

                    parseValues (values, settings.probeType, settings);
                }
            }
        }

        return true;
    }

    static void parseValues (Array<int> values, ProbeType probeType, ProbeSettings& settings)
    {
        if (probeType == ProbeType::NP1 
            || probeType == ProbeType::NHP10 
            || probeType == ProbeType::NHP25
            || probeType == ProbeType::NHP45)
        {
            // 0 = 1.0 probe
            // channel ID
            // bank number
            // reference ID (0=ext, 1=tip, [2..4] = on-shank-ref)
            // AP band gain (e.g. 500)
            // LFP band gain (e.g. 250)
            // AP highpass applied (1 = on)

            Bank bank;

            switch (values[1])
            {
                case 0:
                    bank = Bank::A;
                    break;
                case 1:
                    bank = Bank::B;
                    break;
                case 2:
                    bank = Bank::C;
                    break;
                case 3:
                    bank = Bank::D;
                    break;
                case 4:
                    bank = Bank::E;
                    break;
                case 5:
                    bank = Bank::F;
                    break;
                case 6:
                    bank = Bank::G;
                    break;
                case 7:
                    bank = Bank::H;
                    break;
                case 8:
                    bank = Bank::I;
                    break;
                case 9:
                    bank = Bank::J;
                    break;
                case 10:
                    bank = Bank::K;
                    break;
                case 11:
                    bank = Bank::L;
                    break;
                default:
                    bank = Bank::A;
            }

            settings.selectedChannel.add (values[0]);
            settings.selectedBank.add (bank);
            settings.referenceIndex = values[2];
            settings.apGainIndex = getIndexFromGain (values[3]);
            settings.lfpGainIndex = getIndexFromGain (values[4]);
            settings.apFilterState = bool (values[5]);

            // std::cout << values[0] << " "
            //     << values[1] << " "
            //    << values[2] << " "
            //    << values[3] << " "
            //    << values[4] << " "
            //   << values[5] << std::endl;
        }
        else if (probeType == ProbeType::NP2_1)
        {
            // 21 = 1-shank 2.0
            // channel ID
            // bank mask (logical OR of {1=bnk-0, 2=bnk-1, 4=bnk-2, 8=bnk-3}),
            // reference ID (0=ext, 1=tip, [2..5] = on-shank ref)
            // electrode ID [0,1279]

            settings.selectedChannel.add (values[0]);

            Bank bank;

            switch (values[1])
            {
                case 1:
                    bank = Bank::A;
                    break;
                case 2:
                    bank = Bank::B;
                    break;
                case 4:
                    bank = Bank::C;
                    break;
                case 8:
                    bank = Bank::D;
                    break;
                default:
                    bank = Bank::A;
            }
            settings.selectedBank.add (bank);
            settings.referenceIndex = values[2];
            settings.selectedElectrode.add (values[3]);
        }
        else if (probeType == ProbeType::NP2_4 || probeType == ProbeType::QUAD_BASE)
        {
            // 4-shank 2.0
            // channel ID
            // shank ID
            // bank ID
            // reference ID index (0=ext, [1..4]=tip{0,1,2,3},[5..8]=on shank0, etc.
            // electrode ID [0,1279]

            Bank bank;

            switch (values[2])
            {
                case 0:
                    bank = Bank::A;
                    break;
                case 1:
                    bank = Bank::B;
                    break;
                case 2:
                    bank = Bank::C;
                    break;
                case 3:
                    bank = Bank::D;
                    break;
                default:
                    bank = Bank::A;
            }

            settings.selectedChannel.add (values[0]);
            settings.selectedShank.add (values[1]);
            settings.selectedBank.add (bank);
            settings.referenceIndex = values[3];
            settings.selectedElectrode.add (values[4]);
        }
    }

    static int getIndexFromGain (int value)
    {
        switch (value)
        {
            case 50:
                return 0;
                break;
            case 125:
                return 1;
                break;
            case 250:
                return 2;
                break;
            case 500:
                return 3;
                break;
            case 1000:
                return 4;
                break;
            case 1500:
                return 5;
                break;
            case 2000:
                return 6;
                break;
            case 30000:
                return 7;
                break;
            default:
                return 3;
        }
    }

    static void writeUHDFile (File file, ProbeSettings settings)
    {
        String configuration = settings.availableElectrodeConfigurations[settings.electrodeConfigurationIndex];

        int columnMode = 2; // ALL

        if (configuration.startsWith ("1 x 384"))
            columnMode = 1; // OUTER

        file.appendText ("(1110,"
                         + String (columnMode) + ","
                         + String (settings.referenceIndex) + ","
                         + String ((int) settings.availableApGains[settings.apGainIndex]) + ","
                         + String ((int) settings.availableLfpGains[settings.lfpGainIndex]) + ","
                         + String ((int) settings.apFilterState) + ")");

        // TODO -- write the rest of the file
    }
};

#endif