
Number.prototype.map = function (in_min, in_max, out_min, out_max) {
  return ((this - in_min) * (out_max - out_min)) / (in_max - in_min) + out_min;
};

function clamp(num, min, max) {
  return Math.max(min, Math.min(num, max));
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
      "guiRefresh": 128,
      "identChannel": "",
      "automatable": 0.0,
      "visible": 1,
      "scrollbars": 0,
      "titleBarColour": '57, 70, 76',
      "titleBarGradient": 0.15,
      "titleBarHeight": 24,
      "style": "",
      "channelType": "number",
      "colour": '2, 149, 207'
    }
  }
}

export class RotarySlider {
  constructor() {
    this.props = {
      "top": 10,
      "left": 10,
      "width": 60,
      "height": 60,
      "channel": 'rslider',
      "min": 0,
      "max": 1,
      "value": 0,
      "sliderSkew": 1,
      "increment": .001,
      "text": "",
      "valueTextBox": 0.,
      "colour": '#dddddd',
      "trackerColour": '#d1d323',
      "trackerBackgroundColour": '#000000',
      "trackerStrokeColour": '#222222',
      "trackerStrokeWidth": 1,
      "trackerWidth": .5,
      "outlineWidth" : 0.3,
      "trackerColour": '#dddddd',
      "fontColour": '#222222',
      "textColour": '#222222',
      "outlineColour": '#999999',
      "textBoxOutlineColour": '#999999',
      "textBoxColour": '#555555',
      "markerColour": '#222222',
      "markerThickness": .2,
      "markerStart": 0.5,
      "markerEnd": 0.9,
      "name": "",
      "type": "rslider",
      "kind": "rotary",
      "decimalPlaces": 1,
      "velocity": 0,
      "identChannel": "",
      "trackerStart": 0.1,
      "trackerEnd": 0.9,
      "trackerCentre": 0.1,
      "visible": 1,
      "automatable": 1,
      "valuePrefix": "",
      "valuePostfix": ""
    }

    this.moveListener = this.pointerMove.bind(this);
    this.upListener = this.pointerUp.bind(this);
    this.startY = 0;
    this.startValue = 0;
    this.vscode = null;
  }

  pointerUp() { 
    window.removeEventListener("pointermove", this.moveListener);
    window.removeEventListener("pointerup", this.upListener);
    console.log('pointer up');
  }

  pointerDown(evt) {
    // console.log('slider on down');
    this.startY = evt.clientY;
    this.startValue = this.props.value;
    window.addEventListener("pointermove", this.moveListener);
    window.addEventListener("pointerup", this.upListener);
  }

  addEventListeners(widgetDiv, vs) {
    this.vscode = vs;
    widgetDiv.addEventListener("pointerdown", this.pointerDown.bind(this));
  }

  pointerMove({ clientY }) {
    // console.log('slider on move');
    const steps = 200;
    const valueDiff = ((this.props.max - this.props.min) * (clientY - this.startY)) / steps;
    const value = clamp(this.startValue - valueDiff, this.props.min, this.props.max);
   
    this.props.value = Math.round(value / this.props.increment) * this.props.increment;
    const widgetDiv = document.getElementById(this.props.name);
    widgetDiv.innerHTML = this.getSVG();

    const msg = {channel:this.props.channel, value: this.props.value.map(this.props.min, this.props.max, 0, 1)}
    this.vscode.postMessage({
      command: 'channelUpdate',
      text: JSON.stringify(msg)
    })
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
    console.log(arguments)
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
    const w = (this.props.width > this.props.height ? this.props.height : this.props.width)*.75;
    const trackerPath = this.describeArc(   this.props.width / 2, this.props.height / 2, (w/2)*(1-(this.props.trackerWidth*.5)), -130, 132);
    const trackerArcPath = this.describeArc(this.props.width / 2, this.props.height / 2, (w/2)*(1-(this.props.trackerWidth*.5)), -130, this.props.value.map(this.props.min, this.props.max, -130, 132));

  

return `
      <svg xmlns="http://www.w3.org/2000/svg"  viewBox="0 0 ${this.props.width} ${this.props.height}" width="100%" height="100%" preserveAspectRatio="none">
      <path d='${trackerPath}' id="arc" fill="none" stroke=${this.props.trackerBackgroundColour} stroke-width=${this.props.trackerWidth*.5*w} />
      <path d='${trackerArcPath}' id="arc" fill="none" stroke=${this.props.trackerColour} stroke-width=${this.props.trackerWidth*.5*w} /> 
      <circle cx=${this.props.width / 2} cy=${this.props.height / 2} r=${(w / 2) - this.props.trackerWidth*.5*w} stroke=${this.props.outlineColour} stroke-width=${this.props.outlineWidth} fill=${this.props.colour} />
      </svg>
      `;
  }
}





//
// export function WidgetSVG(widget) {
//   switch (widget.type) {
//     case 'rslider':
//       const arcPath = describeArc(widget.width / 2, widget.height / 2, widget.trackerWidth, -130, widget.value.map(widget.min, widget.max, -130, 132));
//       const trackerPath = describeArc(widget.width / 2, widget.height / 2, widget.trackerWidth, -130, 132);
//       return `
//       <svg xmlns="http://www.w3.org/2000/svg" width="100%" height="100%">
//         <path d='${trackerPath}' id="arc" fill="none" stroke=${widget.trackerBackgroundColour} stroke-width=${widget.trackerWidth} />
//         <path d='${arcPath}' id="arc" fill="none" stroke=${widget.trackerColour} stroke-width=${widget.trackerWidth} />
//         <circle cx=${widget.width / 2} cy=${widget.height / 2} r=${widget.thumbRadius} stroke=${widget.thumbStrokeColour} stroke-width=${widget.thumbStrokeWidth} fill=${widget.thumbColour} />
//         <text text-anchor="middle" x=${widget.width / 2} y=${widget.height} font-size="${widget.fontHeight}px" font-family="Arial, Helvetica, sans-serif" fill="${widget.textColour}">${widget.text}</text>
//     </svg>
//       `;
//     //       return `
//     //       <svg width="100%" height="100%" viewBox="0 0 87 99" fill="none" xmlns="http://www.w3.org/2000/svg">
//     // <path d="M65.9417 80.5413C73.837 75.7352 79.9735 68.5131 83.4416 59.9454C86.9097 51.3777 87.5248 41.9205 85.1957 32.9758C82.8666 24.031 77.7173 16.0749 70.511 10.2866C63.3048 4.49843 54.4253 1.18627 45.1887 0.841165C35.9522 0.496059 26.8503 3.13637 19.2322 8.37071C11.6142 13.6051 5.88549 21.1548 2.8954 29.9008C-0.0946829 38.6468 -0.187024 48.1235 2.63207 56.9261C5.45116 65.7287 11.0316 73.3886 18.5462 78.7704L43.5833 43.8112L65.9417 80.5413Z" fill="${widget.trackerBgColour}"/>
//     // <circle cx="44" cy="44" r="33" fill="${widget.colour}"/>
//     // <rect x="23" y="66.8579" width="13.3991" height="5.72696" rx="1" transform="rotate(-54.1296 23 66.8579)" fill="${widget.markerColour}"/>
//     // </svg>
//     //       `;
//     default:
//       return "";
//   }
// }