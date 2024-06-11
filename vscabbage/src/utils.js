export class CabbageUtils {
  /**
 * This uses a simple regex pattern to parse a line of Cabbage code such as 
 * rslider bounds(22, 14, 60, 60) channel("clip") thumbRadius(5), text("Clip") range(0, 1, 0, 1, 0.001)
 * and converts it to a JSON object
 */
  static getCabbageCodeAsJSON(text) {
    const regex = /(\w+)\(([^)]+)\)/g;
    const jsonObj = {};

    let match;
    while ((match = regex.exec(text)) !== null) {
      const name = match[1];
      let value = match[2].replace(/"/g, ''); // Remove double quotes

      if (name === 'bounds') {
        // Splitting the value into individual parts for top, left, width, and height
        const [left, top, width, height] = value.split(',').map(v => parseInt(v.trim()));
        jsonObj['left'] = left;
        jsonObj['top'] = top;
        jsonObj['width'] = width;
        jsonObj['height'] = height;
      } else if (name === 'range') {
        // Splitting the value into individual parts for min, max, defaultValue, skew, and increment
        const [min, max, defaultValue, skew, increment] = value.split(',').map(v => parseFloat(v.trim()));
        jsonObj['min'] = min;
        jsonObj['max'] = max;
        jsonObj['defaultValue'] = defaultValue;
        jsonObj['skew'] = skew;
        jsonObj['increment'] = increment;
      } else if (name === 'size') {
        // Splitting the value into individual parts for width and height
        const [width, height] = value.split(',').map(v => parseInt(v.trim()));
        jsonObj['width'] = width;
        jsonObj['height'] = height;
      } else if (name === 'items') {
        // Handling the items attribute
        const items = value.split(',').map(v => v.trim()).join(', ');
        jsonObj['items'] = items;
      } else {
        // Check if the value is a number
        const numericValue = parseFloat(value);
        if (!isNaN(numericValue)) {
          // If it's a number, assign it as a number
          jsonObj[name] = numericValue;
        } else {
          // If it's not a number, assign it as a string
          jsonObj[name] = value;
        }
      }
    }

    return jsonObj;
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
      const codeProps = CabbageUtils.getCabbageCodeAsJSON(line);
      const type = `${line.trimStart().split(' ')[0]}`;
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

  /**
   * this function will return the number of plugin parameter in our widgets array
   */
  static getNumberOfPluginParameters(widgets, ...types) {
    // Create a set from the types for faster lookup
    const typeSet = new Set(types);

    // Initialize the counter
    let count = 0;

    // Iterate over each widget in the array
    for (const widget of widgets) {
      // Check if the widget's type is one of the specified types
      if (typeSet.has(widget.props.type)) {
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
    console.log('props.type:', props.type);
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


  static updateInnerHTML(channel, instance) {
    const element = document.getElementById(channel);
    if (element) {
      element.innerHTML = instance.getInnerHTML();
    }
  }

  static getWidgetDiv(channel) {
    const element = document.getElementById(channel);
    return element || null;
  }

  static updateBounds(props, identifier) {
    const element = document.getElementById(props.channel);
    if(element){
      switch(identifier){
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
  static brighter(hex, amount) {
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
