import { CabbageUtils } from "./utils.js";

/**
 * Vertical Slider (vslider) class
 */
export class VerticalSlider {
  constructor() {
    this.props = {
      "top": 10,
      "left": 10,
      "width": 20,
      "height": 160,
      "channel": "vslider",
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
      "colour": "#0295cf",
      "trackerColour": "#93D200",
      "trackerBackgroundColour": "#ffffff",
      "trackerOutlineColour": "#525252",
      "fontColour": "#222222",
      "textColour": "#222222",
      "outlineColour": "#999999",
      "textBoxOutlineColour": "#999999",
      "textBoxColour": "#555555",
      "trackerOutlineWidth": 2,
      "outlineWidth": 0.3,
      "markerThickness": 0.2,
      "markerStart": 0.1,
      "markerEnd": 0.9,
      "name": "hslider",
      "type": "vslider",
      "kind": "vertical",
      "decimalPlaces": 1,
      "velocity": 0,
      "trackerStart": 0.1,
      "trackerEnd": 0.9,
      "visible": 1,
      "automatable": 1,
      "valuePrefix": "",
      "valuePostfix": ""
    }

    this.panelSections = {
      "Properties": ["type", "channel"],
      "Bounds": ["left", "top", "width", "height"],
      "Range": ["min", "max", "defaultValue", "skew", "increment"],
      "Text": ["text", "fontSize", "fontFamily", "fontColour", "textOffsetY", "align"],
      "Colours": ["colour", "trackerBackgroundColour", "trackerStrokeColour", "outlineColour", "textBoxOutlineColour", "textBoxColour"]
    };

    this.moveListener = this.pointerMove.bind(this);
    this.upListener = this.pointerUp.bind(this);
    this.startY = 0;
    this.startValue = 0;
    this.vscode = null;
    this.isMouseDown = false;
    this.decimalPlaces = 0;
  }

  pointerUp() {
    const popup = document.getElementById('popupValue');
    popup.classList.add('hide');
    popup.classList.remove('show');
    window.removeEventListener("pointermove", this.moveListener);
    window.removeEventListener("pointerup", this.upListener);
    this.isMouseDown = false;
  }

  pointerDown(evt) {
    this.isMouseDown = true;
    this.startY = evt.offsetY;
    this.props.value = CabbageUtils.map(this.startY, this.props.height, 0, this.props.min, this.props.max)

    window.addEventListener("pointermove", this.moveListener);
    window.addEventListener("pointerup", this.upListener);

    this.props.value = Math.round(this.props.value / this.props.increment) * this.props.increment;
    this.startValue = this.props.value;
    const widgetDiv = document.getElementById(this.props.name);
    console.log(this.props.name);
    widgetDiv.innerHTML = this.getSVG();
  }

  mouseEnter(evt) {
    const popup = document.getElementById('popupValue');
    const form = document.getElementById('MainForm');
    const rect = form.getBoundingClientRect();
    this.decimalPlaces = CabbageUtils.getDecimalPlaces(this.props.increment);

    if (popup) {
      popup.textContent = parseFloat(this.props.value).toFixed(this.decimalPlaces);

      // Calculate the position for the popup
      const sliderLeft = this.props.left;
      const sliderWidth = this.props.width;
      const formLeft = rect.left;
      const formWidth = rect.width;

      // Determine if the popup should be on the right or left side of the slider
      const sliderCenter = formLeft + (formWidth / 2);
      let popupLeft;
      if (sliderLeft + (sliderWidth / 2) > sliderCenter) {
        // Place popup on the left of the slider thumb
        popupLeft = sliderLeft;
        console.log("Pointer on the left");
        popup.classList.add('right');
      } else {
        // Place popup on the right of the slider thumb
        popupLeft = sliderLeft + sliderWidth + 60;
        console.log("Pointer on the right");
        popup.classList.remove('right');
      }

      const popupTop = this.props.top + this.props.height;

      // Set the calculated position
      popup.style.left = `${popupLeft}px`;
      popup.style.top = `${popupTop}px`;
      popup.style.display = 'block';
      popup.classList.add('show');
      popup.classList.remove('hide');
    }
  }

