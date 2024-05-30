export class WidgetWrapper {
    constructor(updatePanelCallback, selectedSet) {
        const restrictions = {
            restriction: 'parent',
            endOnly: true
        };
        this.snapSize = 4;
        this.selectedElements = selectedSet;
        this.updatePanelCallback = updatePanelCallback;
        this.dragMoveListener = this.dragMoveListener.bind(this);
        this.dragEndListener = this.dragEndListener.bind(this);

        this.applyInteractConfig(restrictions);
    }

    dragMoveListener(event) {
        if (event.shiftKey || event.altKey) {
            return;
        }

        const { dx, dy } = event;
        this.selectedElements.forEach(element => {
            console.log(`Dragging element ${element.id}: dx=${dx}, dy=${dy}`); // Logging drag details
            const x = (parseFloat(element.getAttribute('data-x')) || 0) + dx;
            const y = (parseFloat(element.getAttribute('data-y')) || 0) + dy;

            element.style.transform = `translate(${x}px, ${y}px)`;
            element.setAttribute('data-x', x);
            element.setAttribute('data-y', y);
        });
    }

    dragEndListener(event) {
        console.log("DRAG_END");
        const { dx, dy } = event;
        this.selectedElements.forEach(element => {
            console.log(`Drag ended for element ${element.id}: dx=${dx}, dy=${dy}`); // Logging drag end details
            const x = (parseFloat(element.getAttribute('data-x')) || 0) + dx;
            const y = (parseFloat(element.getAttribute('data-y')) || 0) + dy;

            element.style.transform = `translate(${x}px, ${y}px)`;
            element.setAttribute('data-x', x);
            element.setAttribute('data-y', y);
            this.updatePanelCallback("resize", element.id, { x: x, y: y, w: element.offsetWidth, h: element.offsetHeight });
        });
    }

    applyInteractConfig(restrictions) {
        interact('.draggable').unset(); // Unset previous interact configuration

        interact('.draggable').on('down', (event) => {
            if (event.target.id) {
                this.updatePanelCallback("click", event.target.id, {});
            } else {
                const widgetId = event.target.parentElement.parentElement.id.replace(/(<([^>]+)>)/ig, '');
                this.updatePanelCallback("click", widgetId, {});
            }
        }).resizable({
            edges: { left: true, right: true, bottom: true, top: true },
            listeners: {
                move: (event) => {
                    if (event.shiftKey || event.altKey) {
                        return;
                    }
                    const target = event.target;
                    restrictions.restriction = (target.id === 'MainForm' ? 'none' : 'parent');
                    console.log(`Restrictions applied to ${target.id}:`, restrictions); // Log restrictions
                    let x = (parseFloat(target.getAttribute('data-x')) || 0);
                    let y = (parseFloat(target.getAttribute('data-y')) || 0);

                    target.style.width = event.rect.width + 'px';
                    target.style.height = event.rect.height + 'px';

                    x += event.deltaRect.left;
                    y += event.deltaRect.top;

                    this.updatePanelCallback("resize", event.target.id, { x: x, y: y, w: event.rect.width, h: event.rect.height });

                    target.style.transform = 'translate(' + x + 'px,' + y + 'px)';

                    target.setAttribute('data-x', x);
                    target.setAttribute('data-y', y);
                }
            },
            modifiers: [
                interact.modifiers.restrictRect(restrictions),
                interact.modifiers.restrictSize({
                    min: { width: 100, height: 50 }
                }),
                interact.modifiers.snap({
                    targets: [
                        interact.snappers.grid({ x: this.snapSize, y: this.snapSize })
                    ],
                    range: Infinity,
                    relativePoints: [{ x: 0, y: 0 }]
                }),
            ],
            inertia: true
        }).draggable({
            listeners: {
                move: this.dragMoveListener,
                end: this.dragEndListener
            },
            inertia: true,
            modifiers: [
                interact.modifiers.snap({
                    targets: [
                        interact.snappers.grid({ x: this.snapSize, y: this.snapSize })
                    ],
                    range: Infinity,
                    relativePoints: [{ x: 0, y: 0 }]
                }),
                interact.modifiers.restrictRect(restrictions),
            ]
        });

        //main form only..........
        interact('.resizeOnly').on('down', (event) => {
            if (event.target.id) {
                this.updatePanelCallback("click", event.target.id, {});
            } else {
                const widgetId = event.target.parentElement.parentElement.id.replace(/(<([^>]+)>)/ig, '');
                this.updatePanelCallback("click", widgetId, {});
            }
        }).draggable(false).resizable({
            edges: { left: true, right: true, bottom: true, top: true }, // Enable resizing from all edges
            listeners: {
                move: (event) => {
                    if (event.shiftKey || event.altKey) {
                        return;
                    }
                    const target = event.target;
                    restrictions.restriction = (target.id === 'MainForm' ? 'none' : 'parent');
                    console.log(`Restrictions applied to ${target.id}:`, restrictions); // Log restrictions
                    let x = (parseFloat(target.getAttribute('data-x')) || 0);
                    let y = (parseFloat(target.getAttribute('data-y')) || 0);

                    target.style.width = event.rect.width + 'px';
                    target.style.height = event.rect.height + 'px';

                    x += event.deltaRect.left;
                    y += event.deltaRect.top;

                    this.updatePanelCallback("resize", event.target.id, { x: x, y: y, w: event.rect.width, h: event.rect.height });

                    target.style.transform = 'translate(' + x + 'px,' + y + 'px)';

                    target.setAttribute('data-x', x);
                    target.setAttribute('data-y', y);
                }
            },
            modifiers: [
                interact.modifiers.restrictSize({
                    min: { width: 50, height: 50 }, // Minimum size for the element
                    max: { width: 1500, height: 1500 } // Maximum size for the element
                })
            ],
            inertia: true
        });

        // Logging to verify MainForm is targeted
        console.log('applyInteractConfig called with restrictions:', restrictions);
        console.log('MainForm draggable configuration applied.');
    }

    setSnapSize(size) {
        this.snapSize = size;
        this.applyInteractConfig({
            restriction: 'parent',
            endOnly: true
        });
    }
}

/*
This is a simple panel that the main form sits on. It can be dragged around without restriction
*/
interact('.draggable-panel')
    .draggable({
        inertia: true,
        autoScroll: true,
        onmove: dragMoveListener
    });

function dragMoveListener(event) {
    var target = event.target;

    // keep the dragged position in the data-x/data-y attributes
    var x = (parseFloat(target.getAttribute('data-x')) || 0) + event.dx;
    var y = (parseFloat(target.getAttribute('data-y')) || 0) + event.dy;

    // translate the element
    target.style.webkitTransform =
        target.style.transform =
        'translate(' + x + 'px, ' + y + 'px)';

    // update the position attributes
    target.setAttribute('data-x', x);
    target.setAttribute('data-y', y);
}



