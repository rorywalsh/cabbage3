import { CabbageUtils, CabbageColours } from "../utils.js";
import { Cabbage } from "../cabbage.js";
/**
 * Rotary Slider (rslider) class
 */
export class RotarySlider {
  constructor() {
    this.props = {
      "top": 10, // Top position of the rotary slider widget
      "left": 10, // Left position of the rotary slider widget
      "width": 60, // Width of the rotary slider widget
      "height": 60, // Height of the rotary slider widget
      "channel": "rslider", // Unique identifier for the rotary slider widget
      "min": 0, // Minimum value of the slider
      "max": 1, // Maximum value of the slider
      "value": 0, // Current value of the slider
      "skew": 1, // Skew factor for the slider
      "increment": 0.001, // Incremental value change per step
      "index": 0, // Index of the slider
      "text": "", // Text displayed on the slider
      "fontFamily": "Verdana", // Font family for the text displayed on the slider
      "fontSize": 0, // Font size for the text displayed on the slider
      "align": "centre", // Alignment of the text on the slider
      "textOffsetY": 0, // Vertical offset for the text displayed on the slider
      "valueTextBox": 0, // Display a textbox showing the current value
      "colour": CabbageColours.getColour("blue"), // Background color of the slider
      "trackerColour": CabbageColours.getColour('green'), // Color of the slider tracker
      "trackerBackgroundColour": "#ffffff", // Background color of the slider tracker
      "trackerOutlineColour": "#525252", // Outline color of the slider tracker
      "fontColour": "#dddddd", // Font color for the text displayed on the slider
      "outlineColour": "#525252", // Color of the slider outline
      "textBoxOutlineColour": "#999999", // Outline color of the value textbox
      "textBoxColour": "#555555", // Background color of the value textbox
      "markerColour": "#222222", // Color of the marker on the slider
      "trackerOutlineWidth": 3, // Outline width of the slider tracker
      "trackerWidth": 20, // Width of the slider tracker
      "outlineWidth": 2, // Width of the slider outline
      "type": "rslider", // Type of the widget (rotary slider)
      "decimalPlaces": 1, // Number of decimal places in the slider value
      "velocity": 0, // Velocity value for the slider
      "popup": 1, // Display a popup when the slider is clicked
      "visible": 1, // Visibility of the slider
      "automatable": 1, // Ability to automate the slider
      "valuePrefix": "", // Prefix to be displayed before the slider value
      "valuePostfix": "", // Postfix to be displayed after the slider value
      "presetIgnore": 0, // Ignore preset value for the slider
    };


    this.panelSections = {
      "Properties": ["type", "channel"],
      "Bounds": ["left", "top", "width", "height"],
      "Range": ["min", "max", "value", "skew", "increment"],
      "Text": ["text", "fontSize", "fontFamily", "fontColour", "textOffsetY", "align"],
      "Colours": ["colour", "trackerColour", "trackerBackgroundColour", "trackerOutlineColour", "trackerStrokeColour", "outlineColour", "textBoxOutlineColour", "textBoxColour", "markerColour"]
    };

    this.moveListener = this.pointerMove.bind(this);
    this.upListener = this.pointerUp.bind(this);
    this.startY = 0;
    this.startValue = 0;
    this.vscode = null;
    this.isMouseDown = false;
    this.decimalPlaces = 0;
    this.parameterIndex = 0;
  }

  pointerUp() {
    if (this.props.active === 0) {
      return '';
    }
    const popup = document.getElementById('popupValue');
    popup.classList.add('hide');
    popup.classList.remove('show');
    window.removeEventListener("pointermove", this.moveListener);
    window.removeEventListener("pointerup", this.upListener);
    this.isMouseDown = false;
  }

  pointerDown(evt) {
    if (this.props.active === 0) {
      return '';
    }

    this.isMouseDown = true;
    this.startY = evt.clientY;
    console.log(this.props.value)
    this.startValue = this.props.value;
    window.addEventListener("pointermove", this.moveListener);
    window.addEventListener("pointerup", this.upListener);
  }

