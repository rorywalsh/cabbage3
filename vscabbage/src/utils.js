export class CabbageUtils {
  /**
 * This uses a simple regex pattern to parse a line of Cabbage code such as 
 * rslider bounds(22, 14, 60, 60) channel("clip") thumbRadius(5), text("Clip") range(0, 1, 0, 1, 0.001)
 * and converts it to a JSON object
 */
  static getCabbageCodeAsJSON(text) {
    const type = `${text.trimStart().split(' ')[0]}`;
    const regex = /(\w+)\(([^)]+)\)/g;
    const jsonObj = {};

    let match;
    while ((match = regex.exec(text)) !== null) {
      const name = match[1];
      let value = match[2].replace(/"/g, ''); // Remove double quotes

      // console.log(`Processing name: ${name}, value: ${value}`);

      if (name === 'bounds') {
        const [left, top, width, height] = value.split(',').map(v => parseInt(v.trim()));
        jsonObj['left'] = left || 0;
        jsonObj['top'] = top || 0;
        jsonObj['width'] = width || 0;
        jsonObj['height'] = height || 0;
      } else if (name === 'range') {
        const [min, max, val, skew, increment] = value.split(',').map(v => parseFloat(v.trim()));
        jsonObj['min'] = min || 0;
        jsonObj['max'] = max || 0;
        jsonObj['value'] = val || 0;
        jsonObj['skew'] = skew || 0;
        jsonObj['increment'] = increment || 0;
      } else if (name === 'size') {
        const [width, height] = value.split(',').map(v => parseInt(v.trim()));
        jsonObj['width'] = width || 0;
        jsonObj['height'] = height || 0;
      } else if (name === 'sampleRange') {
        const [start, end] = value.split(',').map(v => parseInt(v.trim()));
        jsonObj['startSample'] = start || 0;
        jsonObj['endSample'] = end || 0;
      } else if (name === 'populate') {
        const [directory, fileType] = value.split(',').map(v => (v.trim()));
        jsonObj['currentDirectory'] = directory || '';
        jsonObj['fileType'] = fileType || '';
      } else if (name === 'items') {
        const items = value.split(',').map(v => v.trim()).join(', ');
        jsonObj['items'] = items;
      } else if (name === 'text') {
        if (type.indexOf('button') > -1) {
          const textItems = value.split(',').map(v => v.trim());
          jsonObj['textOff'] = textItems[0] || '';
          jsonObj['textOn'] = (textItems.length > 1 ? textItems[1] : textItems[0]) || '';
        } else {
          jsonObj[name] = value;
        }
      } else if (name === 'samples') {
        const samples = value.split(',').map(v => parseFloat(v.trim()));
        jsonObj['samples'] = samples;
      } else {
        const numericValue = parseFloat(value);
        jsonObj[name] = !isNaN(numericValue) ? numericValue : value;
      }

      // console.log(`jsonObj so far: ${JSON.stringify(jsonObj)}`);
    }

    return jsonObj;
  }

  static updateInnerHTML(channel, instance) {
    const element = document.getElementById(channel);
    if (element) {
      element.innerHTML = instance.getInnerHTML();
    }
  }

  static getFileNameFromPath(fullPath) {
    return fullPath.split(/[/\\]/).pop();
  }

  /**
   * this function parses the Cabbage code and creates new widgets accordingly..
   */
  static async parseCabbageCode(text, widgets, form, insertWidget) {
    // Leave main form in the widget array - there is only one..
    widgets.splice(1, widgets.length - 1);

    let cabbageStart = 0;
    let cabbageEnd = 0;
    let lines = text.split(/\r?\n/);
    let count = 0;

    lines.forEach((line) => {
      if (line.trimStart().startsWith("<Cabbage>"))
        cabbageStart = count + 1;
      else if (line.trimStart().startsWith("</Cabbage>"))
        cabbageEnd = count;
      count++;
    })

    const cabbageCode = lines.slice(cabbageStart, cabbageEnd);
    for (const line of cabbageCode) {

      if (!line.trim().startsWith(";") && line.trim() !== "") {
        console.log("line", line)
        const codeProps = CabbageUtils.getCabbageCodeAsJSON(line);
        const type = `${line.trimStart().split(' ')[0]}`;
        console.log("type", type);
        console.log("copeProps", codeProps);
        if (line.trim() != "") {
          if (type != "form") {
            await insertWidget(type, codeProps);
          } else {
            widgets.forEach((widget) => {
              if (widget.props.channel == "MainForm") {
                const w = codeProps.width;
                const h = codeProps.height;
                form.style.width = w + "px";
                form.style.height = h + "px";
                widget.props.width = w;
                widget.props.width = h;
              }
            });
          }
        }
      }
    }
  }

  /**
   * this function will return the number of plugin parameter in our widgets array
   */
  static getNumberOfPluginParameters(widgets) {

    // Initialize the counter
    let count = 0;

    // Iterate over each widget in the array
    for (const widget of widgets) {
      // Check if the widget's type is one of the specified types
      if (widget.props.automatable === 1) {
        // Increment the counter if the type matches
        count++;
      }
    }

    // Return the final count
    return count;
  }

  /**
   * show / hide Cabbage overlays
   */
  static showOverlay() {
    const overlay = document.getElementById('fullScreenOverlay')
    if (overlay) {
      overlay.style.display = 'flex';
      const leftPanel = document.getElementById('LeftPanel');
      const rightPanel = document.getElementById('RightPanel');
      leftPanel.style.display = 'none';
      rightPanel.style.display = 'none';
    }
  }

  static hideOverlay() {
    const overlay = document.getElementById('fullScreenOverlay')
    if (overlay) {
      overlay.style.display = 'none';
      const leftPanel = document.getElementById('LeftPanel');
      const rightPanel = document.getElementById('RightPanel');
      leftPanel.style.display = 'flex';
      rightPanel.style.display = 'flex';
    }
  }

  /**
   * clamps a value
   * @param {*} num 
   * @param {*} min 
   * @param {*} max 
   * @returns clamped value
   */
  static clamp(num, min, max) {
    return Math.max(min, Math.min(num, max));
  }

  /**
   * returns a remapped value
   * @param {*} value 
   * @param {*} in_min 
   * @param {*} in_max 
   * @param {*} out_min 
   * @param {*} out_max 
   * @returns mapped value
   */
  static map(value, in_min, in_max, out_min, out_max) {
    return ((value - in_min) * (out_max - out_min)) / (in_max - in_min) + out_min;
  };

  /**
   * 
   * @param {*} num 
   * @returns number of decimal places in value
   */
  static getDecimalPlaces(num) {
    const numString = num.toString();
    if (numString.includes('.')) {
      return numString.split('.')[1].length;
    } else {
      return 0;
    }
  }

  /**
   * Returns a unique channel name based on the type and number
   * @param {Array} widgets - Array of JSON objects with unique 'channel' values
   * @returns {String} unique channel name
   */
  static getUniqueChannelName(type, widgets) {
    // Extract all existing channel names
    const existingChannels = widgets.map(widget => widget.channel);

    // Define a function to generate a channel name based on type and a number
    function generateChannelName(type, number) {
      return `${type}${number}`;
    }

    // Start with a number based on the size of the array + 1
    let number = widgets.length + 1;
    let newChannelName = generateChannelName(type, number);

    // Increment the number until a unique channel name is found
    while (existingChannels.includes(newChannelName)) {
      number += 1;
      newChannelName = generateChannelName(type, number);
    }

    return newChannelName;
  }

  static findValidId(event) {
    var target = event.target;

    while (target !== null) {
      if (target.tagName === "DIV" && target.id) {
        return target.id;
      }
      target = target.parentNode;
    }

    return null
  }

  static getElementByIdInChildren(parentElement, targetId) {
    const queue = [parentElement];

    while (queue.length > 0) {
      const currentElement = queue.shift();

      // Check if the current element has the target ID
      if (currentElement.id === targetId) {
        return currentElement;
      }

      // Check if the current element has children
      if (currentElement.children && currentElement.children.length > 0) {
        // Convert HTMLCollection to an array and add the children of the current element to the queue
        const childrenArray = Array.from(currentElement.children);
        queue.push(...childrenArray);
      }
    }

    // If no element with the target ID is found, return null
    return null;
  }

  static getWidgetFromChannel(widgets, channel) {
    widgets.forEach((widget) => {
      if (widget["channel"] === channel)
        return widget;
    })
    return null;
  }

  static getStringWidth(text, props, padding = 10) {
    var canvas = document.createElement('canvas');
    let fontSize = 0;
    switch (props.type) {

      case 'hslider':
        fontSize = props.height * .8;
        break;
      case "rslider":
        fontSize = props.width * .3;
        break;
      case "vslider":
        fontSize = props.width * .3;
        break;
      case "combobox":
        fontSize = props.height * .5;
        break;
      default:
        console.error('getStringWidth..');
        break;
    }

    var ctx = canvas.getContext("2d");
    ctx.font = `${fontSize}px ${props.fontFamily}`;
    var width = ctx.measureText(text).width;
    return width + padding;
  }

  static getNumberBoxWidth(props) {
    // Get the number of decimal places in props.increment
    const decimalPlaces = CabbageUtils.getDecimalPlaces(props.increment);

    // Format props.max with the correct number of decimal places
    const maxNumber = props.max.toFixed(decimalPlaces);

    // Calculate the width of the string representation of maxNumber
    const maxNumberWidth = CabbageUtils.getStringWidth(maxNumber, props);

    return maxNumberWidth;
  }


  static getWidgetDiv(channel) {
    const element = document.getElementById(channel);
    return element || null;
  }

  /**
 * This uses a simple regex pattern to get tokens from a line of Cabbage code
 */
  static getTokens(text) {
    const inputString = text
    const regex = /(\w+)\(([^)]+)\)/g;
    const tokens = [];
    let match;
    while ((match = regex.exec(inputString)) !== null) {
      const token = match[1];
      const values = match[2].split(',').map(value => value.trim()); // Split values into an array
      tokens.push({ token, values });
    }
    return tokens;
  }

  static sendToBack(currentDiv) {
    const parentElement = currentDiv.parentElement;
    const allDivs = parentElement.getElementsByTagName('div');
    console.log(currentDiv);
    console.log(allDivs);
    for (let i = 0; i < allDivs.length; i++) {
      if (allDivs[i] !== currentDiv) {
        allDivs[i].style.zIndex = 1; // Bring other divs to the top
      } else {
        allDivs[i].style.zIndex = 0; // Keep the current div below others
      }
    }
  }
  /**
   * This function will return an identifier in the form of ident(param) from an incoming
   * JSON object of properties
   */
  static getCabbageCodeFromJson(json, name) {
    const obj = JSON.parse(json);
    let syntax = '';

    if (name === 'range' && obj['type'].indexOf('slider') > -1) {
      const { min, max, value, skew, increment } = obj;
      syntax = `range(${min}, ${max}, ${value}, ${skew}, ${increment})`;
      return syntax;
    }
    if (name === 'bounds') {
      const { left, top, width, height } = obj;
      syntax = `bounds(${left}, ${top}, ${width}, ${height})`;
      return syntax;
    }


    for (const key in obj) {
      if (obj.hasOwnProperty(key) && key === name) {
        const value = obj[key];
        // Check if value is string and if so, wrap it in single quotes
        const formattedValue = typeof value === 'string' ? `"${value}"` : value;
        syntax += `${key}(${formattedValue}), `;
      }
    }
    // Remove the trailing comma and space
    syntax = syntax.slice(0, -2);
    return syntax;
  }

  /**
   * This function will check the current widget props against the default set, and return an 
   * array for any identifiers that are different to their default values - this only returns the identifiers
   * that need updating, not their parameters..
   */
  static findUpdatedIdentifiers(initial, current) {
    const initialWidgetObj = JSON.parse(initial);
    const currentWidgetObj = JSON.parse(current);

    var updatedIdentifiers = [];

    // Iterate over the keys of obj1
    for (var key in initialWidgetObj) {
      // Check if obj2 has the same key
      if (currentWidgetObj.hasOwnProperty(key)) {
        // Compare the values of the keys
        if (initialWidgetObj[key] !== currentWidgetObj[key]) {
          // If values are different, add the key to the differentKeys array
          updatedIdentifiers.push(key);
        }
      } else {
        // If obj2 doesn't have the key from obj1, add it to differentKeys array
        updatedIdentifiers.push(key);
      }
    }

    // Iterate over the keys of obj2 to find any keys not present in obj1
    for (var key in currentWidgetObj) {
      if (!initialWidgetObj.hasOwnProperty(key)) {
        // Add the key to differentKeys array
        updatedIdentifiers.push(key);
      }
    }


    if (currentWidgetObj['type'].indexOf('slider') > -1) {
      updatedIdentifiers.push('min');
      updatedIdentifiers.push('max');
      updatedIdentifiers.push('value');
      updatedIdentifiers.push('skew');
      updatedIdentifiers.push('increment');

    }

    return updatedIdentifiers;
  }


  static generateIdentifierTestCsd(widgets) {



  }

  static updateBounds(props, identifier) {
    const element = document.getElementById(props.channel);
    if (element) {
      switch (identifier) {
        case 'left':
          element.style.left = props.left + "px";
          break;
        case 'top':
          element.style.top = props.top + "px";
          break;
        case 'width':
          element.style.width = props.width + "px";
          break;
        case 'height':
          element.style.height = props.height + "px";
          break;
      }
    }
  }
}

