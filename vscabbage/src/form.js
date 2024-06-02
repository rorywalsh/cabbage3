/**
 * Form class
 */
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