/**
 * Label class
 */
export class Label {
    constructor() {
        this.props = {
            "top": 0,
            "left": 0,
            "width": 600,
            "height": 300,
            "caption": "",
            "type": "label",
            "colour": "#888888",
            "channel": "label",
            "fontColour": "#dddddd",
            "fontFamily": "Verdana",
            "fontSize": 14,
            "corners": 4,
            "align": "centre",
            "visible": 1,
            "label": "Default Label"
        }

        this.panelSections = {
            "Properties": ["type"],
            "Bounds": ["left", "top", "width", "height"],
            "Text": ["label", "fontColour", "fontSize", "fontFamily", "align"],
            "Colours": ["colour"]
        };
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
        const fontSize = this.props.fontSize > 0 ? this.props.fontSize : this.props.height * 0.8;
        const alignMap = {
            'left': 'end',
            'center': 'middle',
            'centre': 'middle',
            'right': 'start',
        };
        const svgAlign = alignMap[this.props.align] || this.props.align;

        return `
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${this.props.width} ${this.props.height}" width="100%" height="100%" preserveAspectRatio="none">
                <rect width="${this.props.width}" height="${this.props.height}" x="0" y="0" rx="${this.props.corners}" ry="${this.props.corners}" fill="${this.props.colour}" 
                    pointer-events="all"></rect>
                <text x="${this.props.width / 2}" y="${this.props.height / 2}" font-family="${this.props.fontFamily}" font-size="${fontSize}"
                    fill="${this.props.fontColour}" text-anchor="${svgAlign}" alignment-baseline="middle" 
                    style="pointer-events: none;">${this.props.label}</text>
            </svg>
        `;
    }
}
