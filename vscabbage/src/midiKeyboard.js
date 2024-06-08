import { Cabbage } from "./cabbagePluginMethods.js";

/**
 * Form class
 */
export class MidiKeyboard {
  constructor() {
    this.props = {
      "top": 0,
      "left": 0,
      "width": 600,
      "height": 300,
      "caption": "",
      "name": "MidiKeyboard",
      "type": "keyboard",
      "colour": "#888888",
      "channel": "keyboard",
      "blackNoteColour": "#000",
      "keySeparatorColour": "#f00",
      "arrowBackgroundColour": "#0295cf",
      "mouseoverKeyColour": "#93D200",
      "keydownColour": "#93D200",
      "arrowColour": "#00f",
    };

    this.panelSections = {
      "Properties": ["type"],
      "Bounds": ["width", "height"],
      "Colours": ["colour"]
    };

    this.isMouseDown = false; // Track the state of the mouse button
    this.octaveOffset = 3;
    this.noteMap = {};
    const octaveCount = 6; // Adjust this value as needed

    // Define an array of note names
    const noteNames = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];

    // Loop through octaves and note names to populate the map
    for (let octave = -2; octave <= octaveCount; octave++) {
      for (let i = 0; i < noteNames.length; i++) {
        const noteName = noteNames[i] + octave;
        const midiNote = (octave + 2) * 12 + i; // Calculate MIDI note number
        console.log(noteName, midiNote);
        this.noteMap[noteName] = midiNote;
      }
    }
  }

  pointerDown(e) {
    if (e.target.classList.contains('white-key') || e.target.classList.contains('black-key')) {
      this.isMouseDown = true;
      e.target.setAttribute('fill', this.props.keydownColour);
      console.log(`Key down: ${this.noteMap[e.target.dataset.note]}`);
      console.log(`Key down: ${e.target.dataset.note}`);
      Cabbage.sendMidiMessageFromUI(0x90, this.noteMap[e.target.dataset.note], 127);
    }
  }


  pointerUp(e) {
    if (this.isMouseDown) {
      this.isMouseDown = false;
      if (e.target.classList.contains('white-key') || e.target.classList.contains('black-key')) {
        e.target.setAttribute('fill', e.target.classList.contains('white-key') ? 'white' : 'black');
        console.log(`Key up: ${this.noteMap[e.target.dataset.note]}`);
        Cabbage.sendMidiMessageFromUI(0x80, this.noteMap[e.target.dataset.note], 127);
      }
    }
  }

  pointerMove(e) {
    if (this.isMouseDown && (e.target.classList.contains('white-key') || e.target.classList.contains('black-key'))) {
      e.target.setAttribute('fill', this.props.mouseoverKeyColour);
      console.log(`Key move: ${this.noteMap[e.target.dataset.note]}`);
    }
  }

  pointerEnter(e) {
    if (this.isMouseDown && (e.target.classList.contains('white-key') || e.target.classList.contains('black-key'))) {
      e.target.setAttribute('fill', this.props.mouseoverKeyColour);
      console.log(`Key enter: ${this.noteMap[e.target.dataset.note]}`);
    }
  }

  pointerLeave(e) {
    if (this.isMouseDown && (e.target.classList.contains('white-key') || e.target.classList.contains('black-key'))) {
      e.target.setAttribute('fill', e.target.classList.contains('white-key') ? 'white' : 'black');
    }
  }

  octaveUpPointerDown(e) {
    console.log('octaveUpPointerDown');
  }

  octaveDownPointerDown(e) {
    console.log('octaveDownPointerDown');
  }

  changeOctave(offset) {
    this.octaveOffset += offset;
    if (this.octaveOffset < 1) this.octaveOffset = 1; // Limit lower octave bound
    if (this.octaveOffset > 7) this.octaveOffset = 7; // Limit upper octave bound
    document.getElementById(this.props.name).innerHTML = this.getSVG(); // Update the SVG with new octave
  }

  addVsCodeEventListeners(widgetDiv, vscode) {
    this.vscode = vscode;
    this.addListeners(widgetDiv)
    widgetDiv.innerHTML = this.getSVG();
  }

  addEventListeners(widgetDiv) {
    this.addListeners(widgetDiv)
    widgetDiv.innerHTML = this.getSVG();
  }

  midiMessageListener(event) {
    console.log("Midi message listener");
    const detail = event.detail;
    const midiData = JSON.parse(detail.data);
    console.log("Midi message listener", midiData.status);
    if(midiData.status == 144) {
      const note = midiData.data1;
      // const velocity = midiData.data2;
      const noteName = Object.keys(this.noteMap).find(key => this.noteMap[key] === note);
      const key = document.querySelector(`[data-note="${noteName}"]`);
      key.setAttribute('fill', this.props.keydownColour);
      console.log(`Key down: ${note} ${noteName}`);
    }
    else if(midiData.status == 128) {
      const note = midiData.data1;
      const noteName = Object.keys(this.noteMap).find(key => this.noteMap[key] === note);
      const key = document.querySelector(`[data-note="${noteName}"]`);
      key.setAttribute('fill', key.classList.contains('white-key') ? 'white' : 'black');
      console.log(`Key up: ${note} ${noteName}`);
    }
  }

  addListeners(widgetDiv){
    widgetDiv.addEventListener("pointerdown", this.pointerDown.bind(this));
    widgetDiv.addEventListener("pointerup", this.pointerUp.bind(this));
    widgetDiv.addEventListener("pointermove", this.pointerMove.bind(this));
    widgetDiv.addEventListener("pointerenter", this.pointerEnter.bind(this), true);
    widgetDiv.addEventListener("pointerleave", this.pointerLeave.bind(this), true);
    document.addEventListener("midiEvent", this.midiMessageListener.bind(this));
    widgetDiv.OctaveButton = this;
  }

  handleClickEvent(e) {
    if (e.target.id == "octave-up") {
      this.changeOctave(1);
    }
    else {
      this.changeOctave(-1);
    }
  }

  getSVG() {
    const whiteKeyWidth = this.props.width / 21; // 21 white keys in 3 octaves
    const whiteKeyHeight = this.props.height;
    const blackKeyWidth = whiteKeyWidth * 0.4;
    const blackKeyHeight = this.props.height * 0.6;
    const strokeWidth = .5;

    const whiteKeys = ['C', 'D', 'E', 'F', 'G', 'A', 'B'];
    const blackKeys = { 'C': 'C#', 'D': 'D#', 'F': 'F#', 'G': 'G#', 'A': 'A#' };

    let whiteSvgKeys = '';
    let blackSvgKeys = '';

    for (let octave = 0; octave < 3; octave++) {
      for (let i = 0; i < whiteKeys.length; i++) {
        const key = whiteKeys[i];
        const note = key + (octave + this.octaveOffset);
        const width = whiteKeyWidth - strokeWidth;
        const height = whiteKeyHeight - strokeWidth;
        const xOffset = octave * whiteKeys.length * whiteKeyWidth + i * whiteKeyWidth;

        whiteSvgKeys += `<rect x="${xOffset}" y="0" width="${width}" height="${height}" fill="white" stroke="black" stroke-width="${strokeWidth}" data-note="${note}" class="white-key" style="height: ${whiteKeyHeight}px;" />`;

        if (blackKeys[key]) {
          const note = blackKeys[key] + (octave + this.octaveOffset);
          blackSvgKeys += `<rect x="${xOffset + whiteKeyWidth * 0.75 - strokeWidth / 2}" y="${strokeWidth / 2}" width="${blackKeyWidth}" height="${blackKeyHeight + strokeWidth}" fill="black" stroke="black" stroke-width="${strokeWidth}" data-note="${note}" class="black-key" />`;
      }

        if (i === 0) { // First white key of the octave
          const textX = xOffset + whiteKeyWidth / 2; // Position text in the middle of the white key
          const textY = whiteKeyHeight * .8; // Position text in the middle vertically
          whiteSvgKeys += `<text x="${textX}" y="${textY}" text-anchor="middle" dominant-baseline="middle" font-size="${whiteKeyHeight / 5}" fill="black" style="pointer-events: none;">${note}</text>`;
        }
      }
    }


    // Calculate button width and height relative to keyboard width
    const buttonWidth = this.props.width / 20;
    const buttonHeight = this.props.height;

    return `
        <div id="${this.props.channel}" style="display: flex; align-items: center; height: ${this.props.height}px;">
        <button id="octave-down" style="width: ${buttonWidth}px; height: ${buttonHeight}px; background-color: ${this.props.arrowBackgroundColour};" onclick="document.getElementById('${this.props.name}').OctaveButton.handleClickEvent(event)">-</button>
            <div id="keyboard" style="flex-grow: 1; height: 100%;">
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${this.props.width} ${this.props.height}" width="100%" height="100%" preserveAspectRatio="none">
                    ${whiteSvgKeys}
                    ${blackSvgKeys}
                </svg>
            </div>
            <button id="octave-up" style="width: ${buttonWidth}px; height: ${buttonHeight}px; background-color: ${this.props.arrowBackgroundColour};" onclick="document.getElementById('${this.props.name}').OctaveButton.handleClickEvent(event)">+</button>

        </div>
    `;
  }



}