export class CabbageColours {
  static getColour(colourName) {
    const colourMap = {
      "blue": "#0295cf",
      "green": "#93d200",
      "red": "#ff0000",
      "yellow": "#f0e14c",
      "purple": "#a020f0",
      "orange": "#ff6600",
      "grey": "#808080",
      "white": "#ffffff",
      "black": "#000000"
    };

    return colourMap[colourName] || colourMap["blue"];
  }
  static lighter(hex, amount) {
    return this.adjustBrightness(hex, amount);
  }

  static darker(hex, amount) {
    return this.adjustBrightness(hex, -amount);
  }

  static adjustBrightness(hex, factor) {
    // Remove the hash at the start if it's there
    hex = hex.replace(/^#/, '');

    // Parse r, g, b values
    let r = parseInt(hex.slice(0, 2), 16);
    let g = parseInt(hex.slice(2, 4), 16);
    let b = parseInt(hex.slice(4, 6), 16);

    // Apply the factor to each color component
    r = Math.round(Math.min(255, Math.max(0, r + (r * factor))));
    g = Math.round(Math.min(255, Math.max(0, g + (g * factor))));
    b = Math.round(Math.min(255, Math.max(0, b + (b * factor))));

    // Convert back to hex and pad with zeroes if necessary
    r = r.toString(16).padStart(2, '0');
    g = g.toString(16).padStart(2, '0');
    b = b.toString(16).padStart(2, '0');

    return `#${r}${g}${b}`;
  }

}

/*
* This class contains utility functions for testing the Cabbage UI
*/
export class CabbageTestUtilities {

