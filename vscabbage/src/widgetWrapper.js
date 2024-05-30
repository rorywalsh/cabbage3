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

        interact('.draggable').on('down', (event) => {
            if (event.target.id) {
                updatePanelCallback("click", event.target.id, {});
            } else {
                const widgetId = event.target.parentElement.parentElement.id.replace(/(<([^>]+)>)/ig, '');
                updatePanelCallback("click", widgetId, {});
            }
        });
    }

    dragMoveListener(event) {
        if (event.shiftKey || event.altKey) {
            return;
        }

        const { dx, dy } = event;
        this.selectedElements.forEach(element => {
            const x = (parseFloat(element.getAttribute('data-x')) || 0) + dx;
            const y = (parseFloat(element.getAttribute('data-y')) || 0) + dy;

            element.style.transform = `translate(${x}px, ${y}px)`;
            element.setAttribute('data-x', x);
            element.setAttribute('data-y', y);
        });
    }

    dragEndListener(event) {
        const { dx, dy } = event;
        this.selectedElements.forEach(element => {
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

        interact('.draggable')
            .resizable({
                edges: { left: true, right: true, bottom: true, top: true },
                listeners: {
                    move: (event) => {
                        if (event.shiftKey || event.altKey) {
                            return;
                        }
                        const target = event.target;
                        restrictions.restriction = (target.id === 'MainForm' ? 'none' : 'parent');
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
                    })
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
    }

    setSnapSize(size) {
        this.snapSize = size;
        this.applyInteractConfig({
            restriction: 'parent',
            endOnly: true
        });
    }
}
