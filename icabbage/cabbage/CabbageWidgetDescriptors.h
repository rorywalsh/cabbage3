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
              "sliderSkew": 1,
              "increment": 0.001,
              "text": "",
              "valueTextBox": 0,
              "colour": "#dddddd",
              "trackerColour": "#d1d323",
              "trackerBackgroundColour": "#000000",
              "trackerStrokeColour": "#222222",
              "trackerStrokeWidth": 1,
              "trackerWidth": 0.5,
              "outlineWidth" : 0.3,
              "trackerColour": "#dddddd",
              "fontColour": "#222222",
              "textColour": "#222222",
              "outlineColour": "#999999",
              "textBoxOutlineColour": "#999999",
              "textBoxColour": "#555555",
              "markerColour": "#222222",
              "markerThickness": 0.2,
              "markerStart": 0.5,
              "markerEnd": 0.9,
              "name": "",
              "type": "rslider",
              "kind": "rotary",
              "decimalPlaces": 1,
              "velocity": 0,
              "identChannel": "",
              "trackerStart": 0.1,
              "trackerEnd": 0.9,
              "trackerCentre": 0.1,
              "visible": 1,
              "automatable": 1,
              "valuePrefix": "",
              "valuePostfix": ""
            }
        )";
        return nlohmann::json::parse(jsonString);
    }
    
};
