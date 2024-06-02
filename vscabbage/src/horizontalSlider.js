import { getDecimalPlaces, map, clamp } from "./utils.js";

/**
 * Horizontal Slider (hslider) class
 */
export class HorizontalSlider {
  constructor() {
    this.props = {
      "top": 10,
      "left": 10,
      "width": 60,
      "height": 60,
      "channel": "rslider",
      "min": 0,
      "max": 1,
      "value": 0,
      "default": 0,
      "skew": 1,
      "increment": 0.001,
      "index": 0,
      "text": "",
      "fontFamily": "Verdana",
      "fontSize": 0,
      "align": "centre",
      "textOffsetY": 0,
      "valueTextBox": 0,
      "colour": "#dddddd",
      "trackerBackgroundColour": "#000000",
      "trackerStrokeColour": "#222222",
      "trackerColour": "#dddddd",
      "fontColour": "#222222",
      "textColour": "#222222",
      "outlineColour": "#999999",
      "textBoxOutlineColour": "#999999",
      "textBoxColour": "#555555",
      "trackerStrokeWidth": 1,
      "outlineWidth": 0.3,
      "markerThickness": 0.2,
      "markerStart": 0.1,
      "markerEnd": 0.9,
      "name": "",
      "type": "hslider",
      "kind": "horizontal",
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
      "Info": ["type", "channel"],
      "Bounds": ["left", "top", "width", "height"],
      "Range": ["min", "max", "default", "skew", "increment"],
      "Text": ["text", "fontSize", "fontFamily", "fontColour", "textOffsetY", "align"],
      "Colours": ["colour", "trackerBackgroundColour", "trackerStrokeColour", "outlineColour", "textBoxOutlineColour", "textBoxColour"]
    };

    this.moveListener = this.pointerMove.bind(this);
    this.upListener = this.pointerUp.bind(this);
    this.startX = 0;
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
    this.startX = evt.offsetX;
    this.props.value = map(this.startX, 0, this.props.width, this.props.min, this.props.max)

    window.addEventListener("pointermove", this.moveListener);
    window.addEventListener("pointerup", this.upListener);

    this.props.value = Math.round(this.props.value / this.props.increment) * this.props.increment;
    this.startValue = this.props.value;
    const widgetDiv = document.getElementById(this.props.name);
    widgetDiv.innerHTML = this.getSVG();
  }

  mouseEnter(evt) {
    const popup = document.getElementById('popupValue');
    const form = document.getElementById('MainForm');
    const rect = form.getBoundingClientRect();
    this.decimalPlaces = getDecimalPlaces(this.props.increment);

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

  pointerMove({ clientX }) {
    // Get the bounding rectangle of the slider
    const sliderRect = document.getElementById(this.props.name).getBoundingClientRect();

    // Calculate the relative position of the mouse pointer within the slider bounds
    let offsetX = clientX - sliderRect.left;

    // Clamp the mouse position to stay within the bounds of the slider
    offsetX = clamp(offsetX, 0, sliderRect.width);

    // Calculate the new value based on the mouse position
    let newValue = map(offsetX, 0, sliderRect.width, this.props.min, this.props.max);
    newValue = Math.round(newValue / this.props.increment) * this.props.increment; // Round to the nearest increment

    // Update the slider value
    this.props.value = newValue;

    // Update the slider appearance
    const widgetDiv = document.getElementById(this.props.name);
    widgetDiv.innerHTML = this.getSVG();

    // Post message if vscode is available
    const msg = { channel: this.props.channel, value: map(this.props.value, this.props.min, this.props.max, 0, 1) }
    if (this.vscode) {
      this.vscode.postMessage({
        command: 'channelUpdate',
        text: JSON.stringify(msg)
      })
    } else {
      var message = {
        "msg": "parameterUpdate",
        "paramIdx": this.props.index,
        "value": map(this.props.value, this.props.min, this.props.max, 0, 1)
      };
      // IPlugSendMsg(message);
    }
  }

  getSVG() {
    const popup = document.getElementById('popupValue');
    if (popup) {
      popup.textContent = parseFloat(this.props.value).toFixed(this.decimalPlaces);
    }

    // const w = (this.props.width > this.props.height ? this.props.height : this.props.width) * 0.75;
    // const trackerPath = this.describeArc(this.props.width / 2, this.props.height / 2, (w / 2) * (1 - (this.props.trackerWidth * 0.5)), -130, 132);
    // const trackerArcPath = this.describeArc(this.props.width / 2, this.props.height / 2, (w / 2) * (1 - (this.props.trackerWidth * 0.5)), -130, map(this.props.value, this.props.min, this.props.max, -130, 132));
    // const textY = this.props.height + (this.props.fontSize > 0 ? this.props.textOffsetY : 0);

    // // Calculate proportional font size if this.props.fontSize is 0
    // const fontSize = this.props.fontSize > 0 ? this.props.fontSize : w * 0.3; // Example: 10% of w


    return `
    <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${this.props.width} ${this.props.height}" width="100%" height="100%" preserveAspectRatio="none">
    <svg width="${this.props.width}" height="${this.props.height}" fill="none" xmlns="http://www.w3.org/2000/svg">
    <rect x="0" y="${this.props.height * .2}" width="${this.props.width}" height="${this.props.height * .6}" rx="4" fill="#717171" stroke-width="2" stroke="black"/>
    <rect x="${map(this.props.value, this.props.min, this.props.max, 0, this.props.width - this.props.width * .05)}" y="0" width="${this.props.width * .05}" height="${this.props.height}" rx="4" fill="white" stroke-width="2" stroke="black"/>
    </svg>

    `;
    // return `
    //   <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${this.props.width} ${this.props.height}" width="100%" height="100%" preserveAspectRatio="none">
    //   <path d='${trackerPath}' id="arc" fill="none" stroke=${this.props.trackerBackgroundColour} stroke-width=${this.props.trackerWidth * 0.5 * w} />
    //   <path d='${trackerArcPath}' id="arc" fill="none" stroke=${this.props.trackerColour} stroke-width=${this.props.trackerWidth * 0.5 * w} /> 
    //   <circle cx=${this.props.width / 2} cy=${this.props.height / 2} r=${(w / 2) - this.props.trackerWidth * 0.5 * w} stroke=${this.props.outlineColour} stroke-width=${this.props.outlineWidth} fill=${this.props.colour} />
    //   <text text-anchor="middle" x=${this.props.width / 2} y=${textY} font-size="${fontSize}px" font-family="${this.props.fontFamily}" stroke="none" fill="${this.props.fontColour}">${this.props.text}</text>
    //   </svg>
    // `;
    return "";
  }


}



