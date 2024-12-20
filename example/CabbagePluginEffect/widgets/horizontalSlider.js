import { Cabbage } from "../cabbage.js";
import { CabbageUtils, CabbageColours } from "../utils.js";

/**
 * Horizontal Slider (hslider) class
 */
export class HorizontalSlider {
  constructor() {
    this.props = {
      "top": 10, // Top position of the horizontal slider
      "left": 10, // Left position of the horizontal slider
      "width": 160, // Width of the horizontal slider
      "height": 20, // Height of the horizontal slider
      "channel": "rslider", // Unique identifier for the horizontal slider
      "min": 0, // Minimum value of the slider
      "max": 1, // Maximum value of the slider
      "value": 0, // Current value of the slider
      "skew": 1, // Skew factor for the slider (for non-linear scales)
      "increment": 0.001, // Value increment/decrement when moving the slider
      "text": "", // Text displayed next to the slider
      "fontFamily": "Verdana", // Font family for the text
      "fontSize": 0, // Font size for the text
      "align": "centre", // Text alignment within the slider (center, left, right)
      "valueTextBox": 0, // Whether to display a text box showing the current value
      "colour": CabbageColours.getColour("blue"), // Background color of the slider track
      "trackerColour": CabbageColours.getColour('green'), // Color of the slider thumb
      "trackerBackgroundColour": "#ffffff", // Background color of the slider thumb
      "trackerOutlineColour": "#525252", // Color of the outline around the slider thumb
      "fontColour": "#dddddd", // Color of the text displayed next to the slider
      "outlineColour": "#999999", // Color of the slider outline
      "textBoxColour": "#555555", // Color of the value text box (if enabled)
      "trackerOutlineWidth": 1, // Width of the outline around the slider thumb
      "outlineWidth": 1, // Width of the slider outline
      "markerThickness": 0.2, // Thickness of the slider markers
      "markerStart": 0.1, // Start position of the slider markers
      "markerEnd": 0.9, // End position of the slider markers
      "type": "hslider", // Type of the slider (horizontal)
      "decimalPlaces": 1, // Number of decimal places to display for the slider value
      "velocity": 0, // Velocity of slider movement (for gesture-based interaction)
      "visible": 1, // Visibility of the slider (0 for hidden, 1 for visible)
      "popup": 1, // Whether to show a popup when interacting with the slider
      "automatable": 1, // Whether the slider value can be automated (0 for no, 1 for yes)
      "valuePrefix": "", // Prefix to display before the slider value
      "valuePostfix": "", // Postfix to display after the slider value
      "presetIgnore": 0 // Whether the slider should be ignored in presets (0 for no, 1 for yes)
  };
  

    this.panelSections = {
      "Info": ["type", "channel"],
      "Bounds": ["left", "top", "width", "height"],
      "Range": ["min", "max", "default", "skew", "increment"],
      "Text": ["text", "fontSize", "fontFamily", "fontColour", "textOffsetY", "align"],
      "Colours": ["colour", "trackerBackgroundColour", "trackerStrokeColour", "outlineColour", "textBoxOutlineColour", "textBoxColour"]
    };

    this.parameterIndex = 0;
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
    if(this.props.active === 0) {
      return '';
    }

    let textWidth = this.props.text ? CabbageUtils.getStringWidth(this.props.text, this.props) : 0;
    textWidth = this.props.sliderOffsetX > 0 ? this.props.sliderOffsetX : textWidth;
    const valueTextBoxWidth = this.props.valueTextBox ? CabbageUtils.getNumberBoxWidth(this.props) : 0;
    const sliderWidth = this.props.width - textWidth - valueTextBoxWidth;
  
    
    if (evt.offsetX >= textWidth && evt.offsetX <= textWidth + sliderWidth && evt.target.tagName !== "INPUT") {
      this.isMouseDown = true;
      this.startX = evt.offsetX - textWidth;
      this.props.value = CabbageUtils.map(this.startX, 0, sliderWidth, this.props.min, this.props.max);
  
      window.addEventListener("pointermove", this.moveListener);
      window.addEventListener("pointerup", this.upListener);
  
      this.props.value = Math.round(this.props.value / this.props.increment) * this.props.increment;
      this.startValue = this.props.value;
      CabbageUtils.updateInnerHTML(this.props.channel, this);
    }
  }
  

