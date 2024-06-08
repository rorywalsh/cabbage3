import { CabbageUtils } from "./utils.js";

export class Button {
  constructor() {
    this.props = {
      "top": 10,
      "left": 10,
      "width": 60,
      "height": 60,
      "channel": "rslider",
      "corners": 2,
      "min": 0,
      "max": 1,
      "value": 0,
      "default": 0,
      "index": 0,
      "textOn": "",
      "textOff": "",
      "fontFamily": "Verdana",
      "fontSize": 0,
      "align": "centre",
      "colourOn": "#0295cf",
      "colourOff": "#0295cf",
      "fontColourOn": "#dddddd",
      "fontColourOff": "#dddddd",
      "outlineColour": "#dddddd",
      "outlineWidth": 2,
      "name": "",
      "type": "button",
      "visible": 1,
      "automatable": 1,
      "presetIgnore": 0
    }

    this.panelSections = {
      "Info": ["type", "channel"],
      "Bounds": ["left", "top", "width", "height"],
      "Text": ["text", "fontSize", "fontFamily", "fontColourOn", "fontColourOff", "align"], // Changed from textOffsetY to textOffsetX for vertical slider
      "Colours": ["colourOn", "colourOff", "outlineColour"]
    };

    this.vscode = null;
    this.isMouseDown = false;
    this.state = false;
  }

  pointerUp() {
    if (this.props.active === 0) {
      return '';
    }
    this.isMouseDown = false;
  }

  pointerDown(evt) {
    if (this.props.active === 0) {
      return '';
    }
    console.log("mouse down")
    this.isMouseDown = true;
    this.state =! this.state;
    document.getElementById(this.props.name).innerHTML = this.getSVG();
  }


  mouseEnter(evt) {
    if (this.props.active === 0) {
      return '';
    }
  }


  mouseLeave(evt) {

  }

  addVsCodeEventListeners(widgetDiv, vs) {
    this.vscode = vs;
    widgetDiv.addEventListener("pointerdown", this.pointerUp.bind(this));
    widgetDiv.addEventListener("pointerup", this.pointerDown.bind(this));
    widgetDiv.addEventListener("mouseenter", this.mouseEnter.bind(this));
    widgetDiv.addEventListener("mouseleave", this.mouseLeave.bind(this));
    widgetDiv.VerticalSliderInstance = this;
  }

  addEventListeners(widgetDiv) {
    widgetDiv.addEventListener("pointerup", this.pointerUp.bind(this));
    widgetDiv.addEventListener("pointerdown", this.pointerDown.bind(this));
    widgetDiv.addEventListener("mouseenter", this.mouseEnter.bind(this));
    widgetDiv.addEventListener("mouseleave", this.mouseLeave.bind(this));
    widgetDiv.VerticalSliderInstance = this;
  }

  getSVG() {
    if (this.props.visible === 0) {
      return '';
    }

    const alignMap = {
      'left': 'start',
      'center': 'middle',
      'centre': 'middle',
      'right': 'end',
    };

    const svgAlign = alignMap[this.props.align] || this.props.align;
    const fontSize = this.props.fontSize > 0 ? this.props.fontSize : this.props.height * 0.6;
    return `
      <svg width="${this.props.width}" height="${this.props.height}" xmlns="http://www.w3.org/2000/svg">
        <rect x="${this.props.corners}" y="${this.props.corners}" width="${this.props.width-this.props.corners*2}" height="${this.props.height-this.props.corners*2}" fill="${this.props.colourOff}" stroke="${this.props.outlineColour}"
          stroke-width="${this.props.outlineWidth}" rx="${this.props.corners}" ry="${this.props.corners}"></rect>
        <text x="${this.props.width / 2}" y="${this.props.height / 2}" font-family="${this.props.fontFamily}" font-size="${fontSize}"
          fill="${this.props.fontColourOff}" text-anchor="${svgAlign}" alignment-baseline="middle">${this.state ? this.props.textOn : this.props.textOff}</text>
      </svg>
    `;
  }
}