  mouseEnter(evt) {
    if (this.props.active === 0) {
      return '';
    }


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

      const popupTop = rect.top + this.props.top + this.props.height * .5; // Adjust top position relative to the form's top

      // Set the calculated position
      popup.style.left = `${popupLeft}px`;
      popup.style.top = `${popupTop}px`;
      popup.style.display = 'block';
      popup.classList.add('show');
      popup.classList.remove('hide');
    }

    console.log("pointerEnter", this);
  }


  mouseLeave(evt) {
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
    widgetDiv.RotarySliderInstance = this;
  }

  addEventListeners(widgetDiv) {
    console.log(JSON.stringify(this.props, null, 2));
    widgetDiv.addEventListener("pointerdown", this.pointerDown.bind(this));
    widgetDiv.addEventListener("mouseenter", this.mouseEnter.bind(this));
    widgetDiv.addEventListener("mouseleave", this.mouseLeave.bind(this));
    widgetDiv.RotarySliderInstance = this;
  }

  pointerMove({ clientY }) {
    if (this.props.active === 0) {
      return '';
    }

    const steps = 200;
    const valueDiff = ((this.props.max - this.props.min) * (clientY - this.startY)) / steps;
    const value = CabbageUtils.clamp(this.startValue - valueDiff, this.props.min, this.props.max);
    

    this.props.value = Math.round(value / this.props.increment) * this.props.increment;

    const widgetDiv = document.getElementById(this.props.channel);
    widgetDiv.innerHTML = this.getInnerHTML();

    const newValue = CabbageUtils.map(this.props.value, this.props.min, this.props.max, 0, 1);
    const msg = { paramIdx:this.parameterIndex, channel: this.props.channel, value: newValue, channelType: "number" }
    Cabbage.sendParameterUpdate(this.vscode, msg);
    
  }

  // https://stackoverflow.com/questions/20593575/making-circular-progress-bar-with-html5-svg
  polarToCartesian(centerX, centerY, radius, angleInDegrees) {
    var angleInRadians = ((angleInDegrees - 90) * Math.PI) / 180.0;
    return {
      x: centerX + radius * Math.cos(angleInRadians),
      y: centerY + radius * Math.sin(angleInRadians),
    };
  }

  describeArc(x, y, radius, startAngle, endAngle) {
    var start = this.polarToCartesian(x, y, radius, endAngle);
    var end = this.polarToCartesian(x, y, radius, startAngle);

    var largeArcFlag = "0";
    if (endAngle >= startAngle) {
      largeArcFlag = endAngle - startAngle <= 180 ? "0" : "1";
    } else {
      largeArcFlag = endAngle + 360.0 - startAngle <= 180 ? "0" : "1";
    }

    var d = ["M", start.x, start.y, "A", radius, radius, 0, largeArcFlag, 0, end.x, end.y].join(" ");

    return d;
  }

  handleInputChange(evt) {
    if (evt.key === 'Enter') {
      const inputValue = parseFloat(evt.target.value);
      if (!isNaN(inputValue) && inputValue >= this.props.min && inputValue <= this.props.max) {
        this.props.value = inputValue;
        const widgetDiv = document.getElementById(this.props.channel);
        widgetDiv.innerHTML = this.getInnerHTML();
        widgetDiv.querySelector('input').focus();
      }
    }
    else if (evt.key === 'Esc') {
      const widgetDiv = document.getElementById(this.props.channel);
      widgetDiv.querySelector('input').blur();
    }
  }

  getInnerHTML() {
    if (this.props.visible === 0) {
      return '';
    }

    const popup = document.getElementById('popupValue');
    if (popup) {
      popup.textContent = this.props.valuePrefix + parseFloat(this.props.value).toFixed(this.decimalPlaces) + this.props.valuePostfix;
    }

    let w = (this.props.width > this.props.height ? this.props.height : this.props.width) * 0.75;
    const innerTrackerWidth = this.props.trackerWidth - this.props.trackerOutlineWidth;
    const innerTrackerEndPoints = this.props.trackerOutlineWidth * 0.5;
    const trackerOutlineColour = this.props.trackerOutlineWidth == 0 ? this.props.trackerBackgroundColour : this.props.trackerOutlineColour;
    const outerTrackerPath = this.describeArc(this.props.width / 2, this.props.height / 2, (w / 2) * (1 - (this.props.trackerWidth / this.props.width / 2)), -130, 132);
    const trackerPath = this.describeArc(this.props.width / 2, this.props.height / 2, (w / 2) * (1 - (this.props.trackerWidth / this.props.width / 2)), -(130 - innerTrackerEndPoints), 132 - innerTrackerEndPoints);
    const trackerArcPath = this.describeArc(this.props.width / 2, this.props.height / 2, (w / 2) * (1 - (this.props.trackerWidth / this.props.width / 2)), -(130 - innerTrackerEndPoints), CabbageUtils.map(this.props.value, this.props.min, this.props.max, -(130 - innerTrackerEndPoints), 132 - innerTrackerEndPoints));

    // Calculate proportional font size if this.props.fontSize is 0
    let fontSize = this.props.fontSize > 0 ? this.props.fontSize : w * 0.24;
    const textY = this.props.height + (this.props.fontSize > 0 ? this.props.textOffsetY : 0);
    let scale = 100;

    if (this.props.valueTextBox == 1) {
      scale = 0.7;
      const moveY = 5;

      const centerX = this.props.width / 2;
      const centerY = this.props.height / 2;
      const inputWidth = CabbageUtils.getNumberBoxWidth(this.props);
      const inputX = this.props.width / 2 - inputWidth / 2;

      return `
        <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${this.props.width} ${this.props.height}" width="100%" height="100%" preserveAspectRatio="none">
        <text text-anchor="middle" x=${this.props.width / 2} y="${fontSize}px" font-size="${fontSize}px" font-family="${this.props.fontFamily}" stroke="none" fill="${this.props.fontColour}">${this.props.text}</text>
        <g transform="translate(${centerX}, ${centerY + moveY}) scale(${scale}) translate(${-centerX}, ${-centerY})">
        <path d='${outerTrackerPath}' id="arc" fill="none" stroke=${trackerOutlineColour} stroke-width=${this.props.trackerWidth} />
        <path d='${trackerPath}' id="arc" fill="none" stroke=${this.props.trackerBackgroundColour} stroke-width=${innerTrackerWidth} />
        <path d='${trackerArcPath}' id="arc" fill="none" stroke=${this.props.trackerColour} stroke-width=${innerTrackerWidth} />
        <circle cx=${this.props.width / 2} cy=${this.props.height / 2} r=${(w / 2) - this.props.trackerWidth * 0.65} stroke=${this.props.outlineColour} stroke-width=${this.props.outlineWidth} fill=${this.props.colour} />
        </g>
        <foreignObject x="${inputX}" y="${textY - fontSize * 1.5}" width="${inputWidth}" height="${fontSize * 2}">
          <input type="text" xmlns="http://www.w3.org/1999/xhtml" value="${this.props.value.toFixed(CabbageUtils.getDecimalPlaces(this.props.increment))}"
          style="width:100%; outline: none; height:100%; text-align:center; font-size:${fontSize}px; font-family:${this.props.fontFamily}; color:${this.props.fontColour}; background:none; border:none; padding:0; margin:0;"
          onKeyDown="document.getElementById('${this.props.channel}').RotarySliderInstance.handleInputChange(event)"/>
          />
        </foreignObject>
        </svg>
        `;
    }

    return `
      <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${this.props.width} ${this.props.height}" width="${scale}%" height="${scale}%" preserveAspectRatio="none">
      <path d='${outerTrackerPath}' id="arc" fill="none" stroke=${trackerOutlineColour} stroke-width=${this.props.trackerWidth} />
      <path d='${trackerPath}' id="arc" fill="none" stroke=${this.props.trackerBackgroundColour} stroke-width=${innerTrackerWidth} />
      <path d='${trackerArcPath}' id="arc" fill="none" stroke=${this.props.trackerColour} stroke-width=${innerTrackerWidth} />
      <circle cx=${this.props.width / 2} cy=${this.props.height / 2} r=${(w / 2) - this.props.trackerWidth * 0.65} stroke=${this.props.outlineColour} stroke-width=${this.props.outlineWidth} fill=${this.props.colour} />
      <text text-anchor="middle" x=${this.props.width / 2} y=${textY} font-size="${fontSize}px" font-family="${this.props.fontFamily}" stroke="none" fill="${this.props.fontColour}">${this.props.text}</text>
      </svg>
    `;
  }

}
