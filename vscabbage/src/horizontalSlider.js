import { CabbageUtils } from "./utils.js";

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
      "sliderOffsetX": 0,
      "valueTextBox": 0,
      "colour": "#0295cf",
      "trackerColour": "#93D200",
      "trackerBackgroundColour": "#ffffff",
      "trackerOutlineColour": "#525252",
      "fontColour": "#dddddd",
      "textColour": "#222222",
      "outlineColour": "#999999",
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
    let textWidth = this.props.text ? CabbageUtils.getStringWidth(this.props.text, this.props) : 0;
    textWidth = this.props.sliderOffsetX > 0 ? this.props.sliderOffsetX : textWidth;

    const sliderWidth = this.props.width - textWidth;

    if (evt.offsetX >= textWidth) {
      this.isMouseDown = true;
      this.startX = evt.offsetX - textWidth;
      this.props.value = CabbageUtils.map(this.startX, 0, sliderWidth, this.props.min, this.props.max);

      window.addEventListener("pointermove", this.moveListener);
      window.addEventListener("pointerup", this.upListener);

      this.props.value = Math.round(this.props.value / this.props.increment) * this.props.increment;
      this.startValue = this.props.value;
      const widgetDiv = document.getElementById(this.props.name);
      widgetDiv.innerHTML = this.getSVG();
    }
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

  pointerMove({ clientX }) {
    let textWidth = this.props.text ? CabbageUtils.getStringWidth(this.props.text, this.props) : 0;
    textWidth = this.props.sliderOffsetX > 0 ? this.props.sliderOffsetX : textWidth;
    const sliderWidth = this.props.width - textWidth;

    // Get the bounding rectangle of the slider
    const sliderRect = document.getElementById(this.props.name).getBoundingClientRect();

    // Calculate the relative position of the mouse pointer within the slider bounds
    let offsetX = clientX - sliderRect.left - textWidth;

    // Clamp the mouse position to stay within the bounds of the slider
    offsetX = CabbageUtils.clamp(offsetX, 0, sliderWidth);

    // Calculate the new value based on the mouse position
    let newValue = CabbageUtils.map(offsetX, 0, sliderWidth, this.props.min, this.props.max);
    newValue = Math.round(newValue / this.props.increment) * this.props.increment; // Round to the nearest increment

    // Update the slider value
    this.props.value = newValue;

    // Update the slider appearance
    const widgetDiv = document.getElementById(this.props.name);
    widgetDiv.innerHTML = this.getSVG();

    // Post message if vscode is available
    const msg = { channel: this.props.channel, value: CabbageUtils.map(this.props.value.map, this.props.min, this.props.max, 0, 1) }
    if (this.vscode) {
      this.vscode.postMessage({
        command: 'channelUpdate',
        text: JSON.stringify(msg)
      })
    }
    else {
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

    const alignMap = {
      'left': 'start',
      'center': 'middle',
      'centre': 'middle',
      'right': 'end',
    };
    
    const svgAlign = alignMap[this.props.align] || this.props.align;
    
    // Add padding if alignment is 'end' or 'centre'
    const padding = (svgAlign === 'end' || svgAlign === 'middle') ? 5 : 0; // Adjust the padding value as needed
    
    // Calculate text width and update SVG width
    let textWidth = this.props.text ? CabbageUtils.getStringWidth(this.props.text, this.props) : 0;
    textWidth = (this.props.sliderOffsetX > 0 ? this.props.sliderOffsetX : textWidth) - padding;
    const sliderWidth = this.props.width - textWidth - padding; // Subtract padding from sliderWidth
    
    const w = (sliderWidth > this.props.height ? this.props.height : sliderWidth) * 0.75;
    const textY = this.props.height / 2 + (this.props.fontSize > 0 ? this.props.textOffsetY : 0) + (this.props.height * 0.25); // Adjusted for vertical centering
    const fontSize = this.props.fontSize > 0 ? this.props.fontSize : this.props.height * 0.8;
    
    textWidth += padding;
    
    const textElement = this.props.text ? `
      <svg x="0" y="0" width="${textWidth}" height="${this.props.height}" preserveAspectRatio="xMinYMid meet" xmlns="http://www.w3.org/2000/svg">
        <text text-anchor="${svgAlign}" x="${svgAlign === 'end' ? textWidth - padding : (svgAlign === 'middle' ? (textWidth - padding) / 2 : 0)}" y="${textY}" font-size="${fontSize}px" font-family="${this.props.fontFamily}" stroke="none" fill="${this.props.fontColour}">
          ${this.props.text}
        </text>
      </svg>
    ` : '';
    
    const sliderElement = `
  <svg x="${textWidth}" width="${sliderWidth}" height="${this.props.height}" fill="none" xmlns="http://www.w3.org/2000/svg">
    <rect x="1" y="${this.props.height * .2}" width="${sliderWidth - 2}" height="${this.props.height * .6}" rx="4" fill="${this.props.trackerBackgroundColour}" stroke-width="2" stroke="black"/>
    <rect x="1" y="${this.props.height * .2}" width="${CabbageUtils.map(this.props.value, this.props.min, this.props.max, 0, sliderWidth) - 2}" height="${this.props.height * .6}" rx="4" fill="${this.props.trackerColour}" stroke-width="${this.props.trackerOutlineWidth}" stroke="${this.props.trackerOutlineColour}"/> 
    <rect x="${CabbageUtils.map(this.props.value, this.props.min, this.props.max, 0, sliderWidth - sliderWidth * .05 - 1) + 1}" y="0" width="${sliderWidth * .05 - 1}" height="${this.props.height}" rx="4" fill="${this.props.colour}" stroke-width="2" stroke="black"/>
`;

    return `
      <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${this.props.width} ${this.props.height}" width="${this.props.width}" height="${this.props.height}" preserveAspectRatio="none">
        ${textElement}
        ${sliderElement}
      </svg>
    `;
  }


}