  /*
  * Generate a CabbageWidgetDescriptors class with all the identifiers for each widget type, this can be inserted
  directly into the Cabbage source code
  */
  static generateCabbageWidgetDescriptorsClass(widgets) {
    let widgetTypes = '{';
    widgets.forEach((widget) => {
      widgetTypes += `"${widget.props.type}", `;
    });

    widgetTypes = widgetTypes.slice(0, -2) + "};";

    let cppCode = `
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
        return ${widgetTypes};
    }

    static nlohmann::json get(std::string widgetType) {
`;

    // Generate the widget descriptors for each widget type
    widgets.forEach((widget) => {
      const jsonString = JSON.stringify(widget.props, null, 2).split('\n').map(line => `            ${line}`).join('\n');
      cppCode += `
        if (widgetType == "${widget.props.type}") {
            std::string jsonString = R"(
${jsonString}
            )";
            return nlohmann::json::parse(jsonString);
        }`;
    });

    cppCode += `
        cabAssert(false, "Invalid widget type");
    }
};
`;

    console.log(cppCode);
  }

  /*
  * Generate a CSD file from the widgets array and tests all identifiers. For now this only tests numeric values
  * for each widget type using cabbageSetValue and only string types for cabbageSet
  */
  static generateIdentifierTestCsd(widgets) {
    let csoundCode = "<Cabbage>\nform size(800, 400)";

    widgets.forEach((widget) => {
      csoundCode += `   ${widget.props.type} bounds(-1000, 0, 100, 100)\n`;
    });

    csoundCode += `   csoundoutput bounds(0, 0, 780, 380)\n`;
    csoundCode += "</Cabbage>\n";

    csoundCode += `
<CsoundSynthesizer>
<CsOptions>
-n -d -m0d
</CsOptions> 
<CsInstruments>
; Initialize the global variables. 
ksmps = 32
nchnls = 2
0dbfs = 1
`;

    // Instrument for setting string values
    csoundCode += `
    
giErrorCnt init 0
giIdentifiersChecked init 0    

instr CabbageSetString
  SChannel strcpy p4
  SIdentifier strcpy p5
  SString strcpy p6
  cabbageSet SChannel, sprintf("%s(\\"%s\\")", SIdentifier, SString)
endin

instr CabbageCheckString
  SChannel strcpy p4
  SIdentifier strcpy p5
  SString strcpy p6
  S1 cabbageGet SChannel, SIdentifier
  iRes strcmp S1, SString
  if iRes != 0 then
      prints("")
      prints("=========CabbageCheckString============")
      prints("")
      prints sprintf("CabbageCheckString Error: %s %s", SChannel, SIdentifier)
      prints sprintf("CurrentValue: [%s] Incoming value: [%s]", S1, SString)
      prints sprintf("Size of string: [%d] Incoming size: [%d]", strlen(S1), strlen(SString))
      giErrorCnt += 1
  endif
  giIdentifiersChecked += 1
  prints(sprintf("Checked %d identifiers", giIdentifiersChecked))
endin

instr CabbageSetFloat
  SChannel strcpy p4
  SIdentifier strcpy p5
  SString = sprintf("%s(%3.3f)", SIdentifier, p6)
  cabbageSet SChannel,SString 
endin

instr CabbageCheckFloat
  SChannel strcpy p4
  SIdentifier strcpy p5
  i1 cabbageGet SChannel, SIdentifier
  ;checking floats can be iffy..
  if i1 <= p6-0.01 || i1 >= p6+0.01 then
        prints("")
        prints("=========CabbageCheckInt============")
        prints("")
        prints sprintf("CabbageCheckFloat Error: %s %s", SChannel, SIdentifier)
        prints sprintf("CurrentValue: [%f] Incoming value: [%f]", i1, p6)
        giErrorCnt += 1
  endif
  giIdentifiersChecked += 1
  prints(sprintf("Checked %d identifiers", giIdentifiersChecked))
endin

instr CabbageSetValue
  SChannel strcpy p4
  cabbageSetValue SChannel, p5
endin

instr CabbageCheckValue
  SChannel strcpy p4
  i1 cabbageGetValue SChannel
  if i1 != p5 then
      prints("")
      prints("=========CabbageCheckValue============")
      prints("")
      prints sprintf("CabbageCheckValue Error: %s %s", SChannel, "value")
      prints sprintf("CurrentValue: [%f] Incoming value: [%f]", i1, p5)
      giErrorCnt += 1
  endif
  giIdentifiersChecked += 1
  prints(sprintf("Checked %d identifiers", giIdentifiersChecked))
endin

instr GetErrorCount
  prints("")
  prints("")
  prints("===========Error report ================")
  prints sprintf("Number of identifiers checked: %d", giIdentifiersChecked)
  prints sprintf("Number of errors found: %d", giErrorCnt)
endin
`;

    csoundCode += '</CsInstruments>\n';

    // Generate CsScore section
    csoundCode += '<CsScore>\n';

    let delay = 0.2; // Delay between each set/check pair (in seconds)
    let setStartTime = 1.0; // Start time for score events
    let checkStartTime = setStartTime + 0.1; // Start time for score events

    widgets.forEach((widget) => {
      for (const [key, value] of Object.entries(widget.props)) {
        if (key !== 'type' && key !== 'index' && key !== 'channel') {
          if (key !== 'value' && key !== 'defaultValue') {
            const newValue = CabbageTestUtilities.getSimilarValue(value);
            if (typeof value === 'number') {
              csoundCode += `i"CabbageSetFloat" ${setStartTime.toFixed(1)} 0.2 "${widget.props.channel}" "${key}" ${newValue}\n`;
              csoundCode += `i"CabbageCheckFloat" ${checkStartTime.toFixed(1)} 0.2 "${widget.props.channel}" "${key}" ${newValue}\n`;
            } else {
              csoundCode += `i"CabbageSetString" ${setStartTime.toFixed(1)} 0.2 "${widget.props.channel}" "${key}" "${newValue}"\n`;
              csoundCode += `i"CabbageCheckString" ${checkStartTime.toFixed(1)} 0.2 "${widget.props.channel}" "${key}" "${newValue}"\n`;
            }
            setStartTime += delay;
            checkStartTime += delay;
          } else if (key === 'value') {
            const newValue = CabbageTestUtilities.getSimilarValue(value);
            csoundCode += `i"CabbageSetValue" ${setStartTime.toFixed(1)} 0.2 "${widget.props.channel}" ${newValue}\n`;
            csoundCode += `i"CabbageCheckValue" ${checkStartTime.toFixed(1)} 0.2 "${widget.props.channel}" ${newValue}\n`;
            setStartTime += delay;
            checkStartTime += delay;
          }
        }
      }
    });

    csoundCode += `i"GetErrorCount" ${setStartTime.toFixed(1)} 0.2\n`;
    csoundCode += '</CsScore>\n';
    csoundCode += '</CsoundSynthesizer>\n';

    console.log(csoundCode);
  }



