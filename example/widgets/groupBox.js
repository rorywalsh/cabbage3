/**
 * Form class
 */
export class GroupBox {
  constructor() {
    this.props = {
      "width": 600,
      "height": 300,
      "text": "Groupbox",
      "type": "groupbox",
      "colour": "#88888800",
      "fontColour": "#888888",
      "corners": 2,
      "channel": "groupbox",
      "text": "", // Text displayed on the slider
      "fontFamily": "Verdana", // Font family for the text displayed on the slider
      "fontSize": 0, // Font size for the text displayed on the slider
      "align": "centre", // Alignment of the text on the slider  
    };

    this.panelSections = {
      "Properties": ["type"],
      "Bounds": ["top", "left", "width", "height"],
      "Text": ["text", "fontSize", "fontFamily", "fontColour", "align"],
      "Colours": ["colour"]
    };

    console.log("adding gp")
  }




  getInnerHTML() {
    // Create the SVG content with the specified rectangle and outline path
    const svgContent = `
    <svg width="300" height="200" xmlns="http://www.w3.org/2000/svg">
    <!-- Outer rectangle with rounded corners -->
    <rect x="10" y="10" width="280" height="150" rx="8" ry="8" fill="#f0f0f0" stroke="#ccc" stroke-width="1"/>
    
    <!-- Text label with background -->
    <rect x="10" y="10" width="100" height="30" rx="8" ry="8" fill="#f0f0f0"/>
    <text x="25" y="30" font-family="Arial" font-size="14" fill="black">Group Title</text>
    
    <!-- Decorative lines -->
    <line x1="10" y1="40" x2="110" y2="40" stroke="#ccc" stroke-width="1"/>
    <line x1="110" y1="10" x2="110" y2="40" stroke="#ccc" stroke-width="1"/>
  </svg>
  
    `;
  
    return svgContent;
  }
}
