//widgets ---------------
import { Form } from "./widgets/form.js";
import { RotarySlider } from "./widgets/rotarySlider.js";
import { HorizontalSlider } from "./widgets/horizontalSlider.js";
import { VerticalSlider } from "./widgets/verticalSlider.js";
import { NumberSlider } from "./widgets/numberSlider.js";
import { Button, FileButton, OptionButton } from "./widgets/button.js";
import { Checkbox } from "./widgets/checkbox.js";
import { ComboBox } from "./widgets/comboBox.js";
import { Label } from "./widgets/label.js";
import { GenTable } from "./widgets/genTable.js";
import { CsoundOutput } from "./widgets/csoundOutput.js";
import { MidiKeyboard } from "./widgets/midiKeyboard.js";
import { TextEditor } from "./widgets/textEditor.js";
//------------------------

const widgetConstructors = {
  "rslider": RotarySlider,
  "hslider": HorizontalSlider,
  "vslider": VerticalSlider,
  "nslider": NumberSlider,
  "keyboard": MidiKeyboard,
  "form": Form,
  "button": Button,
  "filebutton": FileButton,
  "optionbutton": OptionButton,
  "gentable": GenTable,
  "label": Label,
  "combobox": ComboBox,
  "checkbox": Checkbox,
  "csoundoutput": CsoundOutput,
  "texteditor": TextEditor
};

import { PropertyPanel } from "./propertyPanel.js";
import { CabbageUtils, CabbageTestUtilities } from "./utils.js";
import { Cabbage } from "./cabbage.js";

// Uncomment to generate various source code files for testing
const widgetsForTesting = [
  new Button(),
  new Checkbox(),
  new ComboBox(),
  new CsoundOutput(),
  new FileButton(),
  new Form(),
  new GenTable(),
  new HorizontalSlider(),
  new Label(),
  new MidiKeyboard(),
  new NumberSlider(),
  new RotarySlider(),
  new TextEditor(),
  new VerticalSlider(),
  new OptionButton()
];
// CabbageTestUtilities.generateIdentifierTestCsd(widgetsForTesting); // This will generate a test CSD file with the widgets
CabbageTestUtilities.generateCabbageWidgetDescriptorsClass(widgetsForTesting); // This will generate a class with the widget descriptors 


//sending a message to notify when main has been loaded - Cabbage listens for this message 
//before trying to load an interface.
console.log("main.js loaded!")

let vscode = null;
let widgetWrappers = null;
let selectedElements = new Set();
const widgets = [];

if (typeof acquireVsCodeApi === 'function') {
  vscode = acquireVsCodeApi();
  try {
    const module = await import("./widgetWrapper.js");
    const { WidgetWrapper } = module;
    // You can now use WidgetWrapper here
    widgetWrappers = new WidgetWrapper(PropertyPanel.updatePanel, selectedElements, widgets, vscode);
  } catch (error) {
    console.error("Error loading widgetWrapper.js:", error);
  }
}

Cabbage.sendCustomCommand(vscode, 'cabbageIsReadyToLoad');




let cabbageMode = 'draggable';
//adding this messes up dragging of main form
widgets.push(new Form());

const form = document.getElementById('MainForm');
form.style.backgroundColor = widgets[0].props.colour;

CabbageUtils.showOverlay();


/**
 * called from the webview panel on startup, and when a user saves/updates or changes .csd file
 */
window.addEventListener('message', async event => {

  const message = event.data;
  switch (message.command) {

    case 'onFileChanged':
      CabbageUtils.hideOverlay();
      cabbageMode = 'nonDraggable';
      form.className = "form nonDraggable";
      const leftPanel = document.getElementById('LeftPanel');
      if (leftPanel)
        leftPanel.className = "full-height-div nonDraggable"

      const rightPanel = document.getElementById('RightPanel');
      if (rightPanel)
        rightPanel.style.visibility = "hidden";

      try {
        await CabbageUtils.parseCabbageCode(message.text, widgets, form, insertWidget);
        // Additional code to execute if parseCabbageCode succeeds
      } catch (error) {
        console.error("An error occurred while parsing the cabbage code:", error);
        // Handle the error appropriately, such as displaying a message to the user
      }

      //in plugin mode we need to sync with the instrument's widget array
      widgets.forEach(w => {
        Cabbage.sendWidgetUpdate(vscode, w);
      });

      Cabbage.sendCustomCommand(vscode, 'cabbageSetupComplete');
      break;

    case 'snapToSize':
      widgetWrappers.setSnapSize(parseInt(message.text));
      break;

    case 'widgetUpdate':
      const updateMsg = JSON.parse(message.text);
      updateWidget(updateMsg);
      break;

    case 'onEnterEditMode':
      CabbageUtils.hideOverlay();
      cabbageMode = 'draggable';
      //form.className = "form draggable";
      CabbageUtils.parseCabbageCode(message.text, widgets, form, insertWidget);
      break;

    case 'csoundOutputUpdate':
      // Find csoundOutput widget
      let csoundOutput = widgets.find(widget => widget.props.channel === 'csoundoutput');
      if (csoundOutput) {
        // Update the HTML content of the widget's div
        const csoundOutputDiv = CabbageUtils.getWidgetDiv(csoundOutput.props.channel);
        if (csoundOutputDiv) {
          csoundOutputDiv.innerHTML = csoundOutput.getInnerHTML();
          csoundOutput.appendText(message.text);
        }
      }
      break;

    default:
      return;
  }
});

