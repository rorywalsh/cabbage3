

import { Form, RotarySlider } from "./widgets.js";

let vscode = null;
let widgetWrappers = null;

if (typeof acquireVsCodeApi === 'function') {
  vscode = acquireVsCodeApi();
  try {
    const module = await import("./widgetWrapper.js");
    const { WidgetWrapper } = module;
    // You can now use WidgetWrapper here
    console.log('WidgetWrapper loaded:', WidgetWrapper);
    widgetWrappers = new WidgetWrapper(updatePanel);
  } catch (error) {
    console.error("Error loading widgetWrapper.js:", error);
  }
}

const currentWidget = [{ name: "Top", value: 0 }, { name: "Left", value: 0 }, { name: "Width", value: 0 }, { name: "Height", value: 0 }];

const widgets = [];

let cabbageMode = 'editMode';
//adding this messes up dragging of main form
widgets.push(new Form());

let numberOfWidgets = 1;
const contextMenu = document.querySelector(".wrapper");
const form = document.getElementById('MainForm');




function updateWidget(obj) {
  const channel = obj['channel'];
  for (const widget of widgets) {
    if (widget.props.name == channel) {
      if (obj.hasOwnProperty('value')) {
        widget.props.value = obj['value'];
        console.log(widget.props.value)
      }
      else if (obj.hasOwnProperty('data')) {
        const identifierStr = obj['data'];
        Object.entries(getCabbageCodeAsJSON(obj['data'])).forEach((entry) => {
          const [key, value] = entry;
          widget.props[key] = value;
        });
      }
      document.getElementById(widget.props.name).style.transform = 'translate(' + widget.props.left + 'px,' + widget.props.top + 'px)';
      document.getElementById(widget.props.name).setAttribute('data-x', widget.props.left);
      document.getElementById(widget.props.name).setAttribute('data-y', widget.props.top);
      document.getElementById(widget.props.name).innerHTML = widget.getSVG();
    }
  }
}

/**
 * this function will return the number of plugin parameter in our widgets array
 */
