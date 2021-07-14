#ifndef __IMRO_H__
#define __IMRO_H_

#include "../NeuropixComponents.h"
#include "../Utils.h"

class IMRO
{
public:

	static bool writeSettingsToImro(File& file, ProbeSettings& settings)
	{
        file.create();

        if (settings.probeType == ProbeType::NP1 || 
            settings.probeType == ProbeType::NHP10 ||
            settings.probeType == ProbeType::NHP25 || 
            settings.probeType == ProbeType::NHP45 ||
            settings.probeType == ProbeType::UHD1)
        {
            file.appendText("(0,384)");
        }
        else if (settings.probeType == ProbeType::NP2_1)
        {
            file.appendText("(21,384)");
        }
        else if (settings.probeType == ProbeType::NP2_4)
        {
            file.appendText("(24,384)");
        }
        else if (settings.probeType == ProbeType::NHP1)
        {
            file.appendText("(0,128)");
        }
            
        else {
            return false;
        }

        for (int i = 0; i < settings.selectedChannel.size(); i++)
        {
            String channelInfo = "(" + String(settings.selectedChannel[i]); // channel

            if (settings.probeType == ProbeType::NP2_4)
                channelInfo += " " + String(settings.selectedShank[i]); // shank

            if (settings.probeType == ProbeType::NP2_1)
                channelInfo += " " + String(pow(2,int(settings.selectedBank[i]))); // bank
            else
                channelInfo += " " + String(settings.selectedBank[i]);

            channelInfo += " " + String(settings.referenceIndex); // reference

            if (settings.probeType == ProbeType::NP2_1 ||
                settings.probeType == ProbeType::NP2_4)
            {
                channelInfo += " " + String(settings.selectedElectrode[i]); // electrode
            }
                
            else {
                channelInfo += " " + String(int(settings.availableApGains[settings.apGainIndex]));
                channelInfo += " " + String(int(settings.availableLfpGains[settings.lfpGainIndex]));
                channelInfo += " " + String(int(settings.apFilterState));
            }

            channelInfo += ")";
            
            file.appendText(channelInfo);
        }

        return true;
        

	}

	static bool readSettingsFromImro(File& file, ProbeSettings& settings)
	{

        String imro = file.loadFileAsString();

        bool foundHeader = false;
        int lastOpeningParen = 0;

        LOGD("Length: ", imro.length());

        for (int i = 0; i < imro.length(); i++)
        {
            if (imro.substring(i, i + 1) == "(")
            {
                lastOpeningParen = i;
            }

            else if (imro.substring(i, i + 1) == ")")
            {
                //std::cout << imro.substring(lastOpeningParen + 1, i) << std::endl;

                if (!foundHeader)
                {
                    String substring = imro.substring(lastOpeningParen + 1, i);
                    int commaLoc = substring.indexOf(",");

                    int value = substring.substring(0, commaLoc).getIntValue();

                    std::cout << value << std::endl;

                    if (value == 0)
                        settings.probeType = ProbeType::NP1;
                    else if (value == 21)
                        settings.probeType = ProbeType::NP2_1;
                    else if (value == 24)
                        settings.probeType = ProbeType::NP2_4;
                    else
                        return false;

                    foundHeader = true;
                }
                else {
                    StringArray strvals = StringArray::fromTokens(imro.substring(lastOpeningParen+1, i), " ");

                    Array<int> values;

                    for (int j = 0; j < strvals.size(); j++)
                    {
                        // std::cout << strvals[j] << " ";
                        values.add(strvals[j].getIntValue());
                    }

                    // std::cout << std::endl;

                    parseValues(values, settings.probeType, settings);
                }
            }

        }

        return true;

	}


    static void parseValues(Array<int> values, ProbeType probeType, ProbeSettings& settings)
    {
        if (probeType == ProbeType::NP1)
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
            default:
                bank = Bank::A;
            }

            settings.selectedChannel.add(values[0]);
            settings.selectedBank.add(bank);
            settings.referenceIndex = values[2];
            settings.apGainIndex = getIndexFromGain(values[3]);
            settings.lfpGainIndex = getIndexFromGain(values[4]);
            settings.apFilterState = bool(values[5]);

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

            settings.selectedChannel.add(values[0]);

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
            settings.selectedBank.add(bank);
            settings.referenceIndex = values[2];
            settings.selectedElectrode.add(values[3]);

        }
        else if (probeType == ProbeType::NP2_4)
        {

            // 24 = 4-shank 2.0
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

            settings.selectedChannel.add(values[0]);
            settings.selectedShank.add(values[1]);
            settings.selectedBank.add(bank);
            settings.referenceIndex = values[3];
            settings.selectedElectrode.add(values[4]);

            
        }
    }

    static int getIndexFromGain(int value)
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


};

#endif