/*
* this is called from the plugin and will update a corresponding widget
*/
function updateWidget(obj) {
  const channel = obj['channel'];
  for (const widget of widgets) {
    if (widget.props.channel === channel) {
      if (obj.hasOwnProperty('data')) {
        widget.props = JSON.parse(obj["data"]);
      } else if (obj.hasOwnProperty('value')) {
        widget.props.value = obj['value'];
      }
   
      const widgetElement = CabbageUtils.getWidgetDiv(widget.props.channel);
      if (widgetElement) {
        // console.log(widgetElement.id, widgetElement.parentElement.id);
        widgetElement.style.transform = 'translate(' + widget.props.left + 'px,' + widget.props.top + 'px)';
        widgetElement.setAttribute('data-x', widget.props.left);
        widgetElement.setAttribute('data-y', widget.props.top);
        // widgetElement.style.top = widget.props.top + 'px';
        // widgetElement.style.left = widget.props.left + 'px';

        // Do not update the innerHTML of a form as it will remove all its children
        if (widget.props.type !== "form") {
          widgetElement.innerHTML = widget.getInnerHTML();
        }
      }

      // gentable and form are special cases and have dedicated update methods
      if (widget.props.type == "gentable") {
        widget.updateTable();

      } else if (widget.props.type == "form") {
        widget.updateSVG();
      }
    }
  }
}


const contextMenu = document.querySelector(".wrapper");

/**
 * Add listener for context menu. Also keeps the current x and x positions 
 * in case a user adds a widget
 */
