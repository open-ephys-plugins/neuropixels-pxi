//
//  ColourScheme.hpp
//  ProbeViewerPlugin
//
//  Created by Kelly Fox on 10/16/17.
//  Copyright © 2017 Allen Institute. All rights reserved.
//

#ifndef ColourScheme_h
#define ColourScheme_h

#include <VisualizerEditorHeaders.h>

/**
 *  Colour mapping enumeration describing a predefined set of colours for values
 *  0-1. The maps used and described by these values are derived from libmatplot.
 */
enum class ColourSchemeId : int
{
    INFERNO,
    VIRIDIS,
    PLASMA,
    MAGMA,
    JET
};

namespace ColourScheme
{
/**
     *  Get the colour mapping for a given value, assuming normalized [0,1) and
     *  clipping at the bounds. The colour mapping used can be set using
     *  ColourScheme::setColourScheme
     */
Colour getColourForNormalizedValue (float val);

/**
     *  Get the colour mapping for a given value, with a specific ColourSchemeId and
     *  ignoring the value otherwise stored globally.
     */
Colour getColourForNormalizedValueInScheme (float val, ColourSchemeId colourScheme);

/**
     *  Set the global colour scheme, using this value automatically in
     *  ColourScheme::getColourForNormalizedValue. The default value, if never
     *  set by a user is ColourSchemeId::INFERNO.
     */
void setColourScheme (ColourSchemeId colourScheme);
}; // namespace ColourScheme

#endif /* ColourScheme_h */