  mouseEnter(evt) {
    if(this.props.active === 0) {
      return '';
    }
    const popup = document.getElementById('popupValue');
    const form = document.getElementById('MainForm');
    const rect = form.getBoundingClientRect();
    this.decimalPlaces = CabbageUtils.getDecimalPlaces(this.props.increment);
  
    if (popup && this.props.popup) {
      popup.textContent = this.props.valuePrefix + parseFloat(this.props.value).toFixed(this.decimalPlaces) + this.props.valuePostfix;
  
      // Calculate the position for the popup
      const sliderLeft = this.props.left;
      const sliderWidth = this.props.width;
      const formLeft = rect.left;
      const formWidth = rect.width;
  
      // Determine if the popup should be on the right or left side of the slider
      const sliderCenter = formLeft + (formWidth / 2);
      let popupLeft;
      if (sliderLeft + (sliderWidth) > sliderCenter) {
        // Place popup on the left of the slider thumb
        popupLeft = formLeft + sliderLeft - popup.offsetWidth - 10;
        console.log("Pointer on the left");
        popup.classList.add('right');
      } else {
        // Place popup on the right of the slider thumb
        popupLeft = formLeft + sliderLeft + sliderWidth + 10;
        console.log("Pointer on the right");
        popup.classList.remove('right');
      }
  
      const popupTop = rect.top + this.props.top; // Adjust top position relative to the form's top
  
      // Set the calculated position
      popup.style.left = `${popupLeft}px`;
      popup.style.top = `${popupTop}px`;
      popup.style.display = 'block';
      popup.classList.add('show');
      popup.classList.remove('hide');
    }
  }
  

  mouseLeave(evt) {
    if(this.props.active === 0) {
      return '';
    }
    if (!this.isMouseDown) {
      const popup = document.getElementById('popupValue');
      popup.classList.add('hide');
      popup.classList.remove('show');
    }
  }

  addVsCodeEventListeners(widgetDiv, vs) {
    this.vscode = vs;
    widgetDiv.addEventListener("pointerdown", this.pointerDown.bind(this));
    widgetDiv.addEventListener("mouseenter", this.mouseEnter.bind(this));
    widgetDiv.addEventListener("mouseleave", this.mouseLeave.bind(this));
    widgetDiv.HorizontalSliderInstance = this;
  }

  addEventListeners(widgetDiv) {
    widgetDiv.addEventListener("pointerdown", this.pointerDown.bind(this));
    widgetDiv.addEventListener("mouseenter", this.mouseEnter.bind(this));
    widgetDiv.addEventListener("mouseleave", this.mouseLeave.bind(this));
    widgetDiv.HorizontalSliderInstance = this;
  }

  pointerMove({ clientX }) {
    if(this.props.active === 0) {
      return '';
    }
    let textWidth = this.props.text ? CabbageUtils.getStringWidth(this.props.text, this.props) : 0;
    textWidth = this.props.sliderOffsetX > 0 ? this.props.sliderOffsetX : textWidth;
    const valueTextBoxWidth = this.props.valueTextBox ? CabbageUtils.getNumberBoxWidth(this.props) : 0;
    const sliderWidth = this.props.width - textWidth - valueTextBoxWidth;
  
    // Get the bounding rectangle of the slider
    const sliderRect = document.getElementById(this.props.channel).getBoundingClientRect();
  
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
    CabbageUtils.updateInnerHTML(this.props.channel, this);

    //get normalised value
    const normValue = CabbageUtils.map(this.props.value, this.props.min, this.props.max, 0, 1);
    // Post message if vscode is available
    const msg = { paramIdx:this.parameterIndex, channel: this.props.channel, value: normValue, channelType: "number" }
    console.log(newValue);
    Cabbage.sendParameterUpdate(this.vscode, msg);
  }
  