if (typeof acquireVsCodeApi === 'function') {
  let mouseDownPosition = {};
  form.addEventListener("contextmenu", e => {
    console.log("context menu");
    e.preventDefault();
    e.stopImmediatePropagation();
    e.stopPropagation();
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
    if (cabbageMode === 'draggable') { contextMenu.style.visibility = "visible"; }

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
    if (menuItems[i].getAttribute('class') === 'menuItem') {
      menuItems[i].addEventListener("pointerdown", async (e) => {
        console.log('clicked');
        e.stopImmediatePropagation();
        e.stopPropagation();
        const type = e.target.innerHTML.replace(/(<([^>]+)>)/ig);
        const channel = CabbageUtils.getUniqueChannelName(type, widgets);
        const w = await insertWidget(type, { channel: channel, top: mouseDownPosition.y - 20, left: mouseDownPosition.x - 20 });
        if (widgets) {
          //update text editor with last added widget
          vscode.postMessage({
            command: 'widgetUpdate',
            text: JSON.stringify(w)
          });
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
  let isDragging = false;
  let selectionBox;
  let startX, startY;
  let offsetX = 0;
  let offsetY = 0;

  form.addEventListener('pointerdown', (event) => {
    if (event.button !== 0) { return; }

    const clickedElement = event.target;
    const formRect = form.getBoundingClientRect();
    offsetX = formRect.left;
    offsetY = formRect.top;

    if ((event.shiftKey || event.altKey) && event.target === form) {
      // Start selection mode
      isSelecting = true;
      startX = event.clientX - offsetX;
      startY = event.clientY - offsetY;
      selectionBox = document.createElement('div');
      selectionBox.style.position = 'absolute';
      selectionBox.style.border = '1px dashed #000';
      selectionBox.style.backgroundColor = 'rgba(20, 20, 20, 0.3)';
      selectionBox.style.left = `${startX}px`;
      selectionBox.style.top = `${startY}px`;
      form.appendChild(selectionBox);
    } else if (clickedElement.classList.contains('draggable') && event.target.id !== "MainForm") {

      if (!event.shiftKey && !event.altKey) {
        // Deselect all elements if clicking on a non-selected element without Shift or Alt key
        if (!selectedElements.has(clickedElement)) {
          selectedElements.forEach(element => element.classList.remove('selected'));
          selectedElements.clear();
          selectedElements.add(clickedElement);
        }
        clickedElement.classList.add('selected');
      } else {
        // Toggle selection state if Shift or Alt key is pressed
        clickedElement.classList.toggle('selected');
        if (clickedElement.classList.contains('selected')) {
          selectedElements.add(clickedElement);
        } else {
          selectedElements.delete(clickedElement);
        }
      }
    }

    if (event.target === form) {
      // Deselect all elements if clicking on the form without Shift or Alt key
      selectedElements.forEach(element => element.classList.remove('selected'));
      selectedElements.clear();
    }

    if (!event.shiftKey && !event.altKey) {
      if (cabbageMode === 'draggable') {
        PropertyPanel.updatePanel(vscode, { eventType: "click", name: CabbageUtils.findValidId(event), bounds: {} }, widgets);
      }
    }
  });

  document.addEventListener('pointermove', (event) => {
    if (isSelecting) {
      const currentX = event.clientX - offsetX;
      const currentY = event.clientY - offsetY;

      selectionBox.style.width = `${Math.abs(currentX - startX)}px`;
      selectionBox.style.height = `${Math.abs(currentY - startY)}px`;
      selectionBox.style.left = `${Math.min(currentX, startX)}px`;
      selectionBox.style.top = `${Math.min(currentY, startY)}px`;
    }

    if (isDragging && selectionBox) {
      const currentX = event.clientX;
      const currentY = event.clientY;

      const boxWidth = selectionBox.offsetWidth;
      const boxHeight = selectionBox.offsetHeight;

      const parentWidth = form.offsetWidth;
      const parentHeight = form.offsetHeight;

      const maxX = parentWidth - boxWidth;
      const maxY = parentHeight - boxHeight;

      let newLeft = currentX - offsetX;
      let newTop = currentY - offsetY;

      newLeft = Math.max(0, Math.min(maxX, newLeft));
      newTop = Math.max(0, Math.min(maxY, newTop));

      selectionBox.style.left = `${newLeft}px`;
      selectionBox.style.top = `${newTop}px`;
    }
  });

  document.addEventListener('pointerup', (event) => {
    if (isSelecting) {
      const rect = selectionBox.getBoundingClientRect();
      const elements = form.querySelectorAll('.draggable');

      elements.forEach((element) => {
        const elementRect = element.getBoundingClientRect();

        // Check for intersection between the element and the selection box
        if (elementRect.right >= rect.left &&
          elementRect.left <= rect.right &&
          elementRect.bottom >= rect.top &&
          elementRect.top <= rect.bottom) {
          element.classList.add('selected');
          selectedElements.add(element);
        }
      });

      form.removeChild(selectionBox);
      isSelecting = false;
    }

    isDragging = false;
  });
  if (selectionBox) {
    selectionBox.addEventListener('pointerdown', (event) => {
      isDragging = true;
      offsetX = event.clientX - selectionBox.getBoundingClientRect().left;
      offsetY = event.clientY - selectionBox.getBoundingClientRect().top;
      event.stopPropagation();
    });
  }
}

// Function to create widget dynamically based on type
function createWidget(type) {
  const WidgetClass = widgetConstructors[type];
  if (WidgetClass) {
    const widget = new WidgetClass();
    if (type === "gentable") {
      widget.createCanvas(); // Additional logic specific to "gentable"
    }
    return widget;
  } else {
    console.error("Unknown widget type: " + type);
    return null;
  }
}
/**
 * insets a new widget to the form, this can be called when loading/saving a file, or when we right-
 * click and add widgets
 */
async function insertWidget(type, props) {
  const widgetDiv = document.createElement('div');
  let widget = createWidget(type);
  if (widget) {
    console.log("Created widget:", widget);
  } else {
    console.error("Failed to create widget of type:", type);
  }

  if (type === "form")
    widgetDiv.className = "resizeOnly";
  else
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
    });
  }

  //iterate over the incoming props and assign them to the widget object
  Object.entries(props).forEach((entry) => {
    const [key, value] = entry;
    widget.props[key] = value;
  })

  widgets.push(widget); // Push the new widget object into the array
  const index = CabbageUtils.getNumberOfPluginParameters(widgets);//gets any widgets that are automatable - i.e, a parameter a host can see..
  widget.parameterIndex = index - 1;

  if (cabbageMode === 'nonDraggable') {
    if (typeof acquireVsCodeApi === 'function') {
      if (!vscode) {
        vscode = acquireVsCodeApi();
        widget.addVsCodeEventListeners(widgetDiv, vscode);
      }
      else {
        widget.addVsCodeEventListeners(widgetDiv, vscode);
      }
    }
    else
      widget.addEventListeners(widgetDiv);
  }

  widgetDiv.id = widget.props.channel;

  widgetDiv.innerHTML = widget.getInnerHTML();
  if (form) {
    form.appendChild(widgetDiv);
  }

  // gentable and form are special cases and have dedicated update methods
  if (widget.props.type == "gentable") {
    widget.updateTable();

  } else if (widget.props.type == "form") {
    widget.updateSVG();
  }
  else {
    widgetDiv.innerHTML = widget.getInnerHTML();
  }


  //if (typeof acquireVsCodeApi === 'function') {
  // console.error('this is not good - house of cards nonsense with difference in translate between plugin and vscode')
  widgetDiv.style.transform = 'translate(' + widget.props.left + 'px,' + widget.props.top + 'px)';
  //}

  widgetDiv.setAttribute('data-x', widget.props.left);
  widgetDiv.setAttribute('data-y', widget.props.top);
  widgetDiv.style.width = widget.props.width + 'px'
  widgetDiv.style.height = widget.props.height + 'px'

  return widget.props;
}




