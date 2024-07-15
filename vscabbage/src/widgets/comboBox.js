import { CabbageUtils, CabbageColours } from "../utils.js";
import { Cabbage } from "../cabbage.js";

export class ComboBox {
    constructor() {
        this.props = {
            "top": 10, // Top position of the widget
            "left": 10, // Left position of the widget
            "width": 100, // Width of the widget
            "height": 30, // Height of the widget
            "channel": "comboBox", // Unique identifier for the widget
            "corners": 4, // Radius of the corners of the widget rectangle
            "fontFamily": "Verdana", // Font family for the text
            "fontSize": 14, // Font size for the text
            "align": "center", // Text alignment within the widget (left, center, right)
            "colour": CabbageColours.getColour("blue"), // Background color of the widget
            "items": "One, Two, Three", // List of items for the dropdown
            "text": "Select", // Default text displayed when no item is selected
            "fontColour": "#dddddd", // Color of the text
            "outlineColour": "#dddddd", // Color of the outline
            "outlineWidth": 2, // Width of the outline
            "visible": 1, // Visibility of the widget (0 for hidden, 1 for visible)
            "type": "combobox", // Type of the widget (combobox)
            "value": 0, // Value of the widget
            "automatable": 1, // Whether the widget value can be automated (0 for no, 1 for yes)
            "active": 1 // Whether the widget is active (0 for inactive, 1 for active)
        };
        
        this.panelSections = {
            "Properties": ["type"],
            "Bounds": ["top", "left", "width", "height"],
            "Text": ["text", "items", "fontFamily", "align", "fontSize", "fontColour"],
            "Colours": ["colour", "outlineColour"]
        };

        this.isMouseInside = false;
        this.isOpen = false;
        this.selectedItem = this.props.value > 0 ? this.props.items.split(",")[this.props.value] : this.props.text;
        this.parameterIndex = 0;
        this.vscode = null;
    }

    pointerDown(evt) {
        if (this.props.active === 0) {
            return '';
        }

        
        this.isOpen = true;//!this.isOpen;
        console.log("Pointer down", this.isOpen);
        this.isMouseInside = true;
        CabbageUtils.updateInnerHTML(this.props.channel, this);
    }

    handleItemClick(item) {
        this.selectedItem = item;
        const items = this.props.items.split(",");
        const index = items.indexOf(this.selectedItem);
        const normalValue = CabbageUtils.map(index, 0, items.length, 0, 1);
        const msg = { paramIdx:this.parameterIndex, channel: this.props.channel, value: normalValue }
        Cabbage.sendParameterUpdate(this.vscode, msg);

        this.isOpen = false;
        const widgetDiv = CabbageUtils.getWidgetDiv(this.props.channel);
        widgetDiv.style.transform = 'translate(' + this.props.left + 'px,' + this.props.top + 'px)';
        CabbageUtils.updateInnerHTML(this.props.channel, this);
    }

    addVsCodeEventListeners(widgetDiv, vs) {
        this.vscode = vs;
        widgetDiv.addEventListener("pointerdown", this.pointerDown.bind(this));
        document.body.addEventListener("click", this.handleClickOutside.bind(this));
        widgetDiv.ComboBoxInstance = this;
    }

    addEventListeners(widgetDiv) {
        widgetDiv.addEventListener("pointerdown", this.pointerDown.bind(this));
        document.body.addEventListener("click", this.handleClickOutside.bind(this));
        widgetDiv.ComboBoxInstance = this;
    }

    handleClickOutside(event) {
        const widgetDiv = CabbageUtils.getWidgetDiv(this.props.channel);
    
        if (!widgetDiv.contains(event.target)) {
            this.isOpen = false;
            widgetDiv.style.transform = 'translate(' + this.props.left + 'px,' + this.props.top + 'px)';
            CabbageUtils.updateInnerHTML(this.props.channel, this);
        }
    }

