import { Cabbage } from "../cabbage.js";
import { CabbageUtils, CabbageColours } from "../utils.js";

/**
 * Form class
 */
export class MidiKeyboard {
  constructor() {
    this.props = {
      "top": 0, // Top position of the keyboard widget
      "left": 0, // Left position of the keyboard widget
      "width": 600, // Width of the keyboard widget
      "height": 300, // Height of the keyboard widget
      "type": "keyboard", // Type of the widget (keyboard)
      "colour": "#888888", // Background color of the keyboard
      "channel": "keyboard", // Unique identifier for the keyboard widget
      "blackNoteColour": "#000", // Color of the black keys on the keyboard
      "value": "36", // The leftmost note of the keyboard
      "fontFamily": "Verdana", // Font family for the text displayed on the keyboard
      "whiteNoteColour": "#fff", // Color of the white keys on the keyboard
      "keySeparatorColour": "#000", // Color of the separators between keys
      "arrowBackgroundColour": "#0295cf", // Background color of the arrow keys
      "mouseoverKeyColour": CabbageColours.getColour('green'), // Color of keys when hovered over
      "keydownColour": CabbageColours.getColour('green'), // Color of keys when pressed
      "octaveButtonColour": "#00f", // Color of the octave change buttons
  };
  

    this.panelSections = {
      "Properties": ["type"],
      "Bounds": ["left", "top", "width", "height"],
      "Text": ["fontFamily"],
      "Colours": ["colour", "blackNoteColour", "whiteNoteColour", "keySeparatorColour", "arrowBackgroundColour", "keydownColour", "octaveButtonColour"]
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
        e.target.setAttribute('fill', e.target.classList.contains('white-key') ? this.props.whiteNoteColour : this.props.blackNoteColour);
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
      e.target.setAttribute('fill', e.target.classList.contains('white-key') ? this.props.whiteNoteColour : this.props.blackNoteColour);
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
    CabbageUtils.updateInnerHTML(this.props.channel, this);
  }

  addVsCodeEventListeners(widgetDiv, vscode) {
    this.vscode = vscode;
    this.addListeners(widgetDiv)
    CabbageUtils.updateInnerHTML(this.props.channel, this);
  }

  addEventListeners(widgetDiv) {
    this.addListeners(widgetDiv)
    CabbageUtils.updateInnerHTML(this.props.channel, this);
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

  getInnerHTML() {
    const scaleFactor = 0.9; // Adjusting this to fit the UI designer bounding rect
  
    const whiteKeyWidth = (this.props.width / 21) * scaleFactor; 
    const whiteKeyHeight = this.props.height * scaleFactor;
    const blackKeyWidth = whiteKeyWidth * 0.4;
    const blackKeyHeight = whiteKeyHeight * 0.6;
    const strokeWidth = 0.5 * scaleFactor;
  
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
  
        whiteSvgKeys += `<rect x="${xOffset}" y="0" width="${width}" height="${height}" fill="${this.props.whiteNoteColour}" stroke="${this.props.keySeparatorColour}" stroke-width="${strokeWidth}" data-note="${note}" class="white-key" style="height: ${whiteKeyHeight}px;" />`;
  
        if (blackKeys[key]) {
          const note = blackKeys[key] + (octave + this.octaveOffset);
          blackSvgKeys += `<rect x="${xOffset + whiteKeyWidth * 0.75 - strokeWidth / 2}" y="${strokeWidth / 2}" width="${blackKeyWidth}" height="${blackKeyHeight + strokeWidth}" fill="${this.props.blackNoteColour}" stroke="${this.props.keySeparatorColour}"  stroke-width="${strokeWidth}" data-note="${note}" class="black-key" />`;
        }
  
        if (i === 0) { // First white key of the octave
          const textX = xOffset + whiteKeyWidth / 2; // Position text in the middle of the white key
          const textY = whiteKeyHeight * 0.8; // Position text in the middle vertically
          whiteSvgKeys += `<text x="${textX}" y="${textY}" text-anchor="middle"  font-family="${this.props.fontFamily}" dominant-baseline="middle" font-size="${whiteKeyHeight / 5}" fill="${this.props.blackNoteColour}" style="pointer-events: none;">${note}</text>`;
        }
      }
    }
  
    // Calculate button width and height relative to keyboard width
    const buttonWidth = (this.props.width / 20) * scaleFactor;
    const buttonHeight = this.props.height * scaleFactor;
  
    return `
      <div id="${this.props.channel}" style="display: flex; align-items: center; height: ${this.props.height * scaleFactor}px;">
        <button id="octave-down" style="width: ${buttonWidth}px; height: ${buttonHeight}px; background-color: ${this.props.arrowBackgroundColour};" onclick="document.getElementById('${this.props.channel}').OctaveButton.handleClickEvent(event)">-</button>
        <div id="${this.props.channel}" style="flex-grow: 1; height: 100%;">
          <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${this.props.width * scaleFactor} ${this.props.height * scaleFactor}" width="100%" height="100%" preserveAspectRatio="none">
            ${whiteSvgKeys}
            ${blackSvgKeys}
          </svg>
        </div>
        <button id="octave-up" style="width: ${buttonWidth}px; height: ${buttonHeight}px; background-color: ${this.props.arrowBackgroundColour};" onclick="document.getElementById('${this.props.channel}').OctaveButton.handleClickEvent(event)">+</button>
      </div>
    `;
  }
  



}