  mouseLeave(evt) {
    if (!this.isMouseDown) {
      const popup = document.getElementById('popupValue');
      popup.classList.add('hide');
      popup.classList.remove('show');
    }

  }

  addEventListeners(widgetDiv, vs) {
    this.vscode = vs;
    widgetDiv.addEventListener("pointerdown", this.pointerDown.bind(this));
    widgetDiv.addEventListener("mouseenter", this.mouseEnter.bind(this));
    widgetDiv.addEventListener("mouseleave", this.mouseLeave.bind(this));
  }

  addEventListeners(widgetDiv) {
    widgetDiv.addEventListener("pointerdown", this.pointerDown.bind(this));
    widgetDiv.addEventListener("mouseenter", this.mouseEnter.bind(this));
    widgetDiv.addEventListener("mouseleave", this.mouseLeave.bind(this));
  }

pointerMove(evt) {
    // Get the bounding rectangle of the slider
    const sliderRect = document.getElementById(this.props.name).getBoundingClientRect();

    // Calculate the relative position of the mouse pointer within the slider bounds
    let offsetY = evt.clientY - sliderRect.top;

    // Clamp the mouse position to stay within the bounds of the slider
    offsetY = CabbageUtils.clamp(offsetY, 0, sliderRect.height);

    // Calculate the new value based on the mouse position, invert the direction
    let newValue = CabbageUtils.map(offsetY, sliderRect.height, 0, this.props.min, this.props.max); // Note the inverted direction here
    newValue = Math.round(newValue / this.props.increment) * this.props.increment; // Round to the nearest increment

    // Update the slider value
    this.props.value = newValue;

    // Update the slider appearance
    const widgetDiv = document.getElementById(this.props.name);
    widgetDiv.innerHTML = this.getSVG();

    // Post message if vscode is available
    const msg = { channel: this.props.channel, value: CabbageUtils.map(this.props.value, this.props.min, this.props.max, 0, 1) }
    if (this.vscode) {
      this.vscode.postMessage({
        command: 'channelUpdate',
        text: JSON.stringify(msg)
      })
    } else {
      var message = {
        "msg": "parameterUpdate",
        "paramIdx": this.props.index,
        "value": CabbageUtils.map(this.props.value, this.props.min, this.props.max, 0, 1)
      };
      // IPlugSendMsg(message);
    }
}


  getSVG() {
    const popup = document.getElementById('popupValue');
    if (popup) {
      popup.textContent = parseFloat(this.props.value).toFixed(this.decimalPlaces);
    }

    const yPos = CabbageUtils.map(this.props.value, this.props.max, this.props.min, 0, this.props.height*.95);

    return `
    <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${this.props.width+2} ${this.props.height+2}" width="100%" height="100%" preserveAspectRatio="none">
    <svg width="${this.props.width}" height="${this.props.height}" fill="none" xmlns="http://www.w3.org/2000/svg">
    <rect x="${this.props.width*.2}" y="0" width="${this.props.width*.6}" height="${this.props.height}" rx="4" fill="${this.props.trackerBackgroundColour}" stroke-width="${this.props.trackerOutlineWidth}" stroke="${this.props.trackerOutlineColour}"/>
    <rect x="${this.props.width*.2}" y="${yPos}" width="${this.props.width*.6}" height="${this.props.height  - yPos}" rx="4" fill="${this.props.trackerColour}" stroke-width="${this.props.trackerOutlineWidth}" stroke="${this.props.trackerOutlineColour}"/>
    <rect x="0" y="${yPos}" width="${this.props.width}" height="${this.props.height*.05}" rx="4" fill="${this.props.colour}" stroke-width="2" stroke="black"/>
    </svg>

    `;
  }


}



