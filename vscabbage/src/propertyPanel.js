export class PropertyPanel {
  constructor(type, properties, panelSections) {
    this.type = type;
    this.panelSections = panelSections;
    var panel = document.querySelector('.property-panel');

    // Helper function to create a section
    const createSection = (sectionName) => {
      const sectionDiv = document.createElement('div');
      sectionDiv.classList.add('property-section');

      const header = document.createElement('h3');
      header.textContent = sectionName;
      sectionDiv.appendChild(header);

      return sectionDiv;
    };

    // Create sections based on the panelSections object
    const sections = {};

    if (panelSections === undefined)
      return;

    Object.entries(panelSections).forEach(([sectionName, keys]) => {
      sections[sectionName] = createSection(sectionName);
    });

    // Create a section for Miscellaneous properties
    const miscSection = createSection('Misc');
    sections['Misc'] = miscSection;

    // List of popular online fonts
    const fontList = [
      'Arial', 'Verdana', 'Helvetica', 'Tahoma', 'Trebuchet MS',
      'Times New Roman', 'Georgia', 'Garamond', 'Courier New',
      'Brush Script MT', 'Comic Sans MS', 'Impact', 'Lucida Sans',
      'Palatino', 'Century Gothic', 'Bookman', 'Candara', 'Consolas'
    ];

    // Helper function to create an input element based on the property key
    const createInputElement = (key, value) => {
      let input;

      if (key.toLowerCase().includes("colour")) {
        input = document.createElement('input');
        input.type = 'color';
        input.value = value;
      } else if (key.toLowerCase().includes("family")) {
        input = document.createElement('select');
        fontList.forEach((font) => {
          const option = document.createElement('option');
          option.value = font;
          option.textContent = font;
          if (font === 'Verdana') {
            option.selected = true;  // Set Verdana as default
          }
          input.appendChild(option);
        });
        input.value = value || 'Verdana';
      } else if (key.toLowerCase() === 'align') {
        input = document.createElement('select');
        const alignments = ['left', 'right', 'centre'];
        alignments.forEach((align) => {
          const option = document.createElement('option');
          option.value = align;
          option.textContent = align;
          if (align === 'centre') {
            option.selected = true;  // Set centre as default
          }
          input.appendChild(option);
        });
        input.value = value || 'centre';
      } else {
        input = document.createElement('input');
        input.type = 'text';
        input.value = `${value}`;

        // Set the input to readonly if the key is "type"
        if (key.toLowerCase() === 'type') {
          input.readOnly = true;
        }
      }

      return input;
    };

    // Iterate over panelSections and properties to assign them to their respective sections in order
    Object.entries(panelSections).forEach(([sectionName, keys]) => {
      keys.forEach((key) => {
        if (properties.hasOwnProperty(key)) {
          var propertyDiv = document.createElement('div');
          propertyDiv.classList.add('property');

          var label = document.createElement('label');
          let text = `${key}`;

          let result = text.replace(/([A-Z])/g, " $1");
          const separatedName = result.charAt(0).toUpperCase() + result.slice(1);
          label.textContent = separatedName;
          propertyDiv.appendChild(label);

          var input = createInputElement(key, properties[key]);

          input.id = key;
          input.dataset.parent = properties.name;

          input.addEventListener('input', function (evt) {
            widgets.forEach((widget) => {
              if (widget.props.name === evt.target.dataset.parent) {
                const inputValue = evt.target.value;
                let parsedValue;

                // Check if the input value can be parsed to a number
                if (!isNaN(inputValue) && inputValue.trim() !== "") {
                  parsedValue = Number(inputValue);
                } else {
                  parsedValue = inputValue;
                }
                widget.props[evt.target.id] = parsedValue;
                const widgetDiv = document.getElementById(widget.props.name);
                widgetDiv.innerHTML = widget.getSVG();
                vscode.postMessage({
                  command: 'widgetUpdate',
                  text: JSON.stringify(widget.props)
                });
              }
            });
          });

          propertyDiv.appendChild(input);
          sections[sectionName].appendChild(propertyDiv);
        }
      });
    });

    // Append sections to the panel in the specified order
    Object.keys(panelSections).forEach((sectionName) => {
      if (sections[sectionName].childNodes.length > 1) {
        panel.appendChild(sections[sectionName]);
      }
    });

    // Append the Misc section last
    if (sections['Misc'].childNodes.length > 1) {
      panel.appendChild(sections['Misc']);
    }
  }

  /**
* this callback is triggered whenever a user move/drags a widget in edit mode
* The innerHTML is constantly updated. When this is called, the editor is also
* updated accordingly. It accepts an array of object with details about the event
* type, name and bounds updates 
*/
  static async updatePanel(vscode, input, widgets) {
    // Ensure input is an array of objects
    let events = Array.isArray(input) ? input : [input];

    const element = document.querySelector('.property-panel');
    if (element) {
      element.style.visibility = "visible";
      element.innerHTML = '';
    }

    // Iterate over the array of event objects
    events.forEach(eventObj => {
      const { eventType, name, bounds } = eventObj;
      widgets.forEach((widget, index) => {
        if (widget.props.name === name) {
          if (eventType !== 'click') {
            widget.props.left = Math.floor(bounds.x);
            widget.props.top = Math.floor(bounds.y);
            widget.props.width = Math.floor(bounds.w);
            widget.props.height = Math.floor(bounds.h);

            if (widget.props.type !== 'form') {
              document.getElementById(widget.props.name).innerHTML = widget.getSVG();
            }
          }

          if (widget.props.hasOwnProperty('channel')) {
            widget.props.channel = name;
          }

          new PropertyPanel(widget.props.type, widget.props, widget.panelSections);

          //firing these off in one go cause the vs-code editor to shit its pant
          setTimeout(() => {
            vscode.postMessage({
              command: 'widgetUpdate',
              text: JSON.stringify(widget.props)
            });
          }, (index + 1) * 50);
        }
      });
    });
  }
}
