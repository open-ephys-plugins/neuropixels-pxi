#pragma once

#include "../NeuropixComponents.h"

class IMRO
{
public:

	static bool writeSettingsToImro(File& file, ProbeSettings& settings)
	{
        file.create();

        if (settings.probeType == ProbeType::NP1 || 
            settings.probeType == ProbeType::NHP10 ||
            settings.probeType == ProbeType::NHP25 || 
            settings.probeType == ProbeType::NHP45)
        {
            file.appendText("(0,384)");


        }
        else if (settings.probeType == ProbeType::NP2_1)
        {
            file.appendText("(21,384)");
        }
        else if (settings.probeType == ProbeType::NP2_1)
        {
            file.appendText("(24,384)");
        }
        else {
            return false;
        }

        return true;
        

        // 0 = 1.0 probe
        // channel ID
        // bank number
        // reference ID (0=ext, 1=tip, [2..4] = on-shank-ref)
        // AP band gain (e.g. 500)
        // LFP band gain (e.g. 250)
        // AP highpass applied (1 = on)

        // 21 = 1-shank 2.0
        // channel ID
        // bank mask (logical OR of {1=bnk-0, 2=bnk-1, 4=bnk-2, 8=bnk-3}),
        // reference ID (0=ext, 1=tip, [2..5] = on-shank ref)
        // electrode ID [0,1279]

        // 24 = 4-shank 2.0
        // channel ID
        // shank ID
        // bank ID
        // reference ID index (0=ext, [1..4]=tip{0,1,2,3},[5..8]=on shank0, etc.
        // electrode ID [0,1279]

        // write the rows (r)
       // for (int i = 0; i < probe->channel_count; i++)
       // {
       //     file.appendText(info);
        //}
	}

	static bool readSettingsFromImro(File& file, ProbeSettings& s)
	{

        String settings = file.loadFileAsString();

        StringArray lines = StringArray::fromLines(settings);

        int probeType;

        for (int i = 0; i < lines.size(); i++)
        {
            String noParens = lines[i].substring(1).dropLastCharacters(1);
            StringArray values = StringArray::fromTokens(noParens, " ");

            // if (i == 0)
            //     probeType = values[0].getIntValue();
            // // check that this matches
            // else
                 //parseValues(values, probeType);
        }

        return true;

	}


};
