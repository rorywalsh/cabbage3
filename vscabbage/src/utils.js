// utils.js

/**
 * This uses a simple regex pattern to parse a line of Cabbage code such as 
 * rslider bounds(22, 14, 60, 60) channel("clip") thumbRadius(5), text("Clip") range(0, 1, 0, 1, 0.001)
 * and converts it to a JSON object
 */
export function getCabbageCodeAsJSON(text) {
    const regex = /(\w+)\(([^)]+)\)/g;
    const jsonObj = {};

    let match;
    while ((match = regex.exec(text)) !== null) {
        const name = match[1];
        let value = match[2].replace(/"/g, ''); // Remove double quotes

        if (name === 'bounds') {
            // Splitting the value into individual parts for top, left, width, and height
            const [left, top, width, height] = value.split(',').map(v => parseInt(v.trim()));
            jsonObj['left'] = left;
            jsonObj['top'] = top;
            jsonObj['width'] = width;
            jsonObj['height'] = height;
        }
        else if (name === 'range') {
            // Splitting the value into individual parts for top, left, width, and height
            const [min, max, initValue, skew, increment] = value.split(',').map(v => parseFloat(v.trim()));
            jsonObj['min'] = min;
            jsonObj['max'] = max;
            jsonObj['value'] = initValue;
            jsonObj['skew'] = skew;
            jsonObj['increment'] = increment;
        }
        else if (name === 'size') {
            // Splitting the value into individual parts for width and height
            const [width, height] = value.split(',').map(v => parseInt(v.trim()));
            jsonObj['width'] = width;
            jsonObj['height'] = height;
        } else {
            // Check if the value is a number
            const numericValue = parseFloat(value);
            if (!isNaN(numericValue)) {
                // If it's a number, assign it as a number
                jsonObj[name] = numericValue;
            } else {
                // If it's not a number, assign it as a string
                jsonObj[name] = value;
            }
        }
    }

    return jsonObj;
}

/**
 * this function parses the Cabbage code and creates new widgets accordingly..
 */
export function parseCabbageCode(text, widgets, form, insertWidget) {
    // Leave main form in the widget array - there is only one..
    widgets.splice(1, widgets.length - 1);

    let cabbageStart = 0;
    let cabbageEnd = 0;
    let lines = text.split(/\r?\n/);
    let count = 0;

    lines.forEach((line) => {
        if (line.trimStart().startsWith("<Cabbage>"))
            cabbageStart = count + 1;
        else if (line.trimStart().startsWith("</Cabbage>"))
            cabbageEnd = count;
        count++;
    })

    const cabbageCode = lines.slice(cabbageStart, cabbageEnd);
    cabbageCode.forEach(async (line) => {
        const codeProps = getCabbageCodeAsJSON(line);
        const type = `${line.trimStart().split(' ')[0]}`;
        if (line.trim() != "") {
            if (type != "form") {
                await insertWidget(type, codeProps);
            } else {
                widgets.forEach((widget) => {
                    if (widget.props.name == "MainForm") {
                        const w = codeProps.width;
                        const h = codeProps.height;
                        form.style.width = w + "px";
                        form.style.height = h + "px";
                        widget.props.width = w;
                        widget.props.width = h;
                    }
                });
            }
        }
    });
}

/**
* this function will return the number of plugin parameter in our widgets array
*/
export function getNumberOfPluginParameters(widgets, ...types) {
    // Create a set from the types for faster lookup
    const typeSet = new Set(types);

    // Initialize the counter
    let count = 0;

    // Iterate over each widget in the array
    for (const widget of widgets) {
        // Check if the widget's type is one of the specified types
        if (typeSet.has(widget.props.type)) {
            // Increment the counter if the type matches
            count++;
        }
    }

    // Return the final count
    return count;
}

export function showOverlay() {
    document.getElementById('fullScreenOverlay').style.display = 'flex';
    const leftPanel = document.getElementById('LeftPanel');
    const rightPanel = document.getElementById('RightPanel');
    leftPanel.style.display = 'none';
    rightPanel.style.display = 'none';
}

export function hideOverlay() {
    document.getElementById('fullScreenOverlay').style.display = 'none';
    const leftPanel = document.getElementById('LeftPanel');
    const rightPanel = document.getElementById('RightPanel');
    leftPanel.style.display = 'flex';
    rightPanel.style.display = 'flex';
}