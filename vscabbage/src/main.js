
import { Form } from "./widgets/form.js";
import { RotarySlider } from "./widgets/rotarySlider.js";
import { HorizontalSlider } from "./widgets/horizontalSlider.js";
import { VerticalSlider } from "./widgets/verticalSlider.js";
import { Button } from "./widgets/button.js";
import { Checkbox } from "./widgets/checkbox.js";
import { ComboBox } from "./widgets/comboBox.js";
import { Label } from "./widgets/label.js";
import { CsoundOutput } from "./widgets/csoundOutput.js";
import { MidiKeyboard } from "./widgets/midiKeyboard.js";
import { PropertyPanel } from "./propertyPanel.js";
import { CabbageUtils, CabbageTestUtilities } from "./utils.js";

const widgetsForTesting= [new RotarySlider(), new ComboBox(), new Button(), new Checkbox(), new Label(), 
   new HorizontalSlider(), new VerticalSlider(), new MidiKeyboard()];
CabbageTestUtilities.generateIdentifierTestCsd(widgetsForTesting); // This will generate a test CSD file with the widgets
//CabbageTestUtilities.generateCabbageWidgetDescriptorsClass(widgetsForTesting); // This will generate a class with the widget descriptors







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
    console.log("loading interface");
    widgetWrappers = new WidgetWrapper(PropertyPanel.updatePanel, selectedElements, widgets, vscode);
    vscode.postMessage({ command: 'ready' });
  } catch (error) {
    console.error("Error loading widgetWrapper.js:", error);
  }
}





let cabbageMode = 'draggable';
//adding this messes up dragging of main form
widgets.push(new Form());

const form = document.getElementById('MainForm');
form.style.backgroundColor = widgets[0].props.colour;

CabbageUtils.showOverlay();


/**
 * called from the webview panel on startup, and when a user saves/updates or changes .csd file
 */
window.addEventListener('message', event => {
  
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
      CabbageUtils.parseCabbageCode(message.text, widgets, form, insertWidget);
      break;
    case 'snapToSize':
      widgetWrappers.setSnapSize(parseInt(message.text));
      break;
    case 'widgetUpdate':
      const msg = JSON.parse(message.text);
      updateWidget(msg);
      break;
    case 'onEnterEditMode':
      CabbageUtils.hideOverlay();
      console.log("onEnterEditMode");
      cabbageMode = 'draggable';
      //form.className = "form draggable";
      CabbageUtils.parseCabbageCode(message.text, widgets, form, insertWidget);
      // break;
    case 'csoundOutputUpdate':
    // Find csoundOutput widget
    console.log("csoundOutputUpdate")
      let csoundOutput = widgets.find(widget => widget.props.channel === 'csoundoutput');
      if (csoundOutput) {
        // Update the HTML content of the widget's div
        const csoundOutputDiv = CabbageUtils.getWidgetDiv(csoundOutput.props.channel);
        if (csoundOutputDiv) {

          csoundOutputDiv.innerHTML = csoundOutput.getInnerHTML();
          csoundOutput.appendText(message.text)

        }
      }
      else
        console.log("No csoundoutput widget found");
    default:
      return;
  }
});


/*
* this is called from the plugin and will update either the value of 
* a widget, or some of its properties
*/

function updateWidget(obj) {
  const channel = obj['channel'];
  for (const widget of widgets) {
    if (widget.props.channel == channel) {
      if (obj.hasOwnProperty('value')) {
        widget.props.value = obj['value'];
      }
      else if (obj.hasOwnProperty('data')) {
        Object.entries(CabbageUtils.getCabbageCodeAsJSON(obj['data'])).forEach((entry) => {
          const [key, value] = entry;
          widget.props[key] = value;
        });
      }
      document.getElementById(widget.props.channel).style.transform = 'translate(' + widget.props.left + 'px,' + widget.props.top + 'px)';
      document.getElementById(widget.props.channel).setAttribute('data-x', widget.props.left);
      document.getElementById(widget.props.channel).setAttribute('data-y', widget.props.top);
      document.getElementById(widget.props.channel).style.top = widget.props.top + 'px';
      document.getElementById(widget.props.channel).style.left = widget.props.left + 'px';
      document.getElementById(widget.props.channel).innerHTML = widget.getInnerHTML();
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
    if (cabbageMode === 'draggable')
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

        const channel = CabbageUtils.getUniqueChannelName(type, widgets);
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
  let isDragging = false;
  let selectionBox;
  let startX, startY;
  let offsetX = 0;
  let offsetY = 0;

  form.addEventListener('pointerdown', (event) => {
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

    if (cabbageMode === 'draggable')
      PropertyPanel.updatePanel(vscode, { eventType: "click", name: CabbageUtils.findValidId(event), bounds: {} }, widgets);
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
/**
 * insets a new widget to the form, this can be called when loading/saving a file, or when we right-
 * click and add widgets
 */
async function insertWidget(type, props) {
  const widgetDiv = document.createElement('div');
  let widget = {};

  switch (type) {
    case "rslider":
      widget = new RotarySlider();
      break;
    case "hslider":
      widget = new HorizontalSlider();
      break;
    case "vslider":
      widget = new VerticalSlider();
      break;
    case "keyboard":
      widget = new MidiKeyboard();
      break;
    case "form":
      widget = new Form();
      break;
    case "button":
      widget = new Button();
      break;
    case "label":
      widget = new Label();
      break;
    case "combobox":
      widget = new ComboBox();
      break;
    case "checkbox":
      widget = new Checkbox();
      break;
    case "csoundoutput":
      widget = new CsoundOutput();
      break;
    default:
      return;
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
  const index = CabbageUtils.getNumberOfPluginParameters(widgets, "rslider", "hslider", "vslider", "button", "combobox", "checkbox");
  widget.props.index = index - 1;


  if (cabbageMode === 'nonDraggable') {
    if (typeof acquireVsCodeApi === 'function') {
      if (!vscode){
        vscode = acquireVsCodeApi();
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
  widgetDiv.style.transform = 'translate(' + widget.props.left + 'px,' + widget.props.top + 'px)';
  widgetDiv.setAttribute('data-x', widget.props.left);
  widgetDiv.setAttribute('data-y', widget.props.top);
  widgetDiv.style.width = widget.props.width + 'px'
  widgetDiv.style.height = widget.props.height + 'px'


  return widget.props;
}