  static getSimilarValue(value) {
    if (typeof value === 'string') {
      if (/^#[0-9a-fA-F]{6,8}$/.test(value)) {
        // Hex color code
        return this.generateRandomHexColor(value.length);
      } else if (/^[0-9., ]*$/.test(value)) {
        // Number string (comma-separated, can include floating point)
        return this.generateRandomCommaSeparatedNumbers(value);
      } else if (value.trim() === '') {
        // Empty string
        return this.generateRandomString(5); // Default length of 5 for empty strings
      } else {
        // Comma-separated words
        return this.generateRandomCommaSeparatedWords(value);
      }
    } else if (typeof value === 'number') {
      // Number
      return this.generateRandomNumber(value);
    } else if (value === null || value === undefined) {
      // Null or undefined
      return this.generateRandomString(5); // Default length of 5 for unknown types
    } else {
      // Any other type (including empty arrays/objects, which are uncommon in typical JSON usage)
      return this.generateRandomString(5); // Default length of 5 for unknown types
    }
  }

  static generateRandomHexColor(length) {
    let hex = '#';
    for (let i = 0; i < length - 1; i++) {
      hex += Math.floor(Math.random() * 16).toString(16);
    }
    return hex;
  }

  static generateRandomCommaSeparatedNumbers(value) {
    if (value.trim() === '') {
      // Handle empty string by returning a default random string
      return this.generateRandomString(5);
    }

    return value.split(',').map(num => {
      num = num.trim();
      if (num === '') {
        return this.generateRandomString(5); // Handle empty parts
      } else if (num.includes('.')) {
        // Floating point number
        const floatValue = parseFloat(num);
        if (floatValue < 1) {
          return (Math.random()).toFixed(2); // Generate a new number between 0 and 1
        } else {
          return (floatValue + (Math.random() * 10 - 5)).toFixed(2);
        }
      } else {
        // Integer number
        const intValue = parseInt(num);
        return (intValue + Math.floor(Math.random() * 10) + 1).toString(); // Ensure it's not zero
      }
    }).join(', ');
  }

  static generateRandomCommaSeparatedWords(value) {
    if (value.trim() === '') {
      // Handle empty string by returning a default random word
      return this.generateRandomString(5);
    }

    const words = value.split(',').map(word => {
      if (word.trim() === '') {
        return this.generateRandomString(5); // Handle empty parts
      } else {
        return this.generateRandomString(word.trim().length);
      }
    });
    return words.join(', ');
  }

  static generateRandomString(length) {
    const characters = 'abcdefghijklmnopqrstuvwxyz';
    let result = '';
    for (let i = 0; i < length; i++) {
      result += characters.charAt(Math.floor(Math.random() * characters.length));
    }
    return result;
  }

  static generateRandomNumber(value) {
    if (Number.isInteger(value)) {
      return value + Math.floor(Math.random() * 10) + 1; // Ensure it's not zero
    } else {
      if (value < 1) {
        return (Math.random()).toFixed(2); // Generate a new number between 0 and 1
      } else {
        return (value + (Math.random() * 10 - 5)).toFixed(2); // For floating point numbers >= 1
      }
    }
  }



}