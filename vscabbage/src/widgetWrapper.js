export class WidgetWrapper {
    constructor(updatePanelCallback, selectedSet) {
        const restrictions = {
            restriction: 'parent',
            endOnly: true
        };
        let selectedElements = selectedSet;
        let initialPointerX, initialPointerY;

        function dragMoveListener(event) {
            if (event.shiftKey || event.altKey) {
                // Prevent dragging when Shift or Alt is pressed
                return;
            }

            const { dx, dy } = event;

            // Apply translation to each selected element
            selectedElements.forEach(element => {
                const x = (parseFloat(element.getAttribute('data-x')) || 0) + dx;
                const y = (parseFloat(element.getAttribute('data-y')) || 0) + dy;

                element.style.transform = `translate(${x}px, ${y}px)`;
                // Update data-x and data-y attributes
                element.setAttribute('data-x', x);
                element.setAttribute('data-y', y);

                // Update panel callback for resize
                // updatePanelCallback("resize", element.id, { x: x, y: y, w: element.offsetWidth, h: element.offsetHeight });
            });
        }

        // Reset initial pointer position when drag operation ends
        function dragEndListener(event) {
            const { dx, dy } = event;
            selectedElements.forEach(element => {
                const x = (parseFloat(element.getAttribute('data-x')) || 0) + dx;
                const y = (parseFloat(element.getAttribute('data-y')) || 0) + dy;

                element.style.transform = `translate(${x}px, ${y}px)`;
                // Update data-x and data-y attributes
                element.setAttribute('data-x', x);
                element.setAttribute('data-y', y);
                updatePanelCallback("resize", element.id, { x: x, y: y, w: element.offsetWidth, h: element.offsetHeight });
            });

            initialPointerX = null;
            initialPointerY = null;
        }

        interact('.draggable')
            .resizable({
                edges: { left: true, right: true, bottom: true, top: true },
                listeners: {
                    move(event) {
                        if (event.shiftKey || event.altKey) {
                            // Prevent resizing when Shift or Alt is pressed
                            return;
                        }
                        var target = event.target
                        restrictions.restriction = (target.id === 'MainForm' ? 'none' : 'parent');
                        var x = (parseFloat(target.getAttribute('data-x')) || 0)
                        var y = (parseFloat(target.getAttribute('data-y')) || 0)

                        target.style.width = event.rect.width + 'px'
                        target.style.height = event.rect.height + 'px'

                        x += event.deltaRect.left
                        y += event.deltaRect.top

                        updatePanelCallback("resize", event.target.id, { x: x, y: y, w: event.rect.width, h: event.rect.height });

                        target.style.transform = 'translate(' + x + 'px,' + y + 'px)'

                        target.setAttribute('data-x', x)
                        target.setAttribute('data-y', y)
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
                    move: dragMoveListener,
                    end: dragEndListener
                },
                inertia: true,
                modifiers: [
                    interact.modifiers.restrictRect(restrictions),
                ]
            }).on('down', function (event) {
                if (event.target.id) { //form
                    updatePanelCallback("click", event.target.id, {});
                }
                else { //all widgets placed on form
                    const widgetId = event.target.parentElement.parentElement.id.replace(/(<([^>]+)>)/ig);
                    updatePanelCallback("click", widgetId, {});
                }
            })
    }
}