  handleInputChange(evt) {
    if (evt.key === 'Enter') {
      const inputValue = parseFloat(evt.target.value);
      if (!isNaN(inputValue) && inputValue >= this.props.min && inputValue <= this.props.max) {
        this.props.value = inputValue;
        CabbageUtils.updateInnerHTML(this.props.channel, this);
        widgetDiv.querySelector('input').focus();
      }
    }
  }

  getInnerHTML() {
    if(this.props.visible === 0) {
      return '';
    }
    const popup = document.getElementById('popupValue');
    if (popup) {
      popup.textContent = this.props.valuePrefix + parseFloat(this.props.value).toFixed(this.decimalPlaces) + this.props.valuePostfix;
    }

    const alignMap = {
      'left': 'start',
      'center': 'middle',
      'centre': 'middle',
      'right': 'end',
    };

    const svgAlign = alignMap[this.props.align] || this.props.align;

    // Add padding if alignment is 'end' or 'middle'
    const padding = (svgAlign === 'end' || svgAlign === 'middle') ? 5 : 0; // Adjust the padding value as needed

    // Calculate text width and update SVG width
    let textWidth = this.props.text ? CabbageUtils.getStringWidth(this.props.text, this.props) : 0;
    textWidth = (this.props.sliderOffsetX > 0 ? this.props.sliderOffsetX : textWidth) - padding;
    const valueTextBoxWidth = this.props.valueTextBox ? CabbageUtils.getNumberBoxWidth(this.props) : 0;
    const sliderWidth = this.props.width - textWidth - valueTextBoxWidth - padding; // Subtract padding from sliderWidth

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
        <rect x="1" y="${this.props.height * .2}" width="${sliderWidth - 2}" height="${this.props.height * .6}" rx="4" fill="${this.props.trackerBackgroundColour}" stroke-width="${this.props.outlineWidth}" stroke="black"/>
        <rect x="1" y="${this.props.height * .2}" width="${Math.max(0, CabbageUtils.map(this.props.value, this.props.min, this.props.max, 0, sliderWidth))}" height="${this.props.height * .6}" rx="4" fill="${this.props.trackerColour}" stroke-width="${this.props.trackerOutlineWidth}" stroke="${this.props.trackerOutlineColour}"/> 
        <rect x="${CabbageUtils.map(this.props.value, this.props.min, this.props.max, 0, sliderWidth - sliderWidth * .05 - 1) + 1}" y="0" width="${sliderWidth * .05 - 1}" height="${this.props.height}" rx="4" fill="${this.props.colour}" stroke-width="${this.props.outlineWidth}" stroke="black"/>
      </svg>
    `;

    const valueTextElement = this.props.valueTextBox ? `
      <foreignObject x="${textWidth + sliderWidth}" y="0" width="${valueTextBoxWidth}" height="${this.props.height}">
        <input type="text" value="${this.props.value.toFixed(CabbageUtils.getDecimalPlaces(this.props.increment))}"
        style="width:100%; outline: none; height:100%; text-align:center; font-size:${fontSize}px; font-family:${this.props.fontFamily}; color:${this.props.fontColour}; background:none; border:none; padding:0; margin:0;"
        onKeyDown="document.getElementById('${this.props.channel}').HorizontalSliderInstance.handleInputChange(event)"/>
      </foreignObject>
    ` : '';

    return `
      <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${this.props.width} ${this.props.height}" width="${this.props.width}" height="${this.props.height}" preserveAspectRatio="none">
        ${textElement}
        ${sliderElement}
        ${valueTextElement}
      </svg>
    `;
  }

  
  
}