    getInnerHTML() {
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
        const fontSize = this.props.fontSize > 0 ? this.props.fontSize : this.props.height * 0.5;
    
        let totalHeight = this.props.height;
        const itemHeight = this.props.height * 0.8; // Scale back item height to 80% of the original height
        if (this.isOpen) {
            const items = this.props.items.split(",");
            totalHeight += items.length * itemHeight;
    
            // Check if the dropdown will be off the bottom of the screen
            const mainForm = CabbageUtils.getWidgetDiv("MainForm");
            const widgetDiv = mainForm.querySelector(`#${this.props.channel}`);
            const widgetRect = widgetDiv.getBoundingClientRect();
            const mainFormRect = mainForm.getBoundingClientRect();
            const spaceBelow = mainFormRect.bottom - widgetRect.bottom;
    
            if (spaceBelow < totalHeight) {
                const adjustment = totalHeight - this.props.height * 2; // Adding 10px for some padding
                const currentTopValue = parseInt(widgetDiv.style.top, 10) || this.props.top; // Use props.top if style.top is not set
                const newTopValue = currentTopValue - adjustment;
                widgetDiv.style.transform = 'translate(' + this.props.left + 'px,' + newTopValue + 'px)';
            }
        }
    
        let dropdownItems = "";
        if (this.isOpen) {
            const items = this.props.items.split(",");
            items.forEach((item, index) => {
                dropdownItems += `
                    <rect x="0" y="${index * itemHeight}" width="${this.props.width}" height="${itemHeight}"
                        fill="${CabbageColours.darker(this.props.colour, 0.2)}" rx="0" ry="0"
                        style="cursor: pointer;" pointer-events="all" data-item="${item}"
                        onmouseover="this.setAttribute('fill', '${CabbageColours.lighter(this.props.colour, 0.2)}')"
                        onmouseout="this.setAttribute('fill', '${CabbageColours.darker(this.props.colour, 0.2)}')"></rect>
                    <text x="${(this.props.width - this.props.corners / 2) / 2}" y="${(index + 1) * itemHeight - itemHeight / 2}"
                        font-family="${this.props.fontFamily}" font-size="${this.props.fontSize}" fill="${this.props.fontColour}"
                        text-anchor="middle" alignment-baseline="middle" data-item="${item}"
                        style="cursor: pointer;" pointer-events="all"
                        onmouseover="this.previousElementSibling.setAttribute('fill', '${CabbageColours.lighter(this.props.colour, 0.2)}')"
                        onmouseout="this.previousElementSibling.setAttribute('fill', '${CabbageColours.darker(this.props.colour, 0.2)}')"
                        onmousedown="document.getElementById('${this.props.channel}').ComboBoxInstance.handleItemClick('${item}')">${item}</text>
                `;
            });
        }
    
        const arrowWidth = 10; // Width of the arrow
        const arrowHeight = 6; // Height of the arrow
        const arrowX = this.props.width - arrowWidth - this.props.corners / 2 - 10; // Decreasing arrowX value to move the arrow more to the left
        const arrowY = (this.props.height - arrowHeight) / 2; // Y-coordinate of the arrow
    
        let selectedItemTextX;
        if (svgAlign === 'middle') {
            selectedItemTextX = (this.props.width - arrowWidth - this.props.corners / 2) / 2;
        } else {
            const selectedItemWidth = CabbageUtils.getStringWidth(this.selectedItem, this.props);
            const textPadding = svgAlign === 'start' ? - this.props.width * .1 : - this.props.width * .05;
            selectedItemTextX = svgAlign === 'start' ? (this.props.width - this.props.corners / 2) / 2 - selectedItemWidth / 2 + textPadding : (this.props.width - this.props.corners / 2) / 2 + selectedItemWidth / 2 + textPadding;
        }
        const selectedItemTextY = this.props.height / 2;
    
        return `
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${this.props.width} ${totalHeight}" width="${this.props.width}" height="${totalHeight}" preserveAspectRatio="none">
                <rect x="${this.props.corners / 2}" y="${this.props.corners / 2}" width="${this.props.width - this.props.corners}" height="${this.props.height - this.props.corners * 2}" fill="${this.props.colour}" stroke="${this.props.outlineColour}"
                    stroke-width="${this.props.outlineWidth}" rx="${this.props.corners}" ry="${this.props.corners}" 
                    style="cursor: pointer;" pointer-events="all" 
                    onmousedown="document.getElementById('${this.props.channel}').ComboBoxInstance.pointerDown(event)"></rect>
                ${dropdownItems}
                <polygon points="${arrowX},${arrowY} ${arrowX + arrowWidth},${arrowY} ${arrowX + arrowWidth / 2},${arrowY + arrowHeight}"
                    fill="${this.props.outlineColour}" style="${this.isOpen ? 'display: none;' : ''} pointer-events: none;"/>
                <text x="${selectedItemTextX}" y="${selectedItemTextY}" font-family="${this.props.fontFamily}" font-size="${fontSize}"
                    fill="${this.props.fontColour}" text-anchor="${svgAlign}" alignment-baseline="middle" style="${this.isOpen ? 'display: none;' : ''}"
                    style="pointer-events: none;">${this.selectedItem}</text>
            </svg>
        `;
    }
}
