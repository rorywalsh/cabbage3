/**
 * GenTable class
 */

import { CabbageUtils } from "../utils.js";


export class GenTable {
    constructor() {
        this.props = {
            "top": 0,
            "left": 0,
            "width": 200,
            "height": 100,
            "type": "gentable",
            "colour": "#888888",
            "outlineColour": "#dddddd",
            "outlineWidth": 1,
            "channel": "gentable",
            "backgroundColour": "#a8d388",
            "fontColour": "#dddddd",
            "fontFamily": "Verdana",
            "fontSize": 0,
            "corners": 4,
            "align": "centre",
            "visible": 1,
            "text": "Default Label",
            "tableNumber": 1,
            "samples": []
        }

        this.panelSections = {
            "Properties": ["type"],
            "Bounds": ["left", "top", "width", "height"],
            "Text": ["text", "fontColour", "fontSize", "fontFamily", "align"],
            "Colours": ["colour", "outlineColour", "backgroundColour"]
        };

        // Create canvas element during initialization
        this.canvas = document.createElement('canvas');
        this.ctx = this.canvas.getContext('2d');
    }

    addVsCodeEventListeners(widgetDiv, vs) {
        this.vscode = vs;
        widgetDiv.addEventListener("pointerdown", this.pointerDown.bind(this));
    }

    addEventListeners(widgetDiv) {
        widgetDiv.addEventListener("pointerdown", this.pointerDown.bind(this));
    }

    pointerDown() {
        console.log("Label clicked!");
    }

    getInnerHTML() {
        return ``;
    }

    updateTable(obj) {
        this.props.samples = obj["data"];
        this.canvas.width = this.props.width;
        this.canvas.height = this.props.height;
        // Clear canvas
        this.ctx.clearRect(0, 0, this.props.width, this.props.height);

        // Draw background
        this.ctx.fillStyle = this.props.backgroundColour;
        this.ctx.fillRect(0, 0, this.props.width, this.props.height);

        // Draw waveform
        this.ctx.strokeStyle = this.props.colour;
        this.ctx.lineWidth = this.props.outlineWidth;
        this.ctx.beginPath();
        this.ctx.moveTo(0, this.props.height / 2);

        if (!Array.isArray(this.props.samples) || this.props.samples.length === 0) {
            console.warn('No samples to draw.');
        } else {
            for (let i = 0; i < this.props.samples.length; i++) {
                const x = CabbageUtils.map(i, 0, this.props.samples.length, 0, this.props.width);
                const y = CabbageUtils.map(this.props.samples[i], -1, 1, this.props.height, 0);
                this.ctx.lineTo(x, y);
            }
        }

        this.ctx.stroke();
        this.ctx.closePath();

        // Update DOM with the canvas
        const widgetElement = document.getElementById(this.props.channel);
        if (widgetElement) {
            widgetElement.style.transform = `translate(${this.props.left}px, ${this.props.top}px)`;
            widgetElement.setAttribute('data-x', this.props.left);
            widgetElement.setAttribute('data-y', this.props.top);
            widgetElement.style.top = `${this.props.top}px`;
            widgetElement.style.left = `${this.props.left}px`;

            widgetElement.innerHTML = ''; // Clear existing content
            widgetElement.appendChild(this.canvas); // Append canvas
        } else {
            console.error(`Element with channel ${this.props.channel} not found.`);
        }

    }


}
