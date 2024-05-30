

import { Form, RotarySlider } from "./widgets.js";

let vscode = null;
let widgetWrappers = null;
let selectedElements = new Set();

if (typeof acquireVsCodeApi === 'function') {
  vscode = acquireVsCodeApi();
  try {
    const module = await import("./widgetWrapper.js");
    const { WidgetWrapper } = module;
    // You can now use WidgetWrapper here

    widgetWrappers = new WidgetWrapper(updatePanel, selectedElements);
  } catch (error) {
    console.error("Error loading widgetWrapper.js:", error);
  }
}


const widgets = [];

let cabbageMode = 'draggable';
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
      const [min, max, initValue, skew, increment] = value.split(',').map(v => parseFloat(v.trim()));
      jsonObj['min'] = min;
      jsonObj['max'] = max;
      jsonObj['value'] = initValue;
      jsonObj['skew'] = skew;
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
      cabbageMode = 'nonDraggable';
      form.className = "form";
      parseCabbageCsdTile(message.text);
      break;
    case 'widgetUpdate':
      const msg = JSON.parse(message.text);
      updateWidget(msg);
      break;
    case 'onEnterEditMode':
      cabbageMode = 'draggable';
      form.className = "form draggable";
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
 * this callback is triggered whenever a user move/drags a widget in edit mode
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

      new PropertyPanel(widget.props.type, widget.props, widget.panelSections);
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
  constructor(type, properties, panelSections) {
    this.type = type;
    this.panelSections = panelSections;
    var panel = document.querySelector('.property-panel');

    // Helper function to create a section
    const createSection = (sectionName) => {
      const sectionDiv = document.createElement('div');
      sectionDiv.classList.add('property-section');

      const header = document.createElement('h3');
      header.textContent = sectionName;
      sectionDiv.appendChild(header);

      return sectionDiv;
    };

    // Create sections based on the panelSections object
    const sections = {};

    if (panelSections === undefined)
      return;

    Object.entries(panelSections).forEach(([sectionName, keys]) => {
      sections[sectionName] = createSection(sectionName);
    });

    // Create a section for Miscellaneous properties
    const miscSection = createSection('Misc');
    sections['Misc'] = miscSection;

    // List of popular online fonts
    const fontList = [
      'Arial', 'Verdana', 'Helvetica', 'Tahoma', 'Trebuchet MS',
      'Times New Roman', 'Georgia', 'Garamond', 'Courier New',
      'Brush Script MT', 'Comic Sans MS', 'Impact', 'Lucida Sans',
      'Palatino', 'Century Gothic', 'Bookman', 'Candara', 'Consolas'
    ];

    // Helper function to create an input element based on the property key
    const createInputElement = (key, value) => {
      let input;

      if (key.toLowerCase().includes("colour")) {
        input = document.createElement('input');
        input.type = 'color';
        input.value = value;
      } else if (key.toLowerCase().includes("family")) {
        input = document.createElement('select');
        fontList.forEach((font) => {
          const option = document.createElement('option');
          option.value = font;
          option.textContent = font;
          if (font === 'Verdana') {
            option.selected = true;  // Set Verdana as default
          }
          input.appendChild(option);
        });
        input.value = value || 'Verdana';
      } else if (key.toLowerCase() === 'align') {
        input = document.createElement('select');
        const alignments = ['left', 'right', 'centre'];
        alignments.forEach((align) => {
          const option = document.createElement('option');
          option.value = align;
          option.textContent = align;
          if (align === 'centre') {
            option.selected = true;  // Set centre as default
          }
          input.appendChild(option);
        });
        input.value = value || 'centre';
      } else {
        input = document.createElement('input');
        input.type = 'text';
        input.value = `${value}`;

        // Set the input to readonly if the key is "type"
        if (key.toLowerCase() === 'type') {
          input.readOnly = true;
        }
      }

      return input;
    };

    // Iterate over panelSections and properties to assign them to their respective sections in order
    Object.entries(panelSections).forEach(([sectionName, keys]) => {
      keys.forEach((key) => {
        if (properties.hasOwnProperty(key)) {
          var propertyDiv = document.createElement('div');
          propertyDiv.classList.add('property');

          var label = document.createElement('label');
          let text = `${key}`;

          let result = text.replace(/([A-Z])/g, " $1");
          const separatedName = result.charAt(0).toUpperCase() + result.slice(1);
          label.textContent = separatedName;
          propertyDiv.appendChild(label);

          var input = createInputElement(key, properties[key]);

          input.id = key;
          input.dataset.parent = properties.name;

          input.addEventListener('input', function (evt) {
            widgets.forEach((widget) => {
              if (widget.props.name === evt.target.dataset.parent) {
                const inputValue = evt.target.value;
                let parsedValue;

                // Check if the input value can be parsed to a number
                if (!isNaN(inputValue) && inputValue.trim() !== "") {
                  parsedValue = Number(inputValue);
                } else {
                  parsedValue = inputValue;
                }
                widget.props[evt.target.id] = parsedValue;
                const widgetDiv = document.getElementById(widget.props.name);
                widgetDiv.innerHTML = widget.getSVG();
                vscode.postMessage({
                  command: 'widgetUpdate',
                  text: JSON.stringify(widget.props)
                });
              }
            });
          });

          propertyDiv.appendChild(input);
          sections[sectionName].appendChild(propertyDiv);
        }
      });
    });

    // Append sections to the panel in the specified order
    Object.keys(panelSections).forEach((sectionName) => {
      if (sections[sectionName].childNodes.length > 1) {
        panel.appendChild(sections[sectionName]);
      }
    });

    // Append the Misc section last
    if (sections['Misc'].childNodes.length > 1) {
      panel.appendChild(sections['Misc']);
    }
  }
}



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

  // new PropertyPanel('slider', currentWidget, {});

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
        const w = await insertWidget(type, { channel: channel, top: mouseDownPosition.y - 20, left: mouseDownPosition.x - 20 });
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

/*
 * Various listeners for the main form to handle grouping ans moving of multiple elements
 */
if (form) {
  
  let isSelecting = false;
  let selectionBox;
  let startX, startY;
  
  form.addEventListener('pointerdown', (event) => {
    if ((event.shiftKey || event.altKey) && event.target === form) {
      // Start selection mode
      isSelecting = true;
  
      startX = event.clientX;
      startY = event.clientY;
  
      selectionBox = document.createElement('div');
      selectionBox.style.position = 'absolute';
      selectionBox.style.border = '1px dashed #000';
      selectionBox.style.backgroundColor = 'rgba(20, 20, 20, 0.3)';
      selectionBox.style.left = `${startX}px`;
      selectionBox.style.top = `${startY}px`;
  
      form.appendChild(selectionBox);
    } else if (event.target.classList.contains('draggable')) {
      if (!event.shiftKey && !event.altKey) {
        // Deselect all elements if clicking on a non-selected element without Shift or Alt key
        if (!selectedElements.has(event.target)) {
          selectedElements.forEach(element => element.classList.remove('selected'));
          selectedElements.clear();
          selectedElements.add(event.target);
        }
        event.target.classList.add('selected');
      } else {
        // Toggle selection state if Shift or Alt key is pressed
        event.target.classList.toggle('selected');
        if (event.target.classList.contains('selected')) {
          selectedElements.add(event.target);
        } else {
          selectedElements.delete(event.target);
        }
      }
    } else if (!event.shiftKey && !event.altKey && event.target === form) {
      // Deselect all elements if clicking on the form without Shift or Alt key
      selectedElements.forEach(element => element.classList.remove('selected'));
      selectedElements.clear();
    }
  });
  
  form.addEventListener('pointermove', (event) => {
    if (isSelecting) {
      const currentX = event.clientX;
      const currentY = event.clientY;
  
      selectionBox.style.width = `${Math.abs(currentX - startX)}px`;
      selectionBox.style.height = `${Math.abs(currentY - startY)}px`;
      selectionBox.style.left = `${Math.min(currentX, startX)}px`;
      selectionBox.style.top = `${Math.min(currentY, startY)}px`;
    }
  });
  
  form.addEventListener('pointerup', (event) => {
    if (isSelecting) {
      const rect = selectionBox.getBoundingClientRect();
  
      const elements = form.querySelectorAll('.draggable');
  
      elements.forEach((element) => {
        const elementRect = element.getBoundingClientRect();
        const elementLeft = elementRect.left;
        const elementRight = elementRect.right;
        const elementTop = elementRect.top;
        const elementBottom = elementRect.bottom;
  
        if (
          elementLeft >= rect.left &&
          elementRight <= rect.right &&
          elementTop >= rect.top &&
          elementBottom <= rect.bottom
        ) {
          element.classList.add('selected');
          selectedElements.add(element);
        }
      });
  
      form.removeChild(selectionBox);
      isSelecting = false;
    }
  });
}

/**
 * insets a new widget to the form, this can be called when loading/saving a file, or when we right-
 * click and add widgets
 */
async function insertWidget(type, props) {

  const widgetDiv = document.createElement('div');
  widgetDiv.className = cabbageMode;
  if (cabbageMode === 'draggable') {
    widgetDiv.addEventListener('pointerdown', (e) => {
      if (e.altKey || e.shiftKey) {  // Use Alt key for multi-selection
        widgetDiv.classList.toggle('selected');
        if (widgetDiv.classList.contains('selected')) {
          selectedElements.add(widgetDiv);
        } else {
          selectedElements.delete(widgetDiv);
        }
      } else {
        if (!widgetDiv.classList.contains('selected')) {
          selectedElements.forEach(element => element.classList.remove('selected'));
          selectedElements.clear();
          widgetDiv.classList.add('selected');
          selectedElements.add(widgetDiv);
      }
      }
      console.log(selectedElements.size);
    });
  }

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


  if (cabbageMode === 'nonDraggable') {
    if (typeof acquireVsCodeApi === 'function') {
      if (!vscode)
        vscode = acquireVsCodeApi();

      console.log('adding listeners');
      widget.addEventListeners(widgetDiv, vscode);
    }

  }
  // else {
  //   widget.addEventListeners(widgetDiv);
  // }


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




