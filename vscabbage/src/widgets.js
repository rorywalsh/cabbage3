
Number.prototype.map = function (in_min, in_max, out_min, out_max) {
  return ((this - in_min) * (out_max - out_min)) / (in_max - in_min) + out_min;
};

function clamp(num, min, max) {
  return Math.max(min, Math.min(num, max));
}

function getDecimalPlaces(num) {
  const numString = num.toString();
  if (numString.includes('.')) {
    return numString.split('.')[1].length;
  } else {
    return 0;
  }
}

export class Form {
  constructor() {
    this.props = {
      "top": 0,
      "left": 0,
      "width": 600,
      "height": 300,
      "caption": "",
      "name": "MainForm",
      "type": "form",
      "colour": "#0295CF",
      "channel": "MainForm"
    }

    this.panelSections = {
      "Info": ["type"],
      "Bounds": ["width", "height"],
      "Text": ["caption"],
      "Colours": ["colour"]
    };
  }


  getSVG() {

    return `
      <svg xmlns="http://www.w3.org/2000/svg"  viewBox="0 0 ${this.props.width} ${this.props.height}" width="100%" height="100%" preserveAspectRatio="none">
      <rect width="${this.props.width} " height="${this.props.height}" x="0" y="0" rx="2" ry="2" fill="${this.props.colour}" />
      </svg>
      `;
  }
}

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
      "trackerColour": "#d1d323",
      "trackerBackgroundColour": "#000000",
      "trackerStrokeColour": "#222222",
      "trackerColour": "#dddddd",
      "fontColour": "#222222",
      "textColour": "#222222",
      "outlineColour": "#999999",
      "textBoxOutlineColour": "#999999",
      "textBoxColour": "#555555",
      "markerColour": "#222222",
      "trackerStrokeWidth": 1,
      "trackerWidth": 0.5,
      "outlineWidth": 0.3,
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
      "Colours": ["colour", "trackerColour", "trackerBackgroundColour", "trackerStrokeColour", "trackerColour", "outlineColour", "textBoxOutlineColour", "textBoxColour", "markerColour"]
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
    this.decimalPlaces = getDecimalPlaces(this.props.increment);

    if (popup) {
      
      popup.textContent = parseFloat(this.props.value).toFixed(this.decimalPlaces);
      const left = rect.left + this.props.left + this.props.width + 20;
      const top = rect.top + this.props.top + this.props.height/2;
      popup.style.left = `${left}px`;
      popup.style.top = `${top}px`;
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

  pointerMove({ clientY }) {
    // console.log('slider on move');
    const steps = 200;
    const valueDiff = ((this.props.max - this.props.min) * (clientY - this.startY)) / steps;
    const value = clamp(this.startValue - valueDiff, this.props.min, this.props.max);

    this.props.value = Math.round(value / this.props.increment) * this.props.increment;
    const widgetDiv = document.getElementById(this.props.name);
    widgetDiv.innerHTML = this.getSVG();

    const msg = { channel: this.props.channel, value: this.props.value.map(this.props.min, this.props.max, 0, 1) }
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
        "value": this.props.value.map(this.props.min, this.props.max, 0, 1)
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

  getSVG() {
    const popup = document.getElementById('popupValue');
    if (popup) {
      popup.textContent = parseFloat(this.props.value).toFixed(this.decimalPlaces);
    }

    const w = (this.props.width > this.props.height ? this.props.height : this.props.width) * 0.75;
    const trackerPath = this.describeArc(this.props.width / 2, this.props.height / 2, (w / 2) * (1 - (this.props.trackerWidth * 0.5)), -130, 132);
    const trackerArcPath = this.describeArc(this.props.width / 2, this.props.height / 2, (w / 2) * (1 - (this.props.trackerWidth * 0.5)), -130, this.props.value.map(this.props.min, this.props.max, -130, 132));
    const textY = this.props.height + (this.props.fontSize > 0 ? this.props.textOffsetY : 0);

    // Calculate proportional font size if this.props.fontSize is 0
    const fontSize = this.props.fontSize > 0 ? this.props.fontSize : w * 0.3; // Example: 10% of w

    return `
      <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${this.props.width} ${this.props.height}" width="100%" height="100%" preserveAspectRatio="none">
      <path d='${trackerPath}' id="arc" fill="none" stroke=${this.props.trackerBackgroundColour} stroke-width=${this.props.trackerWidth * 0.5 * w} />
      <path d='${trackerArcPath}' id="arc" fill="none" stroke=${this.props.trackerColour} stroke-width=${this.props.trackerWidth * 0.5 * w} /> 
      <circle cx=${this.props.width / 2} cy=${this.props.height / 2} r=${(w / 2) - this.props.trackerWidth * 0.5 * w} stroke=${this.props.outlineColour} stroke-width=${this.props.outlineWidth} fill=${this.props.colour} />
      <text text-anchor="middle" x=${this.props.width / 2} y=${textY} font-size="${fontSize}px" font-family="${this.props.fontFamily}" stroke="none" fill="${this.props.fontColour}">${this.props.text}</text>
      </svg>
    `;
  }


}



