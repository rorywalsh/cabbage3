
#pragma once

/* this file is generated by CabbageUtils.js */
#include <iostream>
#include <regex>
#include <string>
#include <vector>
#include "json.hpp"
#include "CabbageUtils.h"

class CabbageWidgetDescriptors {
public:
    static std::vector<std::string> getWidgetTypes(){
        return {"button", "checkbox", "combobox", "csoundoutput", "filebutton", "form", "gentable", "hslider", "label", "keyboard", "rslider", "texteditor", "vslider", "optionbutton"};;
    }

    static nlohmann::json get(std::string widgetType) {

        if (widgetType == "button") {
            std::string jsonString = R"(
            {
              "top": 10,
              "left": 10,
              "width": 80,
              "height": 30,
              "channel": "button",
              "corners": 2,
              "min": 0,
              "max": 1,
              "value": 0,
              "textOn": "On",
              "textOff": "Off",
              "fontFamily": "Verdana",
              "fontSize": 0,
              "align": "centre",
              "colourOn": "#0295cf",
              "colourOff": "#0295cf",
              "fontColourOn": "#dddddd",
              "fontColourOff": "#dddddd",
              "outlineColour": "#dddddd",
              "outlineWidth": 2,
              "name": "",
              "type": "button",
              "visible": 1,
              "automatable": 1,
              "presetIgnore": 0
            }
            )";
            return nlohmann::json::parse(jsonString);
        }
        if (widgetType == "checkbox") {
            std::string jsonString = R"(
            {
              "top": 10,
              "left": 10,
              "width": 100,
              "height": 30,
              "channel": "checkbox",
              "corners": 2,
              "min": 0,
              "max": 1,
              "value": 0,
              "text": "On/Off",
              "fontFamily": "Verdana",
              "fontColour": "#dddddd",
              "fontSize": 14,
              "align": "left",
              "colourOn": "#93d200",
              "colourOff": "#ffffff",
              "fontColourOn": "#dddddd",
              "fontColourOff": "#000000",
              "outlineColour": "#999999",
              "outlineWidth": 1,
              "type": "checkbox",
              "visible": 1,
              "automatable": 1,
              "presetIgnore": 0
            }
            )";
            return nlohmann::json::parse(jsonString);
        }
        if (widgetType == "combobox") {
            std::string jsonString = R"(
            {
              "top": 10,
              "left": 10,
              "width": 100,
              "height": 30,
              "channel": "comboBox",
              "corners": 4,
              "fontFamily": "Verdana",
              "fontSize": 14,
              "align": "center",
              "colour": "#0295cf",
              "items": "One, Two, Three",
              "text": "Select",
              "fontColour": "#dddddd",
              "outlineColour": "#dddddd",
              "outlineWidth": 2,
              "visible": 1,
              "type": "combobox",
              "value": 0,
              "active": 1
            }
            )";
            return nlohmann::json::parse(jsonString);
        }
        if (widgetType == "csoundoutput") {
            std::string jsonString = R"(
            {
              "top": 0,
              "left": 0,
              "width": 200,
              "height": 300,
              "type": "csoundoutput",
              "colour": "#000000",
              "channel": "csoundoutput",
              "fontColour": "#dddddd",
              "fontFamily": "Verdana",
              "fontSize": 14,
              "corners": 4,
              "align": "left",
              "visible": 1,
              "text": "Csound Output\n",
              "automatable": 0
            }
            )";
            return nlohmann::json::parse(jsonString);
        }
        if (widgetType == "filebutton") {
            std::string jsonString = R"(
            {
              "top": 10,
              "left": 10,
              "width": 80,
              "height": 30,
              "channel": "fileButton",
              "corners": 2,
              "min": 0,
              "max": 1,
              "value": 0,
              "textOn": "Off",
              "textOff": "Off",
              "fontFamily": "Verdana",
              "fontSize": 0,
              "align": "centre",
              "colourOn": "#0295cf",
              "colourOff": "#0295cf",
              "fontColourOn": "#dddddd",
              "fontColourOff": "#dddddd",
              "outlineColour": "#dddddd",
              "outlineWidth": 2,
              "name": "",
              "type": "filebutton",
              "visible": 1,
              "automatable": 0,
              "presetIgnore": 0,
              "mode": "file",
              "text": "Choose File"
            }
            )";
            return nlohmann::json::parse(jsonString);
        }
        if (widgetType == "form") {
            std::string jsonString = R"(
            {
              "width": 600,
              "height": 300,
              "caption": "",
              "type": "form",
              "colour": "#888888",
              "channel": "MainForm"
            }
            )";
            return nlohmann::json::parse(jsonString);
        }
        if (widgetType == "gentable") {
            std::string jsonString = R"(
            {
              "top": 0,
              "left": 0,
              "width": 200,
              "height": 100,
              "type": "gentable",
              "colour": "#888888",
              "outlineColour": "#dddddd",
              "outlineWidth": 1,
              "channel": "gentable",
              "backgroundColour": "#a8d388",
              "fontColour": "#dddddd",
              "fontFamily": "Verdana",
              "startSample": -1,
              "endSample": -1,
              "file": "",
              "fontSize": 0,
              "corners": 4,
              "align": "left",
              "visible": 1,
              "text": "",
              "tableNumber": 1,
              "samples": [],
              "automatable": 0
            }
            )";
            return nlohmann::json::parse(jsonString);
        }
        if (widgetType == "hslider") {
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
              "skew": 1,
              "increment": 0.001,
              "text": "",
              "fontFamily": "Verdana",
              "fontSize": 0,
              "align": "centre",
              "valueTextBox": 0,
              "colour": "#0295cf",
              "trackerColour": "#93d200",
              "trackerBackgroundColour": "#ffffff",
              "trackerOutlineColour": "#525252",
              "fontColour": "#dddddd",
              "outlineColour": "#999999",
              "textBoxColour": "#555555",
              "trackerOutlineWidth": 1,
              "outlineWidth": 1,
              "markerThickness": 0.2,
              "markerStart": 0.1,
              "markerEnd": 0.9,
              "type": "hslider",
              "decimalPlaces": 1,
              "velocity": 0,
              "visible": 1,
              "popup": 1,
              "automatable": 1,
              "valuePrefix": "",
              "valuePostfix": "",
              "presetIgnore": 0
            }
            )";
            return nlohmann::json::parse(jsonString);
        }
        if (widgetType == "label") {
            std::string jsonString = R"(
            {
              "top": 0,
              "left": 0,
              "width": 100,
              "height": 30,
              "type": "label",
              "colour": "#888888",
              "channel": "label",
              "fontColour": "#dddddd",
              "fontFamily": "Verdana",
              "fontSize": 0,
              "corners": 4,
              "align": "centre",
              "visible": 1,
              "text": "Default Label",
              "automatable": 0
            }
            )";
            return nlohmann::json::parse(jsonString);
        }
        if (widgetType == "keyboard") {
            std::string jsonString = R"(
            {
              "top": 0,
              "left": 0,
              "width": 600,
              "height": 300,
              "type": "keyboard",
              "colour": "#888888",
              "channel": "keyboard",
              "blackNoteColour": "#000",
              "value": "36",
              "fontFamily": "Verdana",
              "whiteNoteColour": "#fff",
              "keySeparatorColour": "#000",
              "arrowBackgroundColour": "#0295cf",
              "mouseoverKeyColour": "#93d200",
              "keydownColour": "#93d200",
              "octaveButtonColour": "#00f",
              "automatable": 0
            }
            )";
            return nlohmann::json::parse(jsonString);
        }
        if (widgetType == "rslider") {
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
              "skew": 1,
              "increment": 0.001,
              "index": 0,
              "text": "",
              "fontFamily": "Verdana",
              "fontSize": 0,
              "align": "centre",
              "textOffsetY": 0,
              "valueTextBox": 0,
              "colour": "#0295cf",
              "trackerColour": "#93d200",
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
        if (widgetType == "texteditor") {
            std::string jsonString = R"(
            {
              "top": 0,
              "left": 0,
              "width": 200,
              "height": 300,
              "type": "texteditor",
              "colour": "#dddddd",
              "channel": "texteditor",
              "fontColour": "#222222",
              "fontFamily": "Verdana",
              "fontSize": 14,
              "corners": 4,
              "align": "left",
              "visible": 1,
              "text": "",
              "automatable": 0
            }
            )";
            return nlohmann::json::parse(jsonString);
        }
        if (widgetType == "vslider") {
            std::string jsonString = R"(
            {
              "top": 10,
              "left": 10,
              "width": 60,
              "height": 60,
              "channel": "vslider",
              "min": 0,
              "max": 1,
              "value": 0,
              "skew": 1,
              "increment": 0.001,
              "text": "",
              "fontFamily": "Verdana",
              "fontSize": 0,
              "align": "centre",
              "valueTextBox": 0,
              "colour": "#0295cf",
              "trackerColour": "#93d200",
              "trackerBackgroundColour": "#ffffff",
              "trackerOutlineColour": "#525252",
              "fontColour": "#dddddd",
              "outlineColour": "#999999",
              "textBoxColour": "#555555",
              "trackerOutlineWidth": 1,
              "outlineWidth": 1,
              "type": "vslider",
              "decimalPlaces": 1,
              "velocity": 0,
              "visible": 1,
              "popup": 1,
              "automatable": 1,
              "valuePrefix": "",
              "valuePostfix": "",
              "presetIgnore": 0
            }
            )";
            return nlohmann::json::parse(jsonString);
        }
        if (widgetType == "optionbutton") {
            std::string jsonString = R"(
            {
              "top": 10,
              "left": 10,
              "width": 80,
              "height": 30,
              "channel": "optionButton",
              "corners": 2,
              "min": 0,
              "max": 1,
              "value": 0,
              "textOn": "Off",
              "textOff": "Off",
              "fontFamily": "Verdana",
              "fontSize": 0,
              "align": "centre",
              "colourOn": "#0295cf",
              "colourOff": "#0295cf",
              "fontColourOn": "#dddddd",
              "fontColourOff": "#dddddd",
              "outlineColour": "#dddddd",
              "outlineWidth": 2,
              "name": "",
              "type": "optionbutton",
              "visible": 1,
              "automatable": 1,
              "presetIgnore": 0,
              "items": "One, Two, Three",
              "text": ""
            }
            )";
            return nlohmann::json::parse(jsonString);
        }
        cabAssert(false, "Invalid widget type");
    }
};