function getNumberOfPluginParameters(...types) {
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
 * This uses a simple regex pattern to parse a line of Cabbage code such as 
 * rslider bounds(22, 14, 60, 60) channel("clip") thumbRadius(5), text("Clip") range(0, 1, 0, 1, 0.001)
 * and converts it to a JSON object
 */
function getCabbageCodeAsJSON(text) {
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
    }
    else if (name === 'range') {
      // Splitting the value into individual parts for top, left, width, and height
      const [min, max, initValue, sliderSkew, increment] = value.split(',').map(v => parseFloat(v.trim()));
      jsonObj['min'] = min;
      jsonObj['max'] = max;
      jsonObj['value'] = initValue;
      jsonObj['sliderSkew'] = sliderSkew;
      jsonObj['increment'] = increment;
    }
    else if (name === 'size') {
      // Splitting the value into individual parts for width and height
      const [width, height] = value.split(',').map(v => parseInt(v.trim()));
      jsonObj['width'] = width;
      jsonObj['height'] = height;
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
 * called whenever a user saves/updates or changes .csd file
 */
window.addEventListener('message', event => {
  const message = event.data;
  switch (message.command) {
    case 'onFileChanged':
      cabbageMode = 'playMode';
      form.className = "form";
      parseCabbageCsdTile(message.text);
      break;
    case 'widgetUpdate':
      const msg = JSON.parse(message.text);
      updateWidget(msg);
      break;
    case 'onEnterEditMode':
      cabbageMode = 'editMode';
      form.className = "form editMode";
      parseCabbageCsdTile(message.text);
      break;
    default:
      return;
  }
});

/**
 * this function parses the Cabbage code and creates new widgets accordingly..
 */
function parseCabbageCsdTile(text) {
  //leave main form in the widget array - there is only one..
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
  cabbageCode.forEach(async (line) => {
    const codeProps = getCabbageCodeAsJSON(line);
    const type = `${line.trimStart().split(' ')[0]}`;
    if (line.trim() != "") {
      if (type != "form") {
        await insertWidget(type, codeProps);
        numberOfWidgets++;
      }
      else {
        widgets.forEach((widget) => {
          if (widget.props.name == "MainForm") {
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

  });


}

/**
 * this callback is triggered whenever a user move/drags a widget in edit modes
 * The innerHTML is constantly updated. When this is called, the editor is also
 * updated accordingly. 
 */
function updatePanel(eventType, name, bounds) {
  const element = document.querySelector('.property-panel');
  element.style.visibility = "visible";

  if (element)
    element.innerHTML = '';

  widgets.forEach((widget) => {

    // DBG(JSON.stringify(widget.props));
    if (widget.props.name == name) {
      // DBG(widget.name, name);
      if (eventType != 'click') {
        widget.props.left = Math.floor(bounds.x);
        widget.props.top = Math.floor(bounds.y);
        widget.props.width = Math.floor(bounds.w);
        widget.props.height = Math.floor(bounds.h);
        // document.getElementById(widget.props.name).style.width = widget.props.width;
        // document.getElementById(widget.props.name).style.height = widget.props.height;
        if (widget.props.type != 'form') {
          document.getElementById(widget.props.name).innerHTML = widget.getSVG();
        }
      }

      if (widget.props.hasOwnProperty('channel'))
        widget.props.channel = name;

      new PropertyPanel(widget.props.type, widget.props);
      vscode.postMessage({
        command: 'widgetUpdate',
        text: JSON.stringify(widget.props)
      })
    }
  });
}
/**
 * PropertyPanel Class. Lightweight component that up updated its innerHTML when properties change.
 * Gets passed the widget type, and a JSON object containing all the widget properties 
 */
class PropertyPanel {
  constructor(type, properties) {
    this.type = type;
    var panel = document.querySelector('.property-panel');

    Object.entries(properties).forEach((entry) => {
      const [key, value] = entry;
      var propertyDiv = document.createElement('div');
      propertyDiv.classList.add('property');

      var label = document.createElement('label');
      let text = `${key}`

      let result = text.replace(/([A-Z])/g, " $1");
      const separatedName = result.charAt(0).toUpperCase() + result.slice(1);
      label.textContent = separatedName;
      propertyDiv.appendChild(label);

      var input = document.createElement('input');
      input.id = text;
      input.dataset.parent = properties.name;

      if (text.toLowerCase().indexOf("colour") != -1) {
        input.type = 'color';
        function rgbToHex(rgbText) {
          return rgbText.replace(/rgb\((.+?)\)/ig, (_, rgb) => {
            return '#' + rgb.split(',')
              .map(str => parseInt(str, 10).toString(16).padStart(2, '0'))
              .join('')
          })
        }
        input.value = value;
      }
      else {
        input.type = 'text';
        input.value = `${value}`;
      }

      input.addEventListener('input', function (evt) {
        if (evt.target.type === 'color') {
          widgets.forEach((widget) => {
            if (widget.props.name == evt.target.dataset.parent) {
              widget.props[evt.target.id] = evt.target.value;
              const widgetDiv = document.getElementById(widget.props.name);
              widgetDiv.innerHTML = widget.getSVG();
              vscode.postMessage({
                command: 'widgetUpdate',
                text: JSON.stringify(widget.props)
              })
            }
          })
        }
        else if (evt.target.type === 'text') {
          widgets.forEach((widget) => {
            if (widget.props.name == evt.target.dataset.parent) {
              widget.props[evt.target.id] = evt.target.value;
              vscode.postMessage({
                command: 'widgetUpdate',
                text: JSON.stringify(widget.props)
              })
            }
          })
        }

      }, this);

      propertyDiv.appendChild(input);

      if (panel)
        panel.appendChild(propertyDiv);
    });
  }
};


/**
 * Add listener for context menu. Also keeps the current x and x positions 
 * in case a user adds a widget
 */
if (typeof acquireVsCodeApi === 'function') {
  let mouseDownPosition = {};
  form.addEventListener("contextmenu", e => {
    e.preventDefault();
    let x = e.offsetX, y = e.offsetY,
      winWidth = window.innerWidth,
      winHeight = window.innerHeight,
      cmWidth = contextMenu.offsetWidth,
      cmHeight = contextMenu.offsetHeight;

    x = x > winWidth - cmWidth ? winWidth - cmWidth - 5 : x;
    y = y > winHeight - cmHeight ? winHeight - cmHeight - 5 : y;

    contextMenu.style.left = `${x}px`;
    contextMenu.style.top = `${y}px`;
    mouseDownPosition = { x: x, y: y };
    contextMenu.style.visibility = "visible";
  });
  document.addEventListener("click", () => contextMenu.style.visibility = "hidden");

  new PropertyPanel('slider', currentWidget);

  /**
   * Add a click callback listener for each item in the menu. Within the click callback
   * a new widget is added to the form, and a new widget object is pushed to the widgets array. 
   * Assigning class type 'editMode' gives it draggable and resizable functionality. 
   */
  let menuItems = document.getElementsByTagName('*');
  for (var i = 0; i < menuItems.length; i++) {
    if (menuItems[i].getAttribute('class') == 'menuItem') {
      menuItems[i].addEventListener("click", async (e) => {
        const type = e.target.innerHTML.replace(/(<([^>]+)>)/ig);
        const channel = type + String(numberOfWidgets);
        numberOfWidgets++;
        const w = await insertWidget(type, { channel: channel, top: mouseDownPosition.y, left: mouseDownPosition.x });
        if (widgets) {
          //update text editor with last added widget
          vscode.postMessage({
            command: 'widgetUpdate',
            text: JSON.stringify(w)
          })
        }

      });
    }
  }
}
/**
 * insets a new widget to the form, this can be called when loading/saving a file, or when we right-
 * click and add widgets
 */
async function insertWidget(type, props) {

  const widgetDiv = document.createElement('div');
  widgetDiv.className = 'editMode';
  widgetDiv.className = cabbageMode;


  let widget = {};

  switch (type) {
    case "rslider":
      widget = new RotarySlider();
      break;
    case "form":
      widget = new Form();
      break;
    default:
      return;
  }


  Object.entries(props).forEach((entry) => {
    const [key, value] = entry;

    widget.props[key] = value;
    if (key === 'channel') {
      widget.props.name = value;
      widget.props.channel = value;
    }
  })




  widgets.push(widget); // Push the new widget object into the array
  const index = getNumberOfPluginParameters("rslider", "hslider", "button", "checkbox");
  widget.props.index = index - 1;
  console.log(JSON.stringify(widget.props, null, 2));

  if (cabbageMode === 'playMode') {
    if (typeof acquireVsCodeApi === 'function') {
      if (!vscode)
        vscode = acquireVsCodeApi();

      widget.addEventListeners(widgetDiv, vscode);
    }

  }
  else {
    widget.addEventListeners(widgetDiv);
  }


  widgetDiv.id = widget.props.name;
  widgetDiv.innerHTML = widget.getSVG();
  if (form) {
    form.appendChild(widgetDiv);
  }
  widgetDiv.style.transform = 'translate(' + widget.props.left + 'px,' + widget.props.top + 'px)';
  widgetDiv.setAttribute('data-x', widget.props.left);
  widgetDiv.setAttribute('data-y', widget.props.top);
  widgetDiv.style.width = widget.props.width + 'px'
  widgetDiv.style.height = widget.props.height + 'px'


  return widget.props;
}




