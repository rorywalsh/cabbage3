import { CabbageUtils } from "./utils.js";

/**
 * Rotary Slider (rslider) class
 */
export class RotarySlider {
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
      "fontColour": "#dddddd",
      "textColour": "#ffffff",
      "outlineColour": "#525252",
      "textBoxOutlineColour": "#999999",
      "textBoxColour": "#555555",
      "markerColour": "#222222",
      "trackerOutlineWidth": 3,
      "trackerWidth": 20,
      "outlineWidth": 2,
      "markerThickness": 0.2,
      "markerStart": 0.5,
      "markerEnd": 0.9,
      "name": "",
      "type": "rslider",
      "kind": "rotary",
      "decimalPlaces": 1,
      "velocity": 0,
      "trackerStart": 0.1,
      "trackerEnd": 0.9,
      "trackerCentre": 0.1,
      "popup": 1,
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
      "Colours": ["colour", "trackerColour", "trackerBackgroundColour", "trackerStrokeColour", "outlineColour", "textBoxOutlineColour", "textBoxColour", "markerColour"]
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
    this.startY = evt.clientY;
    this.startValue = this.props.value;
    window.addEventListener("pointermove", this.moveListener);
    window.addEventListener("pointerup", this.upListener);
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
  
      const popupTop = rect.top + this.props.top + this.props.height*.5; // Adjust top position relative to the form's top
  
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
    widgetDiv.RotarySliderInstance = this;
  }

  addEventListeners(widgetDiv) {
    widgetDiv.addEventListener("pointerdown", this.pointerDown.bind(this));
    widgetDiv.addEventListener("mouseenter", this.mouseEnter.bind(this));
    widgetDiv.addEventListener("mouseleave", this.mouseLeave.bind(this));
    widgetDiv.RotarySliderInstance = this;
  }

  pointerMove({ clientY }) {
    const steps = 200;
    const valueDiff = ((this.props.max - this.props.min) * (clientY - this.startY)) / steps;
    const value = CabbageUtils.clamp(this.startValue - valueDiff, this.props.min, this.props.max);

    this.props.value = Math.round(value / this.props.increment) * this.props.increment;
    const widgetDiv = document.getElementById(this.props.name);
    widgetDiv.innerHTML = this.getSVG();

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
        const widgetDiv = document.getElementById(this.props.name);
        widgetDiv.innerHTML = this.getSVG();
        widgetDiv.querySelector('input').focus();
      }
    }
    else if (evt.key === 'Esc'){
      const widgetDiv = document.getElementById(this.props.name);
      widgetDiv.querySelector('input').blur();
    }
  }

  getSVG() {
    if(this.props.visible === 0) {
      return '';
    }
    const popup = document.getElementById('popupValue');
    if (popup) {
      popup.textContent = parseFloat(this.props.value).toFixed(this.decimalPlaces);
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
          onKeyDown="document.getElementById('${this.props.name}').RotarySliderInstance.handleInputChange(event)"/>
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
