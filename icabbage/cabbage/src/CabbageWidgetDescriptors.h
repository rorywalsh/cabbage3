#pragma once

#include <iostream>
#include <regex>
#include <string>
#include <vector>
#include "json.hpp"

class CabbageWidgetDescriptors {
public:
    static nlohmann::json getRotarySlider()
    {
        std::string jsonString = R"(
        {
            "top": 10,
            "left": 10,
            "width": 60,
            "height": 60,
            "channel": "rslider",
            "min": 0,
            "max": 1,
            "value": 0,
            "defaultValue": 0,
            "skew": 1,
            "increment": 0.001,
            "index": 0,
            "text": "",
            "fontFamily": "Verdana",
            "fontSize": 0,
            "align": "centre",
            "textOffsetY": 0,
            "valueTextBox": 0,
            "colour": "#0000FF",
            "trackerColour": "#00FF00",
            "trackerBackgroundColour": "#ffffff",
            "trackerOutlineColour": "#525252",
            "fontColour": "#dddddd",
            "outlineColour": "#525252",
            "textBoxOutlineColour": "#999999",
            "textBoxColour": "#555555",
            "markerColour": "#222222",
            "trackerOutlineWidth": 3,
            "trackerWidth": 20,
            "outlineWidth": 2,
            "type": "rslider",
            "kind": "rotary",
            "decimalPlaces": 1,
            "velocity": 0,
            "popup": 1,
            "visible": 1,
            "automatable": 1,
            "valuePrefix": "",
            "valuePostfix": "",
            "presetIgnore": 0
        }
        )";
        return nlohmann::json::parse(jsonString);
    }
    
};
