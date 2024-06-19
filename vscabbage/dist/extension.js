/******/ (() => { // webpackBootstrap
/******/ 	"use strict";
/******/ 	var __webpack_modules__ = ([
/* 0 */
/***/ (function(__unused_webpack_module, exports, __webpack_require__) {


var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || function (mod) {
    if (mod && mod.__esModule) return mod;
    var result = {};
    if (mod != null) for (var k in mod) if (k !== "default" && Object.prototype.hasOwnProperty.call(mod, k)) __createBinding(result, mod, k);
    __setModuleDefault(result, mod);
    return result;
};
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", ({ value: true }));
exports.activate = void 0;
// The module 'vscode' contains the VS Code extensibility API
// Import the module and reference it with the alias vscode in your code below
const vscode = __importStar(__webpack_require__(1));
// @ts-ignore
const rotarySlider_js_1 = __webpack_require__(2);
// @ts-ignore
const horizontalSlider_js_1 = __webpack_require__(4);
// @ts-ignore
const verticalSlider_js_1 = __webpack_require__(6);
// @ts-ignore
const button_js_1 = __webpack_require__(7);
// @ts-ignore
const checkbox_js_1 = __webpack_require__(8);
// @ts-ignore
const comboBox_js_1 = __webpack_require__(9);
// @ts-ignore
const label_js_1 = __webpack_require__(10);
// @ts-ignore
const csoundOutput_js_1 = __webpack_require__(11);
// @ts-ignore
const midiKeyboard_js_1 = __webpack_require__(12);
// @ts-ignore
const genTable_js_1 = __webpack_require__(39);
// @ts-ignore
const utils_js_1 = __webpack_require__(3);
// @ts-ignore
const form_js_1 = __webpack_require__(13);
const cp = __importStar(__webpack_require__(14));
let textEditor;
let output;
let panel = undefined;
const ws_1 = __importDefault(__webpack_require__(15));
const wss = new ws_1.default.Server({ port: 9991 });
let websocket;
let cabbageMode = "play";
wss.on('connection', (ws) => {
    console.log('Client connected');
    websocket = ws;
    ws.on('message', (message) => {
        const msg = JSON.parse(message.toString());
        // console.log(JSON.stringify(msg["widgetUpdate"], null, 2));
        if (panel) {
            panel.webview.postMessage({ command: "widgetUpdate", text: JSON.stringify(msg["widgetUpdate"]) });
        }
    });
    ws.on('close', () => {
        console.log('Client disconnected');
    });
});
let processes = [];
// This method is called when your extension is activated
// Your extension is activated the very first time the command is executed
function activate(context) {
    if (output === undefined) {
        output = vscode.window.createOutputChannel("Cabbage output");
    }
    output.clear();
    output.show(true); // true means keep focus in the editor window
    // Use the console to output diagnostic information (console.log) and errors (console.error)
    // This line of code will only be executed once when your extension is activated
    console.log('Congratulations, your extension "cabbage" is now active!');
    //send text to webview for parsing if file has an extension of csd and contains valid Cabbage tags
    function sendTextToWebView(editor, command) {
        if (editor) {
            if (editor.fileName.split('.').pop() === 'csd') {
                //reload the webview
                vscode.commands.executeCommand("workbench.action.webview.reloadWebviewAction");
                //now check for Cabbage tags..
                if (editor?.getText().indexOf('<Cabbage>') != -1 && editor?.getText().indexOf('</Cabbage>') != -1) {
                    if (panel)
                        panel.webview.postMessage({ command: command, text: editor?.getText() });
                }
            }
        }
    }
    context.subscriptions.push(vscode.commands.registerCommand('cabbage.launchUIEditor', () => {
        // The code you place here will be executed every time your command is executed
        panel = vscode.window.createWebviewPanel('cabbageUIEditor', 'Cabbage UI Editor', 
        //load in second column, I guess this could be controlled by settings
        vscode.ViewColumn.Two, {});
        const config = vscode.workspace.getConfiguration("cabbage");
        //makes sure the editor currently displayed has focus..
        vscode.commands.executeCommand('workbench.action.focusNextGroup');
        vscode.commands.executeCommand('workbench.action.focusPreviousGroup');
        //this is a little clunky, but it seems I have to load each src individually
        let onDiskPath = vscode.Uri.joinPath(context.extensionUri, 'src', 'main.js');
        const mainJS = panel.webview.asWebviewUri(onDiskPath);
        onDiskPath = vscode.Uri.joinPath(context.extensionUri, 'media', 'vscode.css');
        const styles = panel.webview.asWebviewUri(onDiskPath);
        onDiskPath = vscode.Uri.joinPath(context.extensionUri, 'media', 'cabbage.css');
        const cabbageStyles = panel.webview.asWebviewUri(onDiskPath);
        onDiskPath = vscode.Uri.joinPath(context.extensionUri, 'src', 'interact.min.js');
        const interactJS = panel.webview.asWebviewUri(onDiskPath);
        onDiskPath = vscode.Uri.joinPath(context.extensionUri, 'src', 'widgetWrapper.js');
        const widgetWrapper = panel.webview.asWebviewUri(onDiskPath);
        onDiskPath = vscode.Uri.joinPath(context.extensionUri, 'src', 'color-picker.js');
        const colourPickerJS = panel.webview.asWebviewUri(onDiskPath);
        onDiskPath = vscode.Uri.joinPath(context.extensionUri, 'src', 'color-picker.css');
        const colourPickerStyles = panel.webview.asWebviewUri(onDiskPath);
        //add widget types to menu
        const widgetTypes = ["hslider", "rslider", "gentable", "vslider", "keyboard", "button", "filebutton", "combobox", "checkbox", "keyboard", "csoundoutput"];
        let menuItems = "";
        widgetTypes.forEach((widget) => {
            menuItems += `
			<li class="menuItem">
			<span>${widget}</span>
	  		</li>
			`;
        });
        // set webview HTML content and options
        panel.webview.html = getWebviewContent(mainJS, styles, cabbageStyles, interactJS, widgetWrapper, colourPickerJS, colourPickerStyles, menuItems);
        panel.webview.options = { enableScripts: true };
        //assign current textEditor so we can track it even if focus changes to the webview
        panel.onDidChangeViewState(() => {
            textEditor = vscode.window.activeTextEditor;
        });
        vscode.workspace.onDidChangeTextDocument((editor) => {
            // sendTextToWebView(editor.document, 'onFileChanged');
        });
        //notify webview when various updates take place in editor
        vscode.workspace.onDidSaveTextDocument(async (editor) => {
            sendTextToWebView(editor, 'onFileChanged');
            cabbageMode = "play";
            const command = config.get("pathToCabbageExecutable") + '/CabbageApp.app/Contents/MacOS/CabbageApp';
            const path = vscode.Uri.file(command);
            try {
                // Attempt to read the directory (or file)
                await vscode.workspace.fs.stat(path);
                output.append("Found Cabbage service app...");
            }
            catch (error) {
                // If an error is thrown, it means the path does not exist
                output.append(`ERROR: Could not locate Cabbage service app at ${path.fsPath}. Please check the path in the Cabbage extension settings.\n`);
                return;
            }
            processes.forEach((p) => {
                p?.kill("SIGKILL");
            });
            // 	process.kill("SIGKILL");
            const process = cp.spawn(command, [editor.fileName], {});
            // currentPid = process.pid;
            processes.push(process);
            process.stdout.on("data", (data) => {
                // I've seen spurious 'ANSI reset color' sequences in some csound output
                // which doesn't render correctly in this context. Stripping that out here.
                output.append(data.toString().replace(/\x1b\[m/g, ""));
            });
        });
        vscode.workspace.onDidOpenTextDocument((editor) => {
            sendTextToWebView(editor, 'onFileChanged');
        });
        vscode.window.tabGroups.onDidChangeTabs((tabs) => {
            //triggered when tab changes
            //console.log(tabs.changed.label);
        });
        // callback for when users update widget properties in webview
        panel.webview.onDidReceiveMessage(message => {
            switch (message.command) {
                case 'widgetUpdate':
                    if (cabbageMode !== "play") {
                        updateText(message.text);
                    }
                    return;
                case 'channelUpdate':
                    if (websocket) {
                        websocket.send(JSON.stringify(message.text));
                    }
                // console.log(message.text);
                case 'ready': //trigger when webview is open
                    if (panel) {
                        panel.webview.postMessage({ command: "snapToSize", text: config.get("snapToSize") });
                    }
                    break;
            }
        }, undefined, context.subscriptions);
    }));
    context.subscriptions.push(vscode.commands.registerCommand('cabbage.editMode', () => {
        // The code you place here will be executed every time your command is executed
        // Place holder - but these commands will eventually launch Cabbage standalone 
        // with the file currently in focus. 
        if (!panel) {
            return;
        }
        const msg = { event: "stopCsound" };
        if (websocket) {
            websocket.send(JSON.stringify(msg));
        }
        processes.forEach((p) => {
            p?.kill("SIGKILL");
        });
        sendTextToWebView(textEditor?.document, 'onEnterEditMode');
        cabbageMode = "draggable";
    }));
}
exports.activate = activate;
/**
 * This function will update the text associated with a widget
 */
async function updateText(jsonText) {
    if (cabbageMode === "play") {
        return;
    }
    const props = JSON.parse(jsonText);
    // console.log(JSON.stringify(props, null, 2));
    if (textEditor) {
        const document = textEditor.document;
        let lineNumber = 0;
        //get default props so we can compare them to incoming ones and display any that are different
        let defaultProps = {};
        switch (props.type) {
            case 'rslider':
                defaultProps = new rotarySlider_js_1.RotarySlider().props;
                break;
            case 'hslider':
                defaultProps = new horizontalSlider_js_1.HorizontalSlider().props;
                break;
            case 'vslider':
                defaultProps = new verticalSlider_js_1.VerticalSlider().props;
                break;
            case 'keyboard':
                defaultProps = new midiKeyboard_js_1.MidiKeyboard().props;
                break;
            case 'button':
                defaultProps = new button_js_1.Button().props;
                break;
            case 'gentable':
                defaultProps = new genTable_js_1.GenTable().props;
                break;
            case 'filebutton':
                defaultProps = new FileButton().props;
                break;
            case 'checkbox':
                defaultProps = new checkbox_js_1.Checkbox().props;
                break;
            case 'combobox':
                defaultProps = new comboBox_js_1.ComboBox().props;
                break;
            case 'label':
                defaultProps = new label_js_1.Label().props;
                break;
            case 'form':
                defaultProps = new form_js_1.Form().props;
                break;
            case 'csoundoutput':
                defaultProps = new csoundOutput_js_1.CsoundOutput().props;
                break;
            default:
                break;
        }
        const internalIdentifiers = ['top', 'left', 'width', 'defaultValue', 'name', 'height', 'increment', 'min', 'max', 'skew', 'index'];
        if (props.type.indexOf('slider') !== -1) {
            internalIdentifiers.push('value');
        }
        await textEditor.edit(async (editBuilder) => {
            if (textEditor) {
                let foundChannel = false;
                let lines = document.getText().split(/\r?\n/);
                for (let i = 0; i < lines.length; i++) {
                    const tokens = utils_js_1.CabbageUtils.getTokens(lines[i]);
                    const index = tokens.findIndex(({ token }) => token === 'channel');
                    if (index != -1) {
                        const channel = tokens[index].values[0].replace(/"/g, "");
                        if (channel == props.channel) {
                            foundChannel = true;
                            //found entry - now update bounds
                            const updatedIdentifiers = utils_js_1.CabbageUtils.findUpdatedIdentifiers(JSON.stringify(defaultProps), jsonText);
                            updatedIdentifiers.forEach((ident) => {
                                // Only want to display user-accessible identifiers...
                                if (!internalIdentifiers.includes(ident)) {
                                    const newIndex = tokens.findIndex(({ token }) => token === ident);
                                    // Create the data array with the appropriate type based on props[ident]
                                    let data = [];
                                    if (Array.isArray(props[ident])) {
                                        data = [...props[ident]];
                                    }
                                    else {
                                        data.push(props[ident]);
                                    }
                                    if (newIndex === -1) {
                                        const identifier = ident;
                                        tokens.push({ token: identifier, values: data });
                                    }
                                    else {
                                        tokens[newIndex].values = data;
                                    }
                                }
                            });
                            if (props.type.indexOf('slider') > -1) {
                                const rangeIndex = tokens.findIndex(({ token }) => token === 'range');
                                // eslint-disable-next-line eqeqeq
                                if (rangeIndex != -1) {
                                    tokens[rangeIndex].values = [props.min, props.max, props.defaultValue, props.skew, props.increment];
                                }
                            }
                            const boundsIndex = tokens.findIndex(({ token }) => token === 'bounds');
                            tokens[boundsIndex].values = [props.left, props.top, props.width, props.height];
                            lines[i] = `${lines[i].split(' ')[0]} ` + tokens.map(({ token, values }) => typeof values[0] === 'string' ? `${token}("${values.join(', ')}")` : `${token}(${values.join(', ')})`).join(', ');
                            // console.log(lines[i]);
                            await editBuilder.replace(new vscode.Range(document.lineAt(i).range.start, document.lineAt(i).range.end), lines[i]);
                            textEditor.selection = new vscode.Selection(i, 0, i, 10000);
                        }
                        else {
                        }
                    }
                    if (lines[i] === '</Cabbage>') {
                        break;
                    }
                }
                let count = 0;
                lines.forEach((line) => {
                    if (line.trimStart().startsWith("</Cabbage>")) {
                        lineNumber = count;
                    }
                    count++;
                });
                //this is called when we create a widgets from the popup menu in the UI builder
                if (!foundChannel && props.type !== "form") {
                    console.log("here");
                    let newLine = `${props.type} bounds(${props.left}, ${props.top}, ${props.width}, ${props.height}), ${utils_js_1.CabbageUtils.getCabbageCodeFromJson(jsonText, "channel")}`;
                    if (props.type.indexOf('slider') > -1) {
                        newLine += ` ${utils_js_1.CabbageUtils.getCabbageCodeFromJson(jsonText, "range")}`;
                    }
                    editBuilder.insert(new vscode.Position(lineNumber, 0), newLine + '\n');
                    textEditor.selection = new vscode.Selection(lineNumber, 0, lineNumber, 10000);
                }
            }
        });
    }
}
/**
 * Returns html text to use in webview - various scripts get passed as vscode.Uri's
 */
function getWebviewContent(mainJS, styles, cabbageStyles, interactJS, widgetWrapper, colourPickerJS, colourPickerStyles, menu) {
    return `
<!doctype html>
<html lang="en">

<head>
  <meta charset="UTF-8" />
  <link rel="icon" type="image/svg+xml" href="/vite.svg" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <script type="module" src="${interactJS}"></script>
  <script type="module" src="${colourPickerJS}"></script>
  <link href="${styles}" rel="stylesheet">
  <link href="${cabbageStyles}" rel="stylesheet">  
  <link href="${colourPickerStyles}" rel="stylesheet">  

  <style>
  .full-height-div {
	height: 100vh; /* Set the height to 100% of the viewport height */
  }
  </style>
</head>

<body data-vscode-context='{"webviewSection": "nav", "preventDefaultContextMenuItems": true}'>


<div id="parent" class="full-height-div">
  <div id="LeftPanel" class="full-height-div draggablePanel">
    <div id="MainForm" class="form resizeOnly">
      <div class="wrapper">
        <div class="content" style="overflow-y: auto;">
          <ul class="menu">
            ${menu}
          </ul>
        </div>
      </div>
    </div>
    <!-- new draggables go here -->
  </div>
  <span class="popup" id="popupValue">50</span>
  <div id="RightPanel" class="full-height-div">
    <div class="property-panel full-height-div">
      <!-- Properties will be dynamically added here -->
    </div>
  </div>
</div>
<div id="fullScreenOverlay" class="full-screen-div" style="display: none;">
  <!-- Insert your SVG code here -->
  <svg width="416" height="350" viewBox="0 0 416 350" fill="none" xmlns="http://www.w3.org/2000/svg">
<path d="M246.474 9.04216C313.909 21.973 358.949 68.9013 383.59 128.201L396.749 166.562L406.806 180.947C413.963 192.552 414.248 198.338 414.098 211.317C413.746 238.601 390.597 258.708 362.134 257.606C320.514 256.007 301.84 208.232 324.905 177.751C335.885 163.221 350.92 158.618 368.839 158.57L353.234 117.012C336.136 81.4166 310.272 54.6758 274.97 34.8559C258.04 25.3616 237.188 19.016 217.978 15.3557C208.944 13.6295 194.679 14.3648 189.482 6.72452C212.078 4.98229 223.761 4.6786 246.474 9.04216ZM73.8728 69.0612C64.1004 78.9551 55.6689 90.4475 49.2992 102.627C41.1192 118.259 33.9785 142.73 32.2017 160.169L30.5422 182.546C30.3746 188.236 32.2184 191.257 30.5422 196.931C19.0935 170.206 14.7521 144.728 30.5422 118.611C37.6997 107.838 42.9295 103.314 52.0315 94.6352C41.9573 97.8479 35.42 100.006 26.9551 106.655C4.19183 124.525 -2.46282 158.602 8.1645 184.144C16.6798 204.683 26.8042 205.626 33.5929 219.309C-3.88761 198.434 -10.14 143.577 16.026 112.217C21.2894 105.904 27.7764 100.965 35.2692 97.2405C40.432 94.6672 47.0531 93.3725 50.9755 89.3605L64.1674 70.6596C75.3143 56.7377 77.9963 56.1143 90.5848 45.0855C90.1322 54.0205 80.0078 62.8595 73.8728 69.0612ZM395.592 271.863C364.716 296.11 318.469 290.005 294.968 259.268C281.457 241.622 277.585 217.982 282.53 196.931C287.659 175.129 301.941 155.102 323.581 145.623C329.163 143.178 347.283 136.992 352.513 140.972C355.077 142.922 355.077 146.183 355.429 148.98C345.69 149.811 338.533 152.305 330.286 157.371C312.015 168.576 298.572 188.588 298.572 209.718C298.572 241.67 331.359 274.117 365.487 271.767C390.681 270.025 397.184 260.706 415.774 248.079C410.192 257.973 404.761 264.67 395.592 271.863Z" fill="#0295CF"/>
<path d="M230.675 348.909H227.077L218.08 349.481L213.282 348.962C177.915 346.548 139.152 335.834 110.124 315.074C69.8324 286.272 49.2547 241.867 47.1256 193.3L46.5499 186.742C46.3339 168.183 49.2547 144.15 55.1563 126.526C60.5782 110.351 67.7632 95.3805 78.271 81.811C88.6408 68.4144 96.7255 63.919 105.176 47.2314L122.833 13.2479C124.194 11.0301 128.441 2.76674 130.114 1.75916C131.817 0.727738 133.389 2.13477 134.714 3.12446C136.849 4.71036 139.776 7.36345 142.511 7.61981C145.672 7.91195 147.981 5.39599 149.684 3.11254C150.728 1.72339 151.819 -0.530244 153.93 0.232892C155.256 0.709852 157.637 3.2437 158.704 4.30494C161.919 7.49461 168.978 15.5195 173.099 16.4853C176.367 17.2544 179.234 15.2273 181.489 14.8338C184.854 14.2496 187.091 18.0712 188.776 20.4023L196.147 31.134L204.082 41.8656L215.879 59.7516C226.615 75.7596 236.409 89.2098 241.813 108.044C245.04 119.288 245.201 126.973 245.07 138.45C244.992 144.758 242.551 157.66 241.123 164.087C236.625 184.34 229.752 204.098 228.222 224.899L227.677 230.861V239.208C227.713 261.697 238.262 288.406 250.281 307.175L265.989 329.234L274.458 339.966C264.55 344.288 241.549 348.778 230.675 348.909ZM168.9 3.11254L168.301 3.70874V3.11254H168.9ZM290.051 11.4593L289.452 12.0555V11.4593H290.051ZM291.251 12.0555L290.651 12.6517V12.0555H291.251ZM292.45 12.6517L291.851 13.2479V12.6517H292.45ZM293.65 13.2479L293.05 13.8441V13.2479H293.65ZM294.849 13.8441L294.25 14.4403V13.8441H294.849ZM296.049 14.4403L295.449 15.0365V14.4403H296.049ZM297.248 15.0365L296.649 15.6327V15.0365H297.248ZM298.448 15.6327L297.848 16.2289V15.6327H298.448ZM299.647 16.2289L299.048 16.8251V16.2289H299.647ZM333.234 37.4895L345.481 47.7143C346.452 47.9766 347.79 47.8216 348.828 47.7143C358.184 48.0124 365.111 55.1072 370.161 62.1364C380.699 76.7969 386.09 93.7529 390.433 111.025C392.646 119.825 394.139 128.839 395.105 137.854C395.399 140.578 396.478 146.219 395.609 148.585C395.027 141.699 389.815 126.824 387.242 119.968C376.53 91.4039 357.788 63.0307 335.033 42.5631C328.37 36.5713 321.449 30.8597 314.042 25.7801C310.779 23.5443 303.798 19.7585 301.447 17.4213C312.068 21.9704 324.136 30.3768 333.234 37.4895ZM221.679 26.9606C223.808 27.7654 226.261 28.1231 227.677 29.9356H215.681C212.767 29.9773 206.781 31.277 204.394 29.9356C202.637 28.934 200.502 25.1839 199.488 23.3834C205.066 23.6397 216.605 25.0468 221.679 26.9606ZM231.275 26.9606L230.675 27.5568V26.9606H231.275ZM230.675 32.9226C239.852 32.9344 246.065 36.5475 254.066 40.5241C268.43 47.6666 282.165 56.4427 294.25 66.9954C300.985 72.8859 304.745 76.7314 310.491 83.5996C311.811 85.1795 315.049 88.7329 315.127 90.754C315.181 92.3936 313.226 95.3209 312.326 96.716C309.262 101.444 305.909 106.106 302.292 110.429C289.41 125.852 278.65 133.358 264.262 146.302C260.555 149.635 252.093 159.532 249.268 161.106C250.569 154.017 252.255 146.236 252.267 139.046V127.122C252.249 114.107 246.887 97.3778 240.685 85.9844L217.073 49.02C214.848 45.5143 209.102 37.2748 207.885 34.115L230.675 32.9226ZM275.657 48.4238L275.057 49.02V48.4238H275.657ZM277.456 49.6162L276.857 50.2124V49.6162H277.456ZM320.039 96.1198C327.548 107.316 334.787 120.349 339.231 133.084C330.283 135.439 325.761 135.94 317.04 140.394C297.656 150.291 282.836 169.226 276.275 189.723C263.488 229.675 283.532 276.655 324.837 290.171C333.378 292.967 338.776 293.462 347.628 293.462C344.623 298.572 336.748 305.255 332.034 309.208C320.555 318.836 309.651 325.591 296.049 331.804C291.407 333.92 286.657 336.418 281.655 337.581L272.311 326.253C264.568 316.386 256.609 305.148 250.755 294.058C234.712 263.634 231.473 233.735 239.594 200.455C241.897 191.023 243.384 179.272 248.872 171.241C267.986 143.273 301.195 128.899 318.84 96.1198H320.039Z" fill="#93D200"/>
</svg>


</div>
  	<script>
  		var vscodeMode = true; 
	</script>
  <script type="module" src="${widgetWrapper}"></script>
  <script type="module" src="${mainJS}"></script>
</body>

</html>`;
}


/***/ }),
/* 1 */
/***/ ((module) => {

module.exports = require("vscode");

/***/ }),
/* 2 */
/***/ ((__unused_webpack_module, __webpack_exports__, __webpack_require__) => {

__webpack_require__.r(__webpack_exports__);
/* harmony export */ __webpack_require__.d(__webpack_exports__, {
/* harmony export */   RotarySlider: () => (/* binding */ RotarySlider)
/* harmony export */ });
/* harmony import */ var _utils_js__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(3);


/**
 * Rotary Slider (rslider) class
 */
class RotarySlider {
  constructor() {
    this.props = {
      "top": 10, // Top position of the rotary slider widget
      "left": 10, // Left position of the rotary slider widget
      "width": 60, // Width of the rotary slider widget
      "height": 60, // Height of the rotary slider widget
      "channel": "rslider", // Unique identifier for the rotary slider widget
      "min": 0, // Minimum value of the slider
      "max": 1, // Maximum value of the slider
      "value": 0, // Current value of the slider
      "defaultValue": 0, // Default value of the slider
      "skew": 1, // Skew factor for the slider
      "increment": 0.001, // Incremental value change per step
      "index": 0, // Index of the slider
      "text": "", // Text displayed on the slider
      "fontFamily": "Verdana", // Font family for the text displayed on the slider
      "fontSize": 0, // Font size for the text displayed on the slider
      "align": "centre", // Alignment of the text on the slider
      "textOffsetY": 0, // Vertical offset for the text displayed on the slider
      "valueTextBox": 0, // Display a textbox showing the current value
      "colour": _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageColours.getColour("blue"), // Background color of the slider
      "trackerColour": _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageColours.getColour('green'), // Color of the slider tracker
      "trackerBackgroundColour": "#ffffff", // Background color of the slider tracker
      "trackerOutlineColour": "#525252", // Outline color of the slider tracker
      "fontColour": "#dddddd", // Font color for the text displayed on the slider
      "outlineColour": "#525252", // Color of the slider outline
      "textBoxOutlineColour": "#999999", // Outline color of the value textbox
      "textBoxColour": "#555555", // Background color of the value textbox
      "markerColour": "#222222", // Color of the marker on the slider
      "trackerOutlineWidth": 3, // Outline width of the slider tracker
      "trackerWidth": 20, // Width of the slider tracker
      "outlineWidth": 2, // Width of the slider outline
      "type": "rslider", // Type of the widget (rotary slider)
      "decimalPlaces": 1, // Number of decimal places in the slider value
      "velocity": 0, // Velocity value for the slider
      "popup": 1, // Display a popup when the slider is clicked
      "visible": 1, // Visibility of the slider
      "automatable": 1, // Ability to automate the slider
      "valuePrefix": "", // Prefix to be displayed before the slider value
      "valuePostfix": "", // Postfix to be displayed after the slider value
      "presetIgnore": 0, // Ignore preset value for the slider
    };


    this.panelSections = {
      "Properties": ["type", "channel"],
      "Bounds": ["left", "top", "width", "height"],
      "Range": ["min", "max", "defaultValue", "skew", "increment"],
      "Text": ["text", "fontSize", "fontFamily", "fontColour", "textOffsetY", "align"],
      "Colours": ["colour", "trackerColour", "trackerBackgroundColour", "trackerOutlineColour", "trackerStrokeColour", "outlineColour", "textBoxOutlineColour", "textBoxColour", "markerColour"]
    };

    this.moveListener = this.pointerMove.bind(this);
    this.upListener = this.pointerUp.bind(this);
    this.startY = 0;
    this.startValue = 0;
    this.vscode = null;
    this.isMouseDown = false;
    this.decimalPlaces = 0;
  }

  pointerUp() {
    if (this.props.active === 0) {
      return '';
    }
    const popup = document.getElementById('popupValue');
    popup.classList.add('hide');
    popup.classList.remove('show');
    window.removeEventListener("pointermove", this.moveListener);
    window.removeEventListener("pointerup", this.upListener);
    this.isMouseDown = false;
  }

  pointerDown(evt) {
    if (this.props.active === 0) {
      return '';
    }

    this.isMouseDown = true;
    this.startY = evt.clientY;
    this.startValue = this.props.value;
    window.addEventListener("pointermove", this.moveListener);
    window.addEventListener("pointerup", this.upListener);
  }

  mouseEnter(evt) {
    if (this.props.active === 0) {
      return '';
    }

    const popup = document.getElementById('popupValue');
    const form = document.getElementById('MainForm');
    const rect = form.getBoundingClientRect();
    this.decimalPlaces = _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageUtils.getDecimalPlaces(this.props.increment);

    if (popup) {
      popup.textContent = parseFloat(this.props.value).toFixed(this.decimalPlaces);

      // Calculate the position for the popup
      const sliderLeft = this.props.left;
      const sliderWidth = this.props.width;
      const formLeft = rect.left;
      const formWidth = rect.width;

      // Determine if the popup should be on the right or left side of the slider
      const sliderCenter = formLeft + (formWidth / 2);
      let popupLeft;
      if (sliderLeft + (sliderWidth) > sliderCenter) {
        // Place popup on the left of the slider thumb
        popupLeft = formLeft + sliderLeft - popup.offsetWidth - 10;
        console.log("Pointer on the left");
        popup.classList.add('right');
      } else {
        // Place popup on the right of the slider thumb
        popupLeft = formLeft + sliderLeft + sliderWidth + 10;
        console.log("Pointer on the right");
        popup.classList.remove('right');
      }

      const popupTop = rect.top + this.props.top + this.props.height * .5; // Adjust top position relative to the form's top

      // Set the calculated position
      popup.style.left = `${popupLeft}px`;
      popup.style.top = `${popupTop}px`;
      popup.style.display = 'block';
      popup.classList.add('show');
      popup.classList.remove('hide');
    }
  }


  mouseLeave(evt) {
    if (!this.isMouseDown) {
      const popup = document.getElementById('popupValue');
      popup.classList.add('hide');
      popup.classList.remove('show');
    }
  }

  addVsCodeEventListeners(widgetDiv, vs) {
    this.vscode = vs;
    widgetDiv.addEventListener("pointerdown", this.pointerDown.bind(this));
    widgetDiv.addEventListener("mouseenter", this.mouseEnter.bind(this));
    widgetDiv.addEventListener("mouseleave", this.mouseLeave.bind(this));
    widgetDiv.RotarySliderInstance = this;
  }

  addEventListeners(widgetDiv) {
    widgetDiv.addEventListener("pointerdown", this.pointerDown.bind(this));
    widgetDiv.addEventListener("mouseenter", this.mouseEnter.bind(this));
    widgetDiv.addEventListener("mouseleave", this.mouseLeave.bind(this));
    widgetDiv.RotarySliderInstance = this;
  }

  pointerMove({ clientY }) {
    if (this.props.active === 0) {
      return '';
    }

    const steps = 200;
    const valueDiff = ((this.props.max - this.props.min) * (clientY - this.startY)) / steps;
    const value = _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageUtils.clamp(this.startValue - valueDiff, this.props.min, this.props.max);

    this.props.value = Math.round(value / this.props.increment) * this.props.increment;
    const widgetDiv = document.getElementById(this.props.channel);
    widgetDiv.innerHTML = this.getInnerHTML();

    const msg = { channel: this.props.channel, value: _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageUtils.map(this.props.value.map, this.props.min, this.props.max, 0, 1) }
    if (this.vscode) {
      this.vscode.postMessage({
        command: 'channelUpdate',
        text: JSON.stringify(msg)
      })
    }
    else {
      var message = {
        "msg": "parameterUpdate",
        "paramIdx": this.props.index,
        "value": _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageUtils.map(this.props.value, this.props.min, this.props.max, 0, 1)
      };
      // IPlugSendMsg(message);
    }
  }

  // https://stackoverflow.com/questions/20593575/making-circular-progress-bar-with-html5-svg
  polarToCartesian(centerX, centerY, radius, angleInDegrees) {
    var angleInRadians = ((angleInDegrees - 90) * Math.PI) / 180.0;
    return {
      x: centerX + radius * Math.cos(angleInRadians),
      y: centerY + radius * Math.sin(angleInRadians),
    };
  }

  describeArc(x, y, radius, startAngle, endAngle) {
    var start = this.polarToCartesian(x, y, radius, endAngle);
    var end = this.polarToCartesian(x, y, radius, startAngle);

    var largeArcFlag = "0";
    if (endAngle >= startAngle) {
      largeArcFlag = endAngle - startAngle <= 180 ? "0" : "1";
    } else {
      largeArcFlag = endAngle + 360.0 - startAngle <= 180 ? "0" : "1";
    }

    var d = ["M", start.x, start.y, "A", radius, radius, 0, largeArcFlag, 0, end.x, end.y].join(" ");

    return d;
  }

  handleInputChange(evt) {
    if (evt.key === 'Enter') {
      const inputValue = parseFloat(evt.target.value);
      if (!isNaN(inputValue) && inputValue >= this.props.min && inputValue <= this.props.max) {
        this.props.value = inputValue;
        const widgetDiv = document.getElementById(this.props.channel);
        widgetDiv.innerHTML = this.getInnerHTML();
        widgetDiv.querySelector('input').focus();
      }
    }
    else if (evt.key === 'Esc') {
      const widgetDiv = document.getElementById(this.props.channel);
      widgetDiv.querySelector('input').blur();
    }
  }

  getInnerHTML() {
    if (this.props.visible === 0) {
      return '';
    }

    const popup = document.getElementById('popupValue');
    if (popup) {
      popup.textContent = this.props.valuePrefix + parseFloat(this.props.value).toFixed(this.decimalPlaces) + this.props.valuePostfix;
    }

    let w = (this.props.width > this.props.height ? this.props.height : this.props.width) * 0.75;
    const innerTrackerWidth = this.props.trackerWidth - this.props.trackerOutlineWidth;
    const innerTrackerEndPoints = this.props.trackerOutlineWidth * 0.5;
    const trackerOutlineColour = this.props.trackerOutlineWidth == 0 ? this.props.trackerBackgroundColour : this.props.trackerOutlineColour;
    const outerTrackerPath = this.describeArc(this.props.width / 2, this.props.height / 2, (w / 2) * (1 - (this.props.trackerWidth / this.props.width / 2)), -130, 132);
    const trackerPath = this.describeArc(this.props.width / 2, this.props.height / 2, (w / 2) * (1 - (this.props.trackerWidth / this.props.width / 2)), -(130 - innerTrackerEndPoints), 132 - innerTrackerEndPoints);
    const trackerArcPath = this.describeArc(this.props.width / 2, this.props.height / 2, (w / 2) * (1 - (this.props.trackerWidth / this.props.width / 2)), -(130 - innerTrackerEndPoints), _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageUtils.map(this.props.value, this.props.min, this.props.max, -(130 - innerTrackerEndPoints), 132 - innerTrackerEndPoints));

    // Calculate proportional font size if this.props.fontSize is 0
    let fontSize = this.props.fontSize > 0 ? this.props.fontSize : w * 0.24;
    const textY = this.props.height + (this.props.fontSize > 0 ? this.props.textOffsetY : 0);
    let scale = 100;

    if (this.props.valueTextBox == 1) {
      scale = 0.7;
      const moveY = 5;

      const centerX = this.props.width / 2;
      const centerY = this.props.height / 2;
      const inputWidth = _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageUtils.getNumberBoxWidth(this.props);
      const inputX = this.props.width / 2 - inputWidth / 2;

      return `
        <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${this.props.width} ${this.props.height}" width="100%" height="100%" preserveAspectRatio="none">
        <text text-anchor="middle" x=${this.props.width / 2} y="${fontSize}px" font-size="${fontSize}px" font-family="${this.props.fontFamily}" stroke="none" fill="${this.props.fontColour}">${this.props.text}</text>
        <g transform="translate(${centerX}, ${centerY + moveY}) scale(${scale}) translate(${-centerX}, ${-centerY})">
        <path d='${outerTrackerPath}' id="arc" fill="none" stroke=${trackerOutlineColour} stroke-width=${this.props.trackerWidth} />
        <path d='${trackerPath}' id="arc" fill="none" stroke=${this.props.trackerBackgroundColour} stroke-width=${innerTrackerWidth} />
        <path d='${trackerArcPath}' id="arc" fill="none" stroke=${this.props.trackerColour} stroke-width=${innerTrackerWidth} />
        <circle cx=${this.props.width / 2} cy=${this.props.height / 2} r=${(w / 2) - this.props.trackerWidth * 0.65} stroke=${this.props.outlineColour} stroke-width=${this.props.outlineWidth} fill=${this.props.colour} />
        </g>
        <foreignObject x="${inputX}" y="${textY - fontSize * 1.5}" width="${inputWidth}" height="${fontSize * 2}">
          <input type="text" xmlns="http://www.w3.org/1999/xhtml" value="${this.props.value.toFixed(_utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageUtils.getDecimalPlaces(this.props.increment))}"
          style="width:100%; outline: none; height:100%; text-align:center; font-size:${fontSize}px; font-family:${this.props.fontFamily}; color:${this.props.fontColour}; background:none; border:none; padding:0; margin:0;"
          onKeyDown="document.getElementById('${this.props.channel}').RotarySliderInstance.handleInputChange(event)"/>
          />
        </foreignObject>
        </svg>
        `;
    }

    return `
      <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${this.props.width} ${this.props.height}" width="${scale}%" height="${scale}%" preserveAspectRatio="none">
      <path d='${outerTrackerPath}' id="arc" fill="none" stroke=${trackerOutlineColour} stroke-width=${this.props.trackerWidth} />
      <path d='${trackerPath}' id="arc" fill="none" stroke=${this.props.trackerBackgroundColour} stroke-width=${innerTrackerWidth} />
      <path d='${trackerArcPath}' id="arc" fill="none" stroke=${this.props.trackerColour} stroke-width=${innerTrackerWidth} />
      <circle cx=${this.props.width / 2} cy=${this.props.height / 2} r=${(w / 2) - this.props.trackerWidth * 0.65} stroke=${this.props.outlineColour} stroke-width=${this.props.outlineWidth} fill=${this.props.colour} />
      <text text-anchor="middle" x=${this.props.width / 2} y=${textY} font-size="${fontSize}px" font-family="${this.props.fontFamily}" stroke="none" fill="${this.props.fontColour}">${this.props.text}</text>
      </svg>
    `;
  }

}


/***/ }),
/* 3 */
/***/ ((__unused_webpack_module, __webpack_exports__, __webpack_require__) => {

__webpack_require__.r(__webpack_exports__);
/* harmony export */ __webpack_require__.d(__webpack_exports__, {
/* harmony export */   CabbageColours: () => (/* binding */ CabbageColours),
/* harmony export */   CabbageTestUtilities: () => (/* binding */ CabbageTestUtilities),
/* harmony export */   CabbageUtils: () => (/* binding */ CabbageUtils)
/* harmony export */ });
class CabbageUtils {
  /**
 * This uses a simple regex pattern to parse a line of Cabbage code such as 
 * rslider bounds(22, 14, 60, 60) channel("clip") thumbRadius(5), text("Clip") range(0, 1, 0, 1, 0.001)
 * and converts it to a JSON object
 */
  static getCabbageCodeAsJSON(text) {
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
      } else if (name === 'range') {
        // Splitting the value into individual parts for min, max, defaultValue, skew, and increment
        const [min, max, defaultValue, skew, increment] = value.split(',').map(v => parseFloat(v.trim()));
        jsonObj['min'] = min;
        jsonObj['max'] = max;
        jsonObj['defaultValue'] = defaultValue;
        jsonObj['skew'] = skew;
        jsonObj['increment'] = increment;
      } else if (name === 'size') {
        // Splitting the value into individual parts for width and height
        const [width, height] = value.split(',').map(v => parseInt(v.trim()));
        jsonObj['width'] = width;
        jsonObj['height'] = height;
      } else if (name === 'items') {
        // Handling the items attribute
        const items = value.split(',').map(v => v.trim()).join(', ');
        jsonObj['items'] = items;
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
  static async parseCabbageCode(text, widgets, form, insertWidget) {
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
    for (const line of cabbageCode) {
      const codeProps = CabbageUtils.getCabbageCodeAsJSON(line);
      const type = `${line.trimStart().split(' ')[0]}`;
      if (line.trim() != "") {
        if (type != "form") {
          await insertWidget(type, codeProps);
        } else {
          widgets.forEach((widget) => {
            if (widget.props.channel == "MainForm") {
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
    }
  }

  /**
   * this function will return the number of plugin parameter in our widgets array
   */
  static getNumberOfPluginParameters(widgets, ...types) {
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

  /**
   * show / hide Cabbage overlays
   */
  static showOverlay() {
    const overlay = document.getElementById('fullScreenOverlay')
    if (overlay) {
      overlay.style.display = 'flex';
      const leftPanel = document.getElementById('LeftPanel');
      const rightPanel = document.getElementById('RightPanel');
      leftPanel.style.display = 'none';
      rightPanel.style.display = 'none';
    }
  }

  static hideOverlay() {
    const overlay = document.getElementById('fullScreenOverlay')
    if (overlay) {
      overlay.style.display = 'none';
      const leftPanel = document.getElementById('LeftPanel');
      const rightPanel = document.getElementById('RightPanel');
      leftPanel.style.display = 'flex';
      rightPanel.style.display = 'flex';
    }
  }

  /**
   * clamps a value
   * @param {*} num 
   * @param {*} min 
   * @param {*} max 
   * @returns clamped value
   */
  static clamp(num, min, max) {
    return Math.max(min, Math.min(num, max));
  }

  /**
   * returns a remapped value
   * @param {*} value 
   * @param {*} in_min 
   * @param {*} in_max 
   * @param {*} out_min 
   * @param {*} out_max 
   * @returns mapped value
   */
  static map(value, in_min, in_max, out_min, out_max) {
    return ((value - in_min) * (out_max - out_min)) / (in_max - in_min) + out_min;
  };

  /**
   * 
   * @param {*} num 
   * @returns number of decimal places in value
   */
  static getDecimalPlaces(num) {
    const numString = num.toString();
    if (numString.includes('.')) {
      return numString.split('.')[1].length;
    } else {
      return 0;
    }
  }

  /**
   * Returns a unique channel name based on the type and number
   * @param {Array} widgets - Array of JSON objects with unique 'channel' values
   * @returns {String} unique channel name
   */
  static getUniqueChannelName(type, widgets) {
    // Extract all existing channel names
    const existingChannels = widgets.map(widget => widget.channel);

    // Define a function to generate a channel name based on type and a number
    function generateChannelName(type, number) {
      return `${type}${number}`;
    }

    // Start with a number based on the size of the array + 1
    let number = widgets.length + 1;
    let newChannelName = generateChannelName(type, number);

    // Increment the number until a unique channel name is found
    while (existingChannels.includes(newChannelName)) {
      number += 1;
      newChannelName = generateChannelName(type, number);
    }

    return newChannelName;
  }

  static findValidId(event) {
    var target = event.target;

    while (target !== null) {
      if (target.tagName === "DIV" && target.id) {
        return target.id;
      }
      target = target.parentNode;
    }

    return null
  }

  static getElementByIdInChildren(parentElement, targetId) {
    const queue = [parentElement];

    while (queue.length > 0) {
      const currentElement = queue.shift();

      // Check if the current element has the target ID
      if (currentElement.id === targetId) {
        return currentElement;
      }

      // Check if the current element has children
      if (currentElement.children && currentElement.children.length > 0) {
        // Convert HTMLCollection to an array and add the children of the current element to the queue
        const childrenArray = Array.from(currentElement.children);
        queue.push(...childrenArray);
      }
    }

    // If no element with the target ID is found, return null
    return null;
  }

  static getWidgetFromChannel(widgets, channel) {
    widgets.forEach((widget) => {
      if (widget["channel"] === channel)
        return widget;
    })
    return null;
  }

  static getStringWidth(text, props, padding = 10) {
    var canvas = document.createElement('canvas');
    let fontSize = 0;
    switch (props.type) {

      case 'hslider':
        fontSize = props.height * .8;
        break;
      case "rslider":
        fontSize = props.width * .3;
        break;
      case "vslider":
        fontSize = props.width * .3;
        break;
      case "combobox":
        fontSize = props.height * .5;
        break;
      default:
        console.error('getStringWidth..');
        break;
    }

    var ctx = canvas.getContext("2d");
    ctx.font = `${fontSize}px ${props.fontFamily}`;
    var width = ctx.measureText(text).width;
    return width + padding;
  }

  static getNumberBoxWidth(props) {
    // Get the number of decimal places in props.increment
    const decimalPlaces = CabbageUtils.getDecimalPlaces(props.increment);

    // Format props.max with the correct number of decimal places
    const maxNumber = props.max.toFixed(decimalPlaces);

    // Calculate the width of the string representation of maxNumber
    const maxNumberWidth = CabbageUtils.getStringWidth(maxNumber, props);

    return maxNumberWidth;
  }


  static updateInnerHTML(channel, instance) {
    const element = document.getElementById(channel);
    if (element) {
      element.innerHTML = instance.getInnerHTML();
    }
  }

  static getWidgetDiv(channel) {
    const element = document.getElementById(channel);
    return element || null;
  }

  /**
 * This uses a simple regex pattern to get tokens from a line of Cabbage code
 */
  static getTokens(text) {
    const inputString = text
    const regex = /(\w+)\(([^)]+)\)/g;
    const tokens = [];
    let match;
    while ((match = regex.exec(inputString)) !== null) {
      const token = match[1];
      const values = match[2].split(',').map(value => value.trim()); // Split values into an array
      tokens.push({ token, values });
    }
    return tokens;
  }

  static sendToBack(currentDiv) {
      const parentElement = currentDiv.parentElement;
      const allDivs = parentElement.getElementsByTagName('div');
      console.log(currentDiv);
      console.log(allDivs);
      for (let i = 0; i < allDivs.length; i++) {
        if (allDivs[i] !== currentDiv) {
          allDivs[i].style.zIndex = 1; // Bring other divs to the top
        } else {
          allDivs[i].style.zIndex = 0; // Keep the current div below others
        }
      }
    }
  /**
   * This function will return an identifier in the form of ident(param) from an incoming
   * JSON object of properties
   */
  static getCabbageCodeFromJson(json, name) {
    const obj = JSON.parse(json);
    let syntax = '';

    if (name === 'range' && obj['type'].indexOf('slider') > -1) {
      const { min, max, defaultValue, skew, increment } = obj;
      syntax = `range(${min}, ${max}, ${defaultValue}, ${skew}, ${increment})`;
      return syntax;
    }
    if (name === 'bounds') {
      const { left, top, width, height } = obj;
      syntax = `bounds(${left}, ${top}, ${width}, ${height})`;
      return syntax;
    }


    for (const key in obj) {
      if (obj.hasOwnProperty(key) && key === name) {
        const value = obj[key];
        // Check if value is string and if so, wrap it in single quotes
        const formattedValue = typeof value === 'string' ? `"${value}"` : value;
        syntax += `${key}(${formattedValue}), `;
      }
    }
    // Remove the trailing comma and space
    syntax = syntax.slice(0, -2);
    return syntax;
  }

  /**
   * This function will check the current widget props against the default set, and return an 
   * array for any identifiers that are different to their default values - this only returns the identifiers
   * that need updating, not their parameters..
   */
  static findUpdatedIdentifiers(initial, current) {
    const initialWidgetObj = JSON.parse(initial);
    const currentWidgetObj = JSON.parse(current);

    var updatedIdentifiers = [];

    // Iterate over the keys of obj1
    for (var key in initialWidgetObj) {
      // Check if obj2 has the same key
      if (currentWidgetObj.hasOwnProperty(key)) {
        // Compare the values of the keys
        if (initialWidgetObj[key] !== currentWidgetObj[key]) {
          // If values are different, add the key to the differentKeys array
          updatedIdentifiers.push(key);
        }
      } else {
        // If obj2 doesn't have the key from obj1, add it to differentKeys array
        updatedIdentifiers.push(key);
      }
    }

    // Iterate over the keys of obj2 to find any keys not present in obj1
    for (var key in currentWidgetObj) {
      if (!initialWidgetObj.hasOwnProperty(key)) {
        // Add the key to differentKeys array
        updatedIdentifiers.push(key);
      }
    }


    if (currentWidgetObj['type'].indexOf('slider') > -1) {
      updatedIdentifiers.push('min');
      updatedIdentifiers.push('max');
      updatedIdentifiers.push('value');
      updatedIdentifiers.push('skew');
      updatedIdentifiers.push('increment');

    }

    return updatedIdentifiers;
  }


  static generateIdentifierTestCsd(widgets) {



  }

  static updateBounds(props, identifier) {
    const element = document.getElementById(props.channel);
    if (element) {
      switch (identifier) {
        case 'left':
          element.style.left = props.left + "px";
          break;
        case 'top':
          element.style.top = props.top + "px";
          break;
        case 'width':
          element.style.width = props.width + "px";
          break;
        case 'height':
          element.style.height = props.height + "px";
          break;
      }
    }
  }
}

class CabbageColours {
  static getColour(colourName) {
    const colourMap = {
      "blue": "#0295cf",
      "green": "#93d200",
      "red": "#ff0000",
      "yellow": "#f0e14c",
      "purple": "#a020f0",
      "orange": "#ff6600",
      "grey": "#808080",
      "white": "#ffffff",
      "black": "#000000"
    };

    return colourMap[colourName] || colourMap["blue"];
  }
  static brighter(hex, amount) {
    return this.adjustBrightness(hex, amount);
  }

  static darker(hex, amount) {
    return this.adjustBrightness(hex, -amount);
  }

  static adjustBrightness(hex, factor) {
    // Remove the hash at the start if it's there
    hex = hex.replace(/^#/, '');

    // Parse r, g, b values
    let r = parseInt(hex.slice(0, 2), 16);
    let g = parseInt(hex.slice(2, 4), 16);
    let b = parseInt(hex.slice(4, 6), 16);

    // Apply the factor to each color component
    r = Math.round(Math.min(255, Math.max(0, r + (r * factor))));
    g = Math.round(Math.min(255, Math.max(0, g + (g * factor))));
    b = Math.round(Math.min(255, Math.max(0, b + (b * factor))));

    // Convert back to hex and pad with zeroes if necessary
    r = r.toString(16).padStart(2, '0');
    g = g.toString(16).padStart(2, '0');
    b = b.toString(16).padStart(2, '0');

    return `#${r}${g}${b}`;
  }

}

/*
* This class contains utility functions for testing the Cabbage UI
*/
class CabbageTestUtilities {

  /*
  * Generate a CabbageWidgetDescriptors class with all the identifiers for each widget type, this can be inserted
  directly into the Cabbage source code
  */
  static generateCabbageWidgetDescriptorsClass(widgets) {
    let cppCode = `
#pragma once

#include <iostream>
#include <regex>
#include <string>
#include <vector>
#include "json.hpp"
#include "CabbageUtils.h"

class CabbageWidgetDescriptors {
public:
    static nlohmann::json get(std::string widgetType) {
`;

    // Generate the widget descriptors for each widget type
    widgets.forEach((widget) => {
      const jsonString = JSON.stringify(widget.props, null, 2).split('\n').map(line => `            ${line}`).join('\n');
      cppCode += `
        if (widgetType == "${widget.props.type}") {
            std::string jsonString = R"(
${jsonString}
            )";
            return nlohmann::json::parse(jsonString);
        }`;
    });

    cppCode += `
        cabAssert(false, "Invalid widget type");
    }
};
`;

    console.log(cppCode);
  }

  /*
  * Generate a CSD file from the widgets array and tests all identifiers. For now this only tests numeric values
  * for each widget type using cabbageSetValue and only string types for cabbageSet
  */
  static generateIdentifierTestCsd(widgets) {
    let csoundCode = "<Cabbage>\nform size(800, 400)";

    widgets.forEach((widget) => {
      csoundCode += `   ${widget.props.type} bounds(-1000, 0, 100, 100)\n`;
    });

    csoundCode += `   csoundoutput bounds(0, 0, 780, 380)\n`;
    csoundCode += "</Cabbage>\n";

     csoundCode += `
<CsoundSynthesizer>
<CsOptions>
-n -d -m0d
</CsOptions> 
<CsInstruments>
; Initialize the global variables. 
ksmps = 32
nchnls = 2
0dbfs = 1
`;

    // Instrument for setting string values
    csoundCode += `
    
giErrorCnt init 0
giIdentifiersChecked init 0    

instr CabbageSetString
  SChannel strcpy p4
  SIdentifier strcpy p5
  SString strcpy p6
  cabbageSet SChannel, sprintf("%s(\\"%s\\")", SIdentifier, SString)
endin

instr CabbageCheckString
  SChannel strcpy p4
  SIdentifier strcpy p5
  SString strcpy p6
  S1 cabbageGet SChannel, SIdentifier
  iRes strcmp S1, SString
  if iRes != 0 then
      prints("")
      prints("=========CabbageCheckString============")
      prints("")
      prints sprintf("CabbageCheckString Error: %s %s", SChannel, SIdentifier)
      prints sprintf("CurrentValue: [%s] Incoming value: [%s]", S1, SString)
      prints sprintf("Size of string: [%d] Incoming size: [%d]", strlen(S1), strlen(SString))
      giErrorCnt += 1
  endif
  giIdentifiersChecked += 1
  prints(sprintf("Checked %d identifiers", giIdentifiersChecked))
endin

instr CabbageSetFloat
  SChannel strcpy p4
  SIdentifier strcpy p5
  SString = sprintf("%s(%3.3f)", SIdentifier, p6)
  cabbageSet SChannel,SString 
endin

instr CabbageCheckFloat
  SChannel strcpy p4
  SIdentifier strcpy p5
  i1 cabbageGet SChannel, SIdentifier
  ;checking floats can be iffy..
  if i1 <= p6-0.01 || i1 >= p6+0.01 then
        prints("")
        prints("=========CabbageCheckInt============")
        prints("")
        prints sprintf("CabbageCheckFloat Error: %s %s", SChannel, SIdentifier)
        prints sprintf("CurrentValue: [%f] Incoming value: [%f]", i1, p6)
        giErrorCnt += 1
  endif
  giIdentifiersChecked += 1
  prints(sprintf("Checked %d identifiers", giIdentifiersChecked))
endin

instr CabbageSetValue
  SChannel strcpy p4
  cabbageSetValue SChannel, p5
endin

instr CabbageCheckValue
  SChannel strcpy p4
  i1 cabbageGetValue SChannel
  if i1 != p5 then
      prints("")
      prints("=========CabbageCheckValue============")
      prints("")
      prints sprintf("CabbageCheckValue Error: %s %s", SChannel, "value")
      prints sprintf("CurrentValue: [%f] Incoming value: [%f]", i1, p5)
      giErrorCnt += 1
  endif
  giIdentifiersChecked += 1
  prints(sprintf("Checked %d identifiers", giIdentifiersChecked))
endin

instr GetErrorCount
  prints("")
  prints("")
  prints("===========Error report ================")
  prints sprintf("Number of identifiers checked: %d", giIdentifiersChecked)
  prints sprintf("Number of errors found: %d", giErrorCnt)
endin
`;

    csoundCode += '</CsInstruments>\n';

    // Generate CsScore section
    csoundCode += '<CsScore>\n';

    let delay = 0.2; // Delay between each set/check pair (in seconds)
    let setStartTime = 1.0; // Start time for score events
    let checkStartTime = setStartTime+0.1; // Start time for score events

    widgets.forEach((widget) => {
        for (const [key, value] of Object.entries(widget.props)) {
            if (key !== 'type' && key !== 'index' && key!== 'channel') {
                if (key !== 'value' && key !== 'defaultValue') {
                    const newValue = CabbageTestUtilities.getSimilarValue(value);
                    if (typeof value === 'number') {
                        csoundCode += `i"CabbageSetFloat" ${setStartTime.toFixed(1)} 0.2 "${widget.props.channel}" "${key}" ${newValue}\n`;
                        csoundCode += `i"CabbageCheckFloat" ${checkStartTime.toFixed(1)} 0.2 "${widget.props.channel}" "${key}" ${newValue}\n`;
                    } else {
                        csoundCode += `i"CabbageSetString" ${setStartTime.toFixed(1)} 0.2 "${widget.props.channel}" "${key}" "${newValue}"\n`;
                        csoundCode += `i"CabbageCheckString" ${checkStartTime.toFixed(1)} 0.2 "${widget.props.channel}" "${key}" "${newValue}"\n`;
                    }
                    setStartTime += delay;
                    checkStartTime += delay;
                } else if (key === 'value') {
                    const newValue = CabbageTestUtilities.getSimilarValue(value);
                    csoundCode += `i"CabbageSetValue" ${setStartTime.toFixed(1)} 0.2 "${widget.props.channel}" ${newValue}\n`;
                    csoundCode += `i"CabbageCheckValue" ${checkStartTime.toFixed(1)} 0.2 "${widget.props.channel}" ${newValue}\n`;
                    setStartTime += delay;
                    checkStartTime += delay;
                }
            }
        }
    });

    csoundCode += `i"GetErrorCount" ${setStartTime.toFixed(1)} 0.2\n`;
    csoundCode += '</CsScore>\n';
    csoundCode += '</CsoundSynthesizer>\n';

    console.log(csoundCode);
}



  static getSimilarValue(value) {
    if (typeof value === 'string') {
      if (/^#[0-9a-fA-F]{6,8}$/.test(value)) {
        // Hex color code
        return this.generateRandomHexColor(value.length);
      } else if (/^[0-9., ]*$/.test(value)) {
        // Number string (comma-separated, can include floating point)
        return this.generateRandomCommaSeparatedNumbers(value);
      } else if (value.trim() === '') {
        // Empty string
        return this.generateRandomString(5); // Default length of 5 for empty strings
      } else {
        // Comma-separated words
        return this.generateRandomCommaSeparatedWords(value);
      }
    } else if (typeof value === 'number') {
      // Number
      return this.generateRandomNumber(value);
    } else if (value === null || value === undefined) {
      // Null or undefined
      return this.generateRandomString(5); // Default length of 5 for unknown types
    } else {
      // Any other type (including empty arrays/objects, which are uncommon in typical JSON usage)
      return this.generateRandomString(5); // Default length of 5 for unknown types
    }
  }

  static generateRandomHexColor(length) {
    let hex = '#';
    for (let i = 0; i < length - 1; i++) {
      hex += Math.floor(Math.random() * 16).toString(16);
    }
    return hex;
  }

  static generateRandomCommaSeparatedNumbers(value) {
    if (value.trim() === '') {
      // Handle empty string by returning a default random string
      return this.generateRandomString(5);
    }

    return value.split(',').map(num => {
      num = num.trim();
      if (num === '') {
        return this.generateRandomString(5); // Handle empty parts
      } else if (num.includes('.')) {
        // Floating point number
        const floatValue = parseFloat(num);
        if (floatValue < 1) {
          return (Math.random()).toFixed(2); // Generate a new number between 0 and 1
        } else {
          return (floatValue + (Math.random() * 10 - 5)).toFixed(2);
        }
      } else {
        // Integer number
        const intValue = parseInt(num);
        return (intValue + Math.floor(Math.random() * 10) + 1).toString(); // Ensure it's not zero
      }
    }).join(', ');
  }

  static generateRandomCommaSeparatedWords(value) {
    if (value.trim() === '') {
      // Handle empty string by returning a default random word
      return this.generateRandomString(5);
    }

    const words = value.split(',').map(word => {
      if (word.trim() === '') {
        return this.generateRandomString(5); // Handle empty parts
      } else {
        return this.generateRandomString(word.trim().length);
      }
    });
    return words.join(', ');
  }

  static generateRandomString(length) {
    const characters = 'abcdefghijklmnopqrstuvwxyz';
    let result = '';
    for (let i = 0; i < length; i++) {
      result += characters.charAt(Math.floor(Math.random() * characters.length));
    }
    return result;
  }

  static generateRandomNumber(value) {
    if (Number.isInteger(value)) {
      return value + Math.floor(Math.random() * 10) + 1; // Ensure it's not zero
    } else {
      if (value < 1) {
        return (Math.random()).toFixed(2); // Generate a new number between 0 and 1
      } else {
        return (value + (Math.random() * 10 - 5)).toFixed(2); // For floating point numbers >= 1
      }
    }
  }



}

/***/ }),
/* 4 */
/***/ ((__unused_webpack_module, __webpack_exports__, __webpack_require__) => {

__webpack_require__.r(__webpack_exports__);
/* harmony export */ __webpack_require__.d(__webpack_exports__, {
/* harmony export */   HorizontalSlider: () => (/* binding */ HorizontalSlider)
/* harmony export */ });
/* harmony import */ var _cabbage_js__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(5);
/* harmony import */ var _utils_js__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(3);



/**
 * Horizontal Slider (hslider) class
 */
class HorizontalSlider {
  constructor() {
    this.props = {
      "top": 10, // Top position of the horizontal slider
      "left": 10, // Left position of the horizontal slider
      "width": 60, // Width of the horizontal slider
      "height": 60, // Height of the horizontal slider
      "channel": "rslider", // Unique identifier for the horizontal slider
      "min": 0, // Minimum value of the slider
      "max": 1, // Maximum value of the slider
      "value": 0, // Current value of the slider
      "defaultValue": 0, // Default value of the slider
      "skew": 1, // Skew factor for the slider (for non-linear scales)
      "increment": 0.001, // Value increment/decrement when moving the slider
      "index": 0, // Index of the slider
      "text": "", // Text displayed next to the slider
      "fontFamily": "Verdana", // Font family for the text
      "fontSize": 0, // Font size for the text
      "align": "centre", // Text alignment within the slider (center, left, right)
      "valueTextBox": 0, // Whether to display a text box showing the current value
      "colour": _utils_js__WEBPACK_IMPORTED_MODULE_1__.CabbageColours.getColour("blue"), // Background color of the slider track
      "trackerColour": _utils_js__WEBPACK_IMPORTED_MODULE_1__.CabbageColours.getColour('green'), // Color of the slider thumb
      "trackerBackgroundColour": "#ffffff", // Background color of the slider thumb
      "trackerOutlineColour": "#525252", // Color of the outline around the slider thumb
      "fontColour": "#dddddd", // Color of the text displayed next to the slider
      "outlineColour": "#999999", // Color of the slider outline
      "textBoxColour": "#555555", // Color of the value text box (if enabled)
      "trackerOutlineWidth": 1, // Width of the outline around the slider thumb
      "outlineWidth": 1, // Width of the slider outline
      "markerThickness": 0.2, // Thickness of the slider markers
      "markerStart": 0.1, // Start position of the slider markers
      "markerEnd": 0.9, // End position of the slider markers
      "type": "hslider", // Type of the slider (horizontal)
      "decimalPlaces": 1, // Number of decimal places to display for the slider value
      "velocity": 0, // Velocity of slider movement (for gesture-based interaction)
      "visible": 1, // Visibility of the slider (0 for hidden, 1 for visible)
      "popup": 1, // Whether to show a popup when interacting with the slider
      "automatable": 1, // Whether the slider value can be automated (0 for no, 1 for yes)
      "valuePrefix": "", // Prefix to display before the slider value
      "valuePostfix": "", // Postfix to display after the slider value
      "presetIgnore": 0 // Whether the slider should be ignored in presets (0 for no, 1 for yes)
  };
  

    this.panelSections = {
      "Info": ["type", "channel"],
      "Bounds": ["left", "top", "width", "height"],
      "Range": ["min", "max", "default", "skew", "increment"],
      "Text": ["text", "fontSize", "fontFamily", "fontColour", "textOffsetY", "align"],
      "Colours": ["colour", "trackerBackgroundColour", "trackerStrokeColour", "outlineColour", "textBoxOutlineColour", "textBoxColour"]
    };

    this.moveListener = this.pointerMove.bind(this);
    this.upListener = this.pointerUp.bind(this);
    this.startX = 0;
    this.startValue = 0;
    this.vscode = null;
    this.isMouseDown = false;
    this.decimalPlaces = 0;
  }

  pointerUp() {
    const popup = document.getElementById('popupValue');
    popup.classList.add('hide');
    popup.classList.remove('show');
    window.removeEventListener("pointermove", this.moveListener);
    window.removeEventListener("pointerup", this.upListener);
    this.isMouseDown = false;
  }

  pointerDown(evt) {
    if(this.props.active === 0) {
      return '';
    }

    let textWidth = this.props.text ? _utils_js__WEBPACK_IMPORTED_MODULE_1__.CabbageUtils.getStringWidth(this.props.text, this.props) : 0;
    textWidth = this.props.sliderOffsetX > 0 ? this.props.sliderOffsetX : textWidth;
    const valueTextBoxWidth = this.props.valueTextBox ? _utils_js__WEBPACK_IMPORTED_MODULE_1__.CabbageUtils.getNumberBoxWidth(this.props) : 0;
    const sliderWidth = this.props.width - textWidth - valueTextBoxWidth;
  
    
    if (evt.offsetX >= textWidth && evt.offsetX <= textWidth + sliderWidth && evt.target.tagName !== "INPUT") {
      this.isMouseDown = true;
      this.startX = evt.offsetX - textWidth;
      this.props.value = _utils_js__WEBPACK_IMPORTED_MODULE_1__.CabbageUtils.map(this.startX, 0, sliderWidth, this.props.min, this.props.max);
  
      window.addEventListener("pointermove", this.moveListener);
      window.addEventListener("pointerup", this.upListener);
  
      this.props.value = Math.round(this.props.value / this.props.increment) * this.props.increment;
      this.startValue = this.props.value;
      _utils_js__WEBPACK_IMPORTED_MODULE_1__.CabbageUtils.updateInnerHTML(this.props.channel, this.getInnerHTML());
    }
  }
  

  mouseEnter(evt) {
    if(this.props.active === 0) {
      return '';
    }
    const popup = document.getElementById('popupValue');
    const form = document.getElementById('MainForm');
    const rect = form.getBoundingClientRect();
    this.decimalPlaces = _utils_js__WEBPACK_IMPORTED_MODULE_1__.CabbageUtils.getDecimalPlaces(this.props.increment);
  
    if (popup && this.props.popup) {
      popup.textContent = this.props.valuePrefix + parseFloat(this.props.value).toFixed(this.decimalPlaces) + this.props.valuePostfix;
  
      // Calculate the position for the popup
      const sliderLeft = this.props.left;
      const sliderWidth = this.props.width;
      const formLeft = rect.left;
      const formWidth = rect.width;
  
      // Determine if the popup should be on the right or left side of the slider
      const sliderCenter = formLeft + (formWidth / 2);
      let popupLeft;
      if (sliderLeft + (sliderWidth) > sliderCenter) {
        // Place popup on the left of the slider thumb
        popupLeft = formLeft + sliderLeft - popup.offsetWidth - 10;
        console.log("Pointer on the left");
        popup.classList.add('right');
      } else {
        // Place popup on the right of the slider thumb
        popupLeft = formLeft + sliderLeft + sliderWidth + 10;
        console.log("Pointer on the right");
        popup.classList.remove('right');
      }
  
      const popupTop = rect.top + this.props.top; // Adjust top position relative to the form's top
  
      // Set the calculated position
      popup.style.left = `${popupLeft}px`;
      popup.style.top = `${popupTop}px`;
      popup.style.display = 'block';
      popup.classList.add('show');
      popup.classList.remove('hide');
    }
  }
  

  mouseLeave(evt) {
    if(this.props.active === 0) {
      return '';
    }
    if (!this.isMouseDown) {
      const popup = document.getElementById('popupValue');
      popup.classList.add('hide');
      popup.classList.remove('show');
    }
  }

  addVsCodeEventListeners(widgetDiv, vs) {
    this.vscode = vs;
    widgetDiv.addEventListener("pointerdown", this.pointerDown.bind(this));
    widgetDiv.addEventListener("mouseenter", this.mouseEnter.bind(this));
    widgetDiv.addEventListener("mouseleave", this.mouseLeave.bind(this));
    widgetDiv.HorizontalSliderInstance = this;
  }

  addEventListeners(widgetDiv) {
    widgetDiv.addEventListener("pointerdown", this.pointerDown.bind(this));
    widgetDiv.addEventListener("mouseenter", this.mouseEnter.bind(this));
    widgetDiv.addEventListener("mouseleave", this.mouseLeave.bind(this));
    widgetDiv.HorizontalSliderInstance = this;
  }

  pointerMove({ clientX }) {
    if(this.props.active === 0) {
      return '';
    }
    let textWidth = this.props.text ? _utils_js__WEBPACK_IMPORTED_MODULE_1__.CabbageUtils.getStringWidth(this.props.text, this.props) : 0;
    textWidth = this.props.sliderOffsetX > 0 ? this.props.sliderOffsetX : textWidth;
    const valueTextBoxWidth = this.props.valueTextBox ? _utils_js__WEBPACK_IMPORTED_MODULE_1__.CabbageUtils.getNumberBoxWidth(this.props) : 0;
    const sliderWidth = this.props.width - textWidth - valueTextBoxWidth;
  
    // Get the bounding rectangle of the slider
    const sliderRect = document.getElementById(this.props.channel).getBoundingClientRect();
  
    // Calculate the relative position of the mouse pointer within the slider bounds
    let offsetX = clientX - sliderRect.left - textWidth;
  
    // Clamp the mouse position to stay within the bounds of the slider
    offsetX = _utils_js__WEBPACK_IMPORTED_MODULE_1__.CabbageUtils.clamp(offsetX, 0, sliderWidth);
  
    // Calculate the new value based on the mouse position
    let newValue = _utils_js__WEBPACK_IMPORTED_MODULE_1__.CabbageUtils.map(offsetX, 0, sliderWidth, this.props.min, this.props.max);
    newValue = Math.round(newValue / this.props.increment) * this.props.increment; // Round to the nearest increment
  
    // Update the slider value
    this.props.value = newValue;
  
    // Update the slider appearance
    _utils_js__WEBPACK_IMPORTED_MODULE_1__.CabbageUtils.updateInnerHTML(this.props.channel, this.getInnerHTML());
  
    // Post message if vscode is available
    const msg = { channel: this.props.channel, value: _utils_js__WEBPACK_IMPORTED_MODULE_1__.CabbageUtils.map(this.props.value, this.props.min, this.props.max, 0, 1) }
    if (this.vscode) {
      this.vscode.postMessage({
        command: 'channelUpdate',
        text: JSON.stringify(msg)
      });
    } else {
      var message = {
        "msg": "parameterUpdate",
        "paramIdx": this.props.index,
        "value": _utils_js__WEBPACK_IMPORTED_MODULE_1__.CabbageUtils.map(this.props.value, this.props.min, this.props.max, 0, 1)
      };
      // IPlugSendMsg(message);
    }
  }
  
  handleInputChange(evt) {
    if (evt.key === 'Enter') {
      const inputValue = parseFloat(evt.target.value);
      if (!isNaN(inputValue) && inputValue >= this.props.min && inputValue <= this.props.max) {
        this.props.value = inputValue;
        _utils_js__WEBPACK_IMPORTED_MODULE_1__.CabbageUtils.updateInnerHTML(this.props.channel, this.getInnerHTML());
        widgetDiv.querySelector('input').focus();
      }
    }
  }

  getInnerHTML() {
    if(this.props.visible === 0) {
      return '';
    }
    const popup = document.getElementById('popupValue');
    if (popup) {
      popup.textContent = this.props.valuePrefix + parseFloat(this.props.value).toFixed(this.decimalPlaces) + this.props.valuePostfix;
    }

    const alignMap = {
      'left': 'start',
      'center': 'middle',
      'centre': 'middle',
      'right': 'end',
    };

    const svgAlign = alignMap[this.props.align] || this.props.align;

    // Add padding if alignment is 'end' or 'middle'
    const padding = (svgAlign === 'end' || svgAlign === 'middle') ? 5 : 0; // Adjust the padding value as needed

    // Calculate text width and update SVG width
    let textWidth = this.props.text ? _utils_js__WEBPACK_IMPORTED_MODULE_1__.CabbageUtils.getStringWidth(this.props.text, this.props) : 0;
    textWidth = (this.props.sliderOffsetX > 0 ? this.props.sliderOffsetX : textWidth) - padding;
    const valueTextBoxWidth = this.props.valueTextBox ? _utils_js__WEBPACK_IMPORTED_MODULE_1__.CabbageUtils.getNumberBoxWidth(this.props) : 0;
    const sliderWidth = this.props.width - textWidth - valueTextBoxWidth - padding; // Subtract padding from sliderWidth

    const w = (sliderWidth > this.props.height ? this.props.height : sliderWidth) * 0.75;
    const textY = this.props.height / 2 + (this.props.fontSize > 0 ? this.props.textOffsetY : 0) + (this.props.height * 0.25); // Adjusted for vertical centering
    const fontSize = this.props.fontSize > 0 ? this.props.fontSize : this.props.height * 0.8;

    textWidth += padding;

    const textElement = this.props.text ? `
      <svg x="0" y="0" width="${textWidth}" height="${this.props.height}" preserveAspectRatio="xMinYMid meet" xmlns="http://www.w3.org/2000/svg">
        <text text-anchor="${svgAlign}" x="${svgAlign === 'end' ? textWidth - padding : (svgAlign === 'middle' ? (textWidth - padding) / 2 : 0)}" y="${textY}" font-size="${fontSize}px" font-family="${this.props.fontFamily}" stroke="none" fill="${this.props.fontColour}">
          ${this.props.text}
        </text>
      </svg>
    ` : '';

    const sliderElement = `
      <svg x="${textWidth}" width="${sliderWidth}" height="${this.props.height}" fill="none" xmlns="http://www.w3.org/2000/svg">
        <rect x="1" y="${this.props.height * .2}" width="${sliderWidth - 2}" height="${this.props.height * .6}" rx="4" fill="${this.props.trackerBackgroundColour}" stroke-width="${this.props.outlineWidth}" stroke="black"/>
        <rect x="1" y="${this.props.height * .2}" width="${Math.max(0, _utils_js__WEBPACK_IMPORTED_MODULE_1__.CabbageUtils.map(this.props.value, this.props.min, this.props.max, 0, sliderWidth))}" height="${this.props.height * .6}" rx="4" fill="${this.props.trackerColour}" stroke-width="${this.props.trackerOutlineWidth}" stroke="${this.props.trackerOutlineColour}"/> 
        <rect x="${_utils_js__WEBPACK_IMPORTED_MODULE_1__.CabbageUtils.map(this.props.value, this.props.min, this.props.max, 0, sliderWidth - sliderWidth * .05 - 1) + 1}" y="0" width="${sliderWidth * .05 - 1}" height="${this.props.height}" rx="4" fill="${this.props.colour}" stroke-width="${this.props.outlineWidth}" stroke="black"/>
      </svg>
    `;

    const valueTextElement = this.props.valueTextBox ? `
      <foreignObject x="${textWidth + sliderWidth}" y="0" width="${valueTextBoxWidth}" height="${this.props.height}">
        <input type="text" value="${this.props.value.toFixed(_utils_js__WEBPACK_IMPORTED_MODULE_1__.CabbageUtils.getDecimalPlaces(this.props.increment))}"
        style="width:100%; outline: none; height:100%; text-align:center; font-size:${fontSize}px; font-family:${this.props.fontFamily}; color:${this.props.fontColour}; background:none; border:none; padding:0; margin:0;"
        onKeyDown="document.getElementById('${this.props.channel}').HorizontalSliderInstance.handleInputChange(event)"/>
      </foreignObject>
    ` : '';

    return `
      <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${this.props.width} ${this.props.height}" width="${this.props.width}" height="${this.props.height}" preserveAspectRatio="none">
        ${textElement}
        ${sliderElement}
        ${valueTextElement}
      </svg>
    `;
  }

  
  
}


/***/ }),
/* 5 */
/***/ ((__unused_webpack_module, __webpack_exports__, __webpack_require__) => {

__webpack_require__.r(__webpack_exports__);
/* harmony export */ __webpack_require__.d(__webpack_exports__, {
/* harmony export */   Cabbage: () => (/* binding */ Cabbage)
/* harmony export */ });



class Cabbage {
  static sendMidiMessageFromUI(statusByte, dataByte1, dataByte2) {
    var message = {
      "msg": "SMMFUI",
      "statusByte": statusByte,
      "dataByte1": dataByte1,
      "dataByte2": dataByte2
    };
    
    console.log("sending midi message from UI", message);
    if (typeof IPlugSendMsg === 'function')
      IPlugSendMsg(message);
  }

  static MidiMessageFromHost(statusByte, dataByte1, dataByte2) {
    console.log("Got MIDI Message" + status + ":" + dataByte1 + ":" + dataByte2);
  }

  static triggerFileOpenDialog(channel) {
    var message = {
      "msg": "fileOpen",
      "channel": channel
    };
    
    if (typeof IPlugSendMsg === 'function')
      IPlugSendMsg(message);
  }
}


function SPVFD(paramIdx, val) {
//  console.log("paramIdx: " + paramIdx + " value:" + val);
  OnParamChange(paramIdx, val);
}

function SCVFD(ctrlTag, val) {
  OnControlChange(ctrlTag, val);
//  console.log("SCVFD ctrlTag: " + ctrlTag + " value:" + val);
}

function SCMFD(ctrlTag, msgTag, msg) {
//  var decodedData = window.atob(msg);
  console.log("SCMFD ctrlTag: " + ctrlTag + " msgTag:" + msgTag + "msg:" + msg);
}

function SAMFD(msgTag, dataSize, msg) {
  //  var decodedData = window.atob(msg);
  console.log("SAMFD msgTag:" + msgTag + " msg:" + msg);
}

function SMMFD(statusByte, dataByte1, dataByte2) {
  console.log("Got MIDI Message" + status + ":" + dataByte1 + ":" + dataByte2);
}

function SSMFD(offset, size, msg) {
  console.log("Got Sysex Message");
}

// FROM UI
// data should be a base64 encoded string
function SAMFUI(msgTag, ctrlTag = -1, data = 0) {
  var message = {
    "msg": "SAMFUI",
    "msgTag": msgTag,
    "ctrlTag": ctrlTag,
    "data": data
  };
  
  IPlugSendMsg(message);
}

function SMMFUI(statusByte, dataByte1, dataByte2) {
  var message = {
    "msg": "SMMFUI",
    "statusByte": statusByte,
    "dataByte1": dataByte1,
    "dataByte2": dataByte2
  };
  
  IPlugSendMsg(message);
}

// data should be a base64 encoded string
function SSMFUI(data = 0) {
  var message = {
    "msg": "SSMFUI",
    "data": data
  };
  
  IPlugSendMsg(message);
}

function EPCFUI(paramIdx) {
  var message = {
    "msg": "EPCFUI",
    "paramIdx": paramIdx,
  };
  
  IPlugSendMsg(message);
}

function BPCFUI(paramIdx) {
  var message = {
    "msg": "BPCFUI",
    "paramIdx": paramIdx,
  };
  
  IPlugSendMsg(message);
}

function SPVFUI(paramIdx, value) {
  var message = {
    "msg": "SPVFUI",
    "paramIdx": paramIdx,
    "value": value
  };

  IPlugSendMsg(message);
}


/***/ }),
/* 6 */
/***/ ((__unused_webpack_module, __webpack_exports__, __webpack_require__) => {

__webpack_require__.r(__webpack_exports__);
/* harmony export */ __webpack_require__.d(__webpack_exports__, {
/* harmony export */   VerticalSlider: () => (/* binding */ VerticalSlider)
/* harmony export */ });
/* harmony import */ var _utils_js__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(3);


class VerticalSlider {
  constructor() {
    this.props = {
      "top": 10, // Top position of the vertical slider widget
      "left": 10, // Left position of the vertical slider widget
      "width": 60, // Width of the vertical slider widget
      "height": 60, // Height of the vertical slider widget
      "channel": "vslider", // Unique identifier for the vertical slider widget
      "min": 0, // Minimum value of the slider
      "max": 1, // Maximum value of the slider
      "value": 0, // Current value of the slider
      "defaultValue": 0, // Default value of the slider
      "skew": 1, // Skew factor for the slider
      "increment": 0.001, // Incremental value change per step
      "index": 0, // Index of the slider
      "text": "", // Text displayed on the slider
      "fontFamily": "Verdana", // Font family for the text displayed on the slider
      "fontSize": 0, // Font size for the text displayed on the slider
      "align": "centre", // Alignment of the text on the slider
      "valueTextBox": 0, // Display a textbox showing the current value
      "colour": _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageColours.getColour("blue"), // Background color of the slider
      "trackerColour": _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageColours.getColour('green'), // Color of the slider tracker
      "trackerBackgroundColour": "#ffffff", // Background color of the slider tracker
      "trackerOutlineColour": "#525252", // Outline color of the slider tracker
      "fontColour": "#dddddd", // Font color for the text displayed on the slider
      "outlineColour": "#999999", // Color of the slider outline
      "textBoxColour": "#555555", // Background color of the value textbox
      "trackerOutlineWidth": 1, // Outline width of the slider tracker
      "outlineWidth": 1, // Width of the slider outline
      "type": "vslider", // Type of the widget (vertical slider)
      "decimalPlaces": 1, // Number of decimal places in the slider value
      "velocity": 0, // Velocity value for the slider
      "visible": 1, // Visibility of the slider
      "popup": 1, // Display a popup when the slider is clicked
      "automatable": 1, // Ability to automate the slider
      "valuePrefix": "", // Prefix to be displayed before the slider value
      "valuePostfix": "", // Postfix to be displayed after the slider value
      "presetIgnore": 0, // Ignore preset value for the slider
    };


    this.panelSections = {
      "Info": ["type", "channel"],
      "Bounds": ["left", "top", "width", "height"],
      "Range": ["min", "max", "default", "skew", "increment"],
      "Text": ["text", "fontSize", "fontFamily", "fontColour", "textOffsetX", "align"],
      "Colours": ["colour", "trackerBackgroundColour", "trackerStrokeColour", "outlineColour", "textBoxOutlineColour", "textBoxColour"]
    };

    this.moveListener = this.pointerMove.bind(this);
    this.upListener = this.pointerUp.bind(this);
    this.startY = 0; // Changed from startX to startY for vertical slider
    this.startValue = 0;
    this.vscode = null;
    this.isMouseDown = false;
    this.decimalPlaces = 0;
  }

  pointerUp() {
    if (this.props.active === 0) {
      return '';
    }
    const popup = document.getElementById('popupValue');
    popup.classList.add('hide');
    popup.classList.remove('show');
    window.removeEventListener("pointermove", this.moveListener);
    window.removeEventListener("pointerup", this.upListener);
    this.isMouseDown = false;
  }

  pointerDown(evt) {
    if (this.props.active === 0) {
      return '';
    }
    let textHeight = this.props.text ? this.props.height * 0.1 : 0;
    const valueTextBoxHeight = this.props.valueTextBox ? this.props.height * 0.1 : 0;
    const sliderHeight = this.props.height - textHeight - valueTextBoxHeight;

    const sliderTop = this.props.valueTextBox ? textHeight : 0; // Adjust slider top position if valueTextBox is present

    if (evt.offsetY >= sliderTop && evt.offsetY <= sliderTop + sliderHeight) {
      this.isMouseDown = true;
      this.startY = evt.offsetY - sliderTop;
      this.props.value = _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageUtils.map(this.startY, 5, sliderHeight, this.props.max, this.props.min);
      this.props.value = Math.round(this.props.value / this.props.increment) * this.props.increment;
      this.startValue = this.props.value;
      window.addEventListener("pointermove", this.moveListener);
      window.addEventListener("pointerup", this.upListener);
      Cabbage.updateInnerHTML(this.props.channel, this);
    }
  }


  mouseEnter(evt) {
    if (this.props.active === 0) {
      return '';
    }

    const popup = document.getElementById('popupValue');
    const form = document.getElementById('MainForm');
    const rect = form.getBoundingClientRect();
    this.decimalPlaces = _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageUtils.getDecimalPlaces(this.props.increment);

    if (popup) {
      popup.textContent = this.props.valuePrefix + parseFloat(this.props.value).toFixed(this.decimalPlaces) + this.props.valuePostfix;

      // Calculate the position for the popup
      const sliderLeft = this.props.left;
      const sliderWidth = this.props.width;
      const formLeft = rect.left;
      const formWidth = rect.width;

      // Determine if the popup should be on the right or left side of the slider
      const sliderCenter = formLeft + (formWidth / 2);
      let popupLeft;
      if (sliderLeft + (sliderWidth) > sliderCenter) {
        // Place popup on the left of the slider thumb
        popupLeft = formLeft + sliderLeft - popup.offsetWidth - 10;
        console.log("Pointer on the left");
        popup.classList.add('right');
      } else {
        // Place popup on the right of the slider thumb
        popupLeft = formLeft + sliderLeft + sliderWidth + 10;
        console.log("Pointer on the right");
        popup.classList.remove('right');
      }

      const popupTop = rect.top + this.props.top + this.props.height * .45; // Adjust top position relative to the form's top

      // Set the calculated position
      popup.style.left = `${popupLeft}px`;
      popup.style.top = `${popupTop}px`;
      popup.style.display = 'block';
      popup.classList.add('show');
      popup.classList.remove('hide');
    }
  }


  mouseLeave(evt) {
    if (!this.isMouseDown) {
      const popup = document.getElementById('popupValue');
      popup.classList.add('hide');
      popup.classList.remove('show');
    }
  }

  addVsCodeEventListeners(widgetDiv, vs) {
    this.vscode = vs;
    widgetDiv.addEventListener("pointerdown", this.pointerDown.bind(this));
    widgetDiv.addEventListener("mouseenter", this.mouseEnter.bind(this));
    widgetDiv.addEventListener("mouseleave", this.mouseLeave.bind(this));
    widgetDiv.VerticalSliderInstance = this;
  }

  addEventListeners(widgetDiv) {
    widgetDiv.addEventListener("pointerdown", this.pointerDown.bind(this));
    widgetDiv.addEventListener("mouseenter", this.mouseEnter.bind(this));
    widgetDiv.addEventListener("mouseleave", this.mouseLeave.bind(this));
    widgetDiv.VerticalSliderInstance = this;
  }

  handleInputChange(evt) {
    if (evt.key === 'Enter') {
      const inputValue = parseFloat(evt.target.value);
      if (!isNaN(inputValue) && inputValue >= this.props.min && inputValue <= this.props.max) {
        this.props.value = inputValue;
        const widgetDiv = document.getElementById(this.props.channel);
        widgetDiv.innerHTML = this.getInnerHTML();
        widgetDiv.querySelector('input').focus();
      }
    }
  }

  pointerMove({ clientY }) {
    if (this.props.active === 0) {
      return '';
    }

    let textHeight = this.props.text ? this.props.height * 0.1 : 0;
    const valueTextBoxHeight = this.props.valueTextBox ? this.props.height * 0.1 : 0;
    const sliderHeight = this.props.height - textHeight - valueTextBoxHeight;

    // Get the bounding rectangle of the slider
    const sliderRect = document.getElementById(this.props.channel).getBoundingClientRect();

    // Calculate the relative position of the mouse pointer within the slider bounds
    let offsetY = sliderRect.bottom - clientY - textHeight;

    // Clamp the mouse position to stay within the bounds of the slider
    offsetY = _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageUtils.clamp(offsetY, 0, sliderHeight);

    // Calculate the new value based on the mouse position
    let newValue = _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageUtils.map(offsetY, 0, sliderHeight, this.props.min, this.props.max);
    newValue = Math.round(newValue / this.props.increment) * this.props.increment;

    // Update the slider value
    this.props.value = newValue;

    // Update the slider appearance
    const widgetDiv = document.getElementById(this.props.channel);
    widgetDiv.innerHTML = this.getInnerHTML();

    // Post message if vscode is available
    const msg = { channel: this.props.channel, value: _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageUtils.map(this.props.value, this.props.min, this.props.max, 0, 1) }
    if (this.vscode) {
      this.vscode.postMessage({
        command: 'channelUpdate',
        text: JSON.stringify(msg)
      });
    } else {
      var message = {
        "msg": "parameterUpdate",
        "paramIdx": this.props.index,
        "value": _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageUtils.map(this.props.value, this.props.min, this.props.max, 0, 1)
      };
      // IPlugSendMsg(message);
    }
  }

  getInnerHTML() {
    if (this.props.visible === 0) {
      return '';
    }

    const popup = document.getElementById('popupValue');
    if (popup) {
      popup.textContent = this.props.valuePrefix + parseFloat(this.props.value).toFixed(this.decimalPlaces) + this.props.valuePostfix;
    }

    const alignMap = {
      'left': 'start',
      'center': 'middle',
      'centre': 'middle',
      'right': 'end',
    };

    const svgAlign = alignMap[this.props.align] || this.props.align;

    // Calculate text height
    let textHeight = this.props.text ? this.props.height * 0.1 : 0;
    const valueTextBoxHeight = this.props.valueTextBox ? this.props.height * 0.1 : 0;
    const sliderHeight = this.props.height - textHeight - valueTextBoxHeight * 1.1;

    const textX = this.props.width / 2;
    const fontSize = this.props.fontSize > 0 ? this.props.fontSize : this.props.width * 0.3;

    const thumbHeight = sliderHeight * 0.05;

    const textElement = this.props.text ? `
    <svg x="0" y="${this.props.valueTextBox ? 0 : this.props.height - textHeight}" width="${this.props.width}" height="${textHeight + 5}" preserveAspectRatio="xMinYMid meet" xmlns="http://www.w3.org/2000/svg">
      <text text-anchor="${svgAlign}" x="${textX}" y="${textHeight}" font-size="${fontSize}px" font-family="${this.props.fontFamily}" stroke="none" fill="${this.props.fontColour}">
        ${this.props.text}
      </text>
    </svg>
  ` : '';

    const sliderElement = `
    <svg x="0" y="${this.props.valueTextBox ? textHeight + 2 : 0}" width="${this.props.width}" height="${sliderHeight}" fill="none" xmlns="http://www.w3.org/2000/svg">
      <rect x="${this.props.width * 0.4}" y="1" width="${this.props.width * 0.2}" height="${sliderHeight * 0.95}" rx="2" fill="${this.props.trackerBackgroundColour}" stroke-width="${this.props.outlineWidth}" stroke="black"/>
      <rect x="${this.props.width * 0.4}" y="${sliderHeight - _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageUtils.map(this.props.value, this.props.min, this.props.max, 0, sliderHeight * 0.95) - 1}" height="${_utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageUtils.map(this.props.value, this.props.min, this.props.max, 0, 1) * sliderHeight * 0.95}" width="${this.props.width * 0.2}" rx="2" fill="${this.props.trackerColour}" stroke-width="${this.props.trackerOutlineWidth}" stroke="${this.props.trackerOutlineColour}"/> 
      <rect x="${this.props.width * 0.3}" y="${sliderHeight - _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageUtils.map(this.props.value, this.props.min, this.props.max, thumbHeight + 1, sliderHeight - 1)}" width="${this.props.width * 0.4}" height="${thumbHeight}" rx="2" fill="${this.props.colour}" stroke-width="${this.props.outlineWidth}" stroke="black"/>
    </svg>
  `;

    const valueTextElement = this.props.valueTextBox ? `
    <foreignObject x="0" y="${this.props.height - valueTextBoxHeight + 2}" width="${this.props.width}" height="${valueTextBoxHeight}">
      <input type="text" value="${this.props.value.toFixed(_utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageUtils.getDecimalPlaces(this.props.increment))}"
      style="width:100%; outline: none; height:100%; text-align:center; font-size:${fontSize}px; font-family:${this.props.fontFamily}; color:${this.props.fontColour}; background:none; border:none; padding:0; margin:0;"
      onKeyDown="document.getElementById('${this.props.channel}').VerticalSliderInstance.handleInputChange(event)"/>
    </foreignObject>
  ` : '';

    return `
    <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${this.props.width} ${this.props.height}" width="${this.props.width}" height="${this.props.height}" preserveAspectRatio="none">
      ${textElement}
      ${sliderElement}
      ${valueTextElement}
    </svg>
  `;
  }




}


/***/ }),
/* 7 */
/***/ ((__unused_webpack_module, __webpack_exports__, __webpack_require__) => {

__webpack_require__.r(__webpack_exports__);
/* harmony export */ __webpack_require__.d(__webpack_exports__, {
/* harmony export */   Button: () => (/* binding */ Button),
/* harmony export */   FileButton: () => (/* binding */ FileButton)
/* harmony export */ });
/* harmony import */ var _utils_js__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(3);
/* harmony import */ var _cabbage_js__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(5);



class Button {
  constructor() {
    this.props = {
      "top": 10, // Top position of the button
      "left": 10, // Left position of the button
      "width": 80, // Width of the button
      "height": 30, // Height of the button
      "channel": "button", // Unique identifier for the button
      "corners": 2, // Radius of the corners of the button rectangle
      "min": 0, // Minimum value for the button (for sliders)
      "max": 1, // Maximum value for the button (for sliders)
      "value": 0, // Current value of the button (for sliders)
      "defaultValue": 0, // Default value of the button
      "index": 0, // Index of the button
      "textOn": "On", // Text displayed when button is in the 'On' state
      "textOff": "Off", // Text displayed when button is in the 'Off' state
      "fontFamily": "Verdana", // Font family for the text
      "fontSize": 0, // Font size for the text (0 for automatic)
      "align": "centre", // Text alignment within the button (left, center, right)
      "colourOn": _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageColours.getColour("blue"), // Background color of the button in the 'On' state
      "colourOff": _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageColours.getColour("blue"), // Background color of the button in the 'Off' state
      "fontColourOn": "#dddddd", // Color of the text in the 'On' state
      "fontColourOff": "#dddddd", // Color of the text in the 'Off' state
      "outlineColour": "#dddddd", // Color of the outline
      "outlineWidth": 2, // Width of the outline
      "name": "", // Name of the button
      "value": 0, // Value of the button (0 for off, 1 for on)
      "type": "button", // Type of the button (button)
      "visible": 1, // Visibility of the button (0 for hidden, 1 for visible)
      "automatable": 1, // Whether the button value can be automated (0 for no, 1 for yes)
      "presetIgnore": 0 // Whether the button should be ignored in presets (0 for no, 1 for yes)
  };
  

    this.panelSections = {
      "Info": ["type", "channel"],
      "Bounds": ["left", "top", "width", "height"],
      "Text": ["textOn", "textOff", "fontSize", "fontFamily", "fontColourOn", "fontColourOff", "align"], // Changed from textOffsetY to textOffsetX for vertical slider
      "Colours": ["colourOn", "colourOff", "outlineColour"]
    };

    this.vscode = null;
    this.isMouseDown = false;
    this.isMouseInside = false;
    this.state = false;
  }

  pointerUp() {
    if (this.props.active === 0) {
      return '';
    }
    this.isMouseDown = false;
  }

  pointerDown(evt) {
    if (this.props.active === 0) {
      return '';
    }
    console.log("pointerDown");
    this.isMouseDown = true;
    this.state =! this.state;
    _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageUtils.updateInnerHTML(this.props.channel, this);
  }


  pointerEnter(evt) {
    if (this.props.active === 0) {
      return '';
    }
    this.isMouseOver = true;
    _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageUtils.updateInnerHTML(this.props.channel, this);
  }


  pointerLeave(evt) {
    if (this.props.active === 0) {
      return '';
    }
    this.isMouseOver = false;
    _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageUtils.updateInnerHTML(this.props.channel, this);
  }

  //adding this at the window level to check if the mouse is inside the widget
  handleMouseMove(evt) {
    const rect = document.getElementById(this.props.channel).getBoundingClientRect();
    const isInside = (
      evt.clientX >= rect.left &&
      evt.clientX <= rect.right &&
      evt.clientY >= rect.top &&
      evt.clientY <= rect.bottom
    );

    if (!isInside) {
      this.isMouseInside = false;
    } else {
      this.isMouseInside = true;
    }

    _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageUtils.updateInnerHTML(this.props.channel, this);
  }


  addVsCodeEventListeners(widgetDiv, vs) {
    console.log("addVsCodeEventListeners");
    this.vscode = vs;
    widgetDiv.addEventListener("pointerdown", this.pointerUp.bind(this));
    widgetDiv.addEventListener("pointerup", this.pointerDown.bind(this));
    window.addEventListener("mousemove", this.handleMouseMove.bind(this));
    widgetDiv.VerticalSliderInstance = this;
  }

  addEventListeners(widgetDiv) {
    widgetDiv.addEventListener("pointerup", this.pointerUp.bind(this));
    widgetDiv.addEventListener("pointerdown", this.pointerDown.bind(this));
    window.addEventListener("mousemove", this.handleMouseMove.bind(this));
    widgetDiv.VerticalSliderInstance = this;
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
    const padding = 5;
  
    let textX;
    if (this.props.align === 'left') {
      textX = this.props.corners; // Add padding for left alignment
    } else if (this.props.align === 'right') {
      textX = this.props.width - this.props.corners - padding; // Add padding for right alignment
    } else {
      textX = this.props.width / 2;
    }
  
    const currentColour = _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageColours.darker(this.state ? this.props.colourOn : this.props.colourOff, this.isMouseInside ? 0.2 : 0);
    return `
      <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${this.props.width} ${this.props.height}" width="${this.props.width}" height="${this.props.height}" preserveAspectRatio="none">
          <rect x="${this.props.corners/2}" y="${this.props.corners/2}" width="${this.props.width-this.props.corners*2}" height="${this.props.height-this.props.corners*2}" fill="${currentColour}" stroke="${this.props.outlineColour}"
            stroke-width="${this.props.outlineWidth}" rx="${this.props.corners}" ry="${this.props.corners}"></rect>
          <text x="${textX}" y="${this.props.height / 2}" font-family="${this.props.fontFamily}" font-size="${fontSize}"
            fill="${this.state ? this.props.fontColourOn : this.props.fontColourOff}" text-anchor="${svgAlign}" alignment-baseline="middle">${this.state ? this.props.textOn : this.props.textOff}</text>
      </svg>
    `;
  }
}

class FileButton extends Button {
  constructor() {
    super();
    this.props.channel = "fileButton";
    delete this.props.textOn;
    delete this.props.textOff;
    delete this.props.colourOn;
    delete this.props.fontColourOff;
    delete this.props.fontColourOn;
    this.props["fontColour"] = "#dddddd";
    this.props["text"] = "Choose File";
    this.props["colour"] = _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageColours.getColour("blue");
  }

  pointerDown(evt) {
    if (this.props.active === 0) {
      return '';
    }
    this.isMouseDown = true;
    this.state =! this.state;
    _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageUtils.updateInnerHTML(this.props.channel, this);
    _cabbage_js__WEBPACK_IMPORTED_MODULE_1__.Cabbage.triggerFileOpenDialog(this.props.channel);
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
    const padding = 5;
  
    let textX;
    if (this.props.align === 'left') {
      textX = this.props.corners; // Add padding for left alignment
    } else if (this.props.align === 'right') {
      textX = this.props.width - this.props.corners - padding; // Add padding for right alignment
    } else {
      textX = this.props.width / 2;
    }
  
    const currentColour = _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageColours.darker(this.props.colour, this.isMouseInside ? 0.2 : 0);
    return `
      <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${this.props.width} ${this.props.height}" width="${this.props.width}" height="${this.props.height}" preserveAspectRatio="none">
          <rect x="${this.props.corners/2}" y="${this.props.corners/2}" width="${this.props.width-this.props.corners*2}" height="${this.props.height-this.props.corners*2}" fill="${currentColour}" stroke="${this.props.outlineColour}"
            stroke-width="${this.props.outlineWidth}" rx="${this.props.corners}" ry="${this.props.corners}"></rect>
          <text x="${textX}" y="${this.props.height / 2}" font-family="${this.props.fontFamily}" font-size="${fontSize}"
            fill="${this.props.fontColour}" text-anchor="${svgAlign}" alignment-baseline="middle">${this.props.text}</text>
      </svg>
    `;
  }
}


/***/ }),
/* 8 */
/***/ ((__unused_webpack_module, __webpack_exports__, __webpack_require__) => {

__webpack_require__.r(__webpack_exports__);
/* harmony export */ __webpack_require__.d(__webpack_exports__, {
/* harmony export */   Checkbox: () => (/* binding */ Checkbox)
/* harmony export */ });
/* harmony import */ var _utils_js__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(3);


class Checkbox {
  constructor() {
    this.props = {
        "top": 10, // Top position of the checkbox
        "left": 10, // Left position of the checkbox
        "width": 100, // Width of the checkbox
        "height": 30, // Height of the checkbox
        "channel": "checkbox", // Unique identifier for the checkbox
        "corners": 2, // Radius of the corners of the checkbox rectangle
        "min": 0, // Minimum value for the checkbox (for sliders)
        "max": 1, // Maximum value for the checkbox (for sliders)
        "value": 0, // Current value of the checkbox (for sliders)
        "defaultValue": 0, // Default value of the checkbox
        "index": 0, // Index of the checkbox
        "text": "On/Off", // Text displayed next to the checkbox
        "fontFamily": "Verdana", // Font family for the text
        "fontColour": "#dddddd", // Color of the text
        "fontSize": 14, // Font size for the text
        "align": "left", // Text alignment within the checkbox (left, center, right)
        "colourOn": _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageColours.getColour("green"), // Background color of the checkbox in the 'On' state
        "colourOff": "#ffffff", // Background color of the checkbox in the 'Off' state
        "fontColourOn": "#dddddd", // Color of the text in the 'On' state
        "fontColourOff": "#000000", // Color of the text in the 'Off' state
        "outlineColour": "#999999", // Color of the outline
        "outlineWidth": 1, // Width of the outline
        "value": 0, // Value of the checkbox (0 for off, 1 for on)
        "type": "checkbox", // Type of the checkbox (checkbox)
        "visible": 1, // Visibility of the checkbox (0 for hidden, 1 for visible)
        "automatable": 1, // Whether the checkbox value can be automated (0 for no, 1 for yes)
        "presetIgnore": 0 // Whether the checkbox should be ignored in presets (0 for no, 1 for yes)
    };
    
    this.panelSections = {
        "Info": ["type", "channel"],
        "Bounds": ["left", "top", "width", "height"],
        "Text": ["text", "fontSize", "fontFamily", "fontColour", "align"], // Changed from textOffsetY to textOffsetX for vertical slider
        "Colours": ["colourOn", "colourOff", "outlineColour"]
      };

    this.vscode = null;
    this.isChecked = false;
  }

  toggle() {
    if (this.props.active === 0) {
      return '';
    }
    this.isChecked = !this.isChecked;
    _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageUtils.updateInnerHTML(this.props.channel, this);
  }



  pointerDown(evt) {
    this.toggle();
  }

  addVsCodeEventListeners(widgetDiv, vscode) {
    this.vscode = vscode;
    widgetDiv.addEventListener("pointerdown", this.pointerDown.bind(this));
    widgetDiv.VerticalSliderInstance = this;
  }

  addEventListeners(widgetDiv) {
    widgetDiv.addEventListener("pointerdown", this.pointerDown.bind(this));
    widgetDiv.VerticalSliderInstance = this;
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
  
    const checkboxSize = this.props.height * 0.8;
    const checkboxX = this.props.align === 'right' ? this.props.width - checkboxSize - this.props.corners : this.props.corners;
    const textX = this.props.align === 'right' ? checkboxX - 10 : checkboxX + checkboxSize + 10; // Add more padding to prevent overlap
  
    const adjustedTextAnchor = this.props.align === 'right' ? 'end' : 'start';
  
    return `
      <svg id="${this.props.channel}-svg" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${this.props.width} ${this.props.height}" width="${this.props.width}" height="${this.props.height}" preserveAspectRatio="none">
        <rect x="${checkboxX}" y="${(this.props.height - checkboxSize) / 2}" width="${checkboxSize}" height="${checkboxSize}" fill="${this.isChecked ? this.props.colourOn : this.props.colourOff}" stroke="${this.props.outlineColour}" stroke-width="${this.props.outlineWidth}" rx="${this.props.corners}" ry="${this.props.corners}"></rect>
        <text x="${textX}" y="${this.props.height / 2}" font-family="${this.props.fontFamily}" font-size="${fontSize}" fill="${this.props.fontColour}" text-anchor="${adjustedTextAnchor}" alignment-baseline="middle">${this.props.text}</text>
      </svg>
    `;
  }
  
}


/***/ }),
/* 9 */
/***/ ((__unused_webpack_module, __webpack_exports__, __webpack_require__) => {

__webpack_require__.r(__webpack_exports__);
/* harmony export */ __webpack_require__.d(__webpack_exports__, {
/* harmony export */   ComboBox: () => (/* binding */ ComboBox)
/* harmony export */ });
/* harmony import */ var _utils_js__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(3);


class ComboBox {
    constructor() {
        this.props = {
            "top": 10, // Top position of the widget
            "left": 10, // Left position of the widget
            "width": 100, // Width of the widget
            "height": 30, // Height of the widget
            "channel": "comboBox", // Unique identifier for the widget
            "corners": 4, // Radius of the corners of the widget rectangle
            "defaultValue": 0, // Default value index for the dropdown items
            "fontFamily": "Verdana", // Font family for the text
            "fontSize": 14, // Font size for the text
            "align": "center", // Text alignment within the widget (left, center, right)
            "colour": _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageColours.getColour("blue"), // Background color of the widget
            "items": "One, Two, Three", // List of items for the dropdown
            "text": "Select", // Default text displayed when no item is selected
            "fontColour": "#dddddd", // Color of the text
            "outlineColour": "#dddddd", // Color of the outline
            "outlineWidth": 2, // Width of the outline
            "visible": 1, // Visibility of the widget (0 for hidden, 1 for visible)
            "type": "combobox", // Type of the widget (combobox)
            "value": 0, // Value of the widget
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
        this.selectedItem = this.props.value > 0 ? this.items[this.props.defaultValue] : this.props.text;

        this.vscode = null;
    }

    pointerDown(evt) {
        if (this.props.active === 0) {
            return '';
        }
        console.log("Pointer down");
        this.isOpen = !this.isOpen;
        this.isMouseInside = true;
        _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageUtils.updateInnerHTML(this.props.channel, this);

        if (this.firstOpen) {
            this.isOpen = true;
            this.firstOpen = false; // Update the flag after the first open
        } else {
            // Check if the event target is a dropdown item
            const selectedItem = evt.target.getAttribute("data-item");
            if (selectedItem) {
                console.log("Item clicked:", selectedItem);
                this.selectedItem = selectedItem;
                this.isOpen = false;
                const widgetDiv = _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageUtils.getWidgetDiv(this.props.channel);
                widgetDiv.style.transform = 'translate(' + this.props.left + 'px,' + this.props.top + 'px)';
                _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageUtils.updateInnerHTML(this.props.channel, this);
            }
        }
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
        // Get the widget div
        const widgetDiv = _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageUtils.getWidgetDiv(this.props.channel);
    
        // Check if the target of the click event is outside of the widget div
        if (!widgetDiv.contains(event.target)) {
            // Close the dropdown menu
            this.isOpen = false;
            // Update the HTML
            const widgetDiv = _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageUtils.getWidgetDiv(this.props.channel);
                widgetDiv.style.transform = 'translate(' + this.props.left + 'px,' + this.props.top + 'px)';
                _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageUtils.updateInnerHTML(this.props.channel, this);
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
            const mainForm = _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageUtils.getWidgetDiv("MainForm");
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
                        fill="${_utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageColours.darker(this.props.colour, 0.2)}" rx="0" ry="0"
                        style="cursor: pointer;" pointer-events="all" data-item="${item}"
                        onmouseover="this.setAttribute('fill', '${_utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageColours.brighter(this.props.colour, 0.2)}')"
                        onmouseout="this.setAttribute('fill', '${_utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageColours.darker(this.props.colour, 0.2)}')"></rect>
                    <text x="${(this.props.width - this.props.corners / 2) / 2}" y="${(index + 1) * itemHeight - itemHeight / 2}"
                        font-family="${this.props.fontFamily}" font-size="${this.props.fontSize}" fill="${this.props.fontColour}"
                        text-anchor="middle" alignment-baseline="middle" data-item="${item}"
                        style="cursor: pointer;" pointer-events="all"
                        onmouseover="this.previousElementSibling.setAttribute('fill', '${_utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageColours.brighter(this.props.colour, 0.2)}')"
                        onmouseout="this.previousElementSibling.setAttribute('fill', '${_utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageColours.darker(this.props.colour, 0.2)}')"
                        onmousedown="document.getElementById('${this.props.channel}').ComboBoxInstance.handleItemClick('${item}')">${item}</text>
                `;
            });
        }
    
        // Adjusting the position of the arrow
        const arrowWidth = 10; // Width of the arrow
        const arrowHeight = 6; // Height of the arrow
        const arrowX = this.props.width - arrowWidth - this.props.corners / 2 - 10; // Decreasing arrowX value to move the arrow more to the left
        const arrowY = (this.props.height - arrowHeight) / 2; // Y-coordinate of the arrow
    
        // Positioning the selected item text within the main rectangle
        let selectedItemTextX;
        if (svgAlign === 'middle') {
            selectedItemTextX = (this.props.width - arrowWidth - this.props.corners / 2) / 2;
        } else {
            const selectedItemWidth = _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageUtils.getStringWidth(this.selectedItem, this.props);
            const textPadding = svgAlign === 'start' ? - this.props.width * .1 : - this.props.width * .05;
            selectedItemTextX = svgAlign === 'start' ? (this.props.width - this.props.corners / 2) / 2 - selectedItemWidth / 2 + textPadding : (this.props.width - this.props.corners / 2) / 2 + selectedItemWidth / 2 + textPadding;
        }
        const selectedItemTextY = this.props.height / 2;
    
        return `
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${this.props.width} ${totalHeight}" width="${this.props.width}" height="${totalHeight}" preserveAspectRatio="none">
                <rect x="${this.props.corners / 2}" y="${this.props.corners / 2}" width="${this.props.width - this.props.corners}" height="${this.props.height - this.props.corners * 2}" fill="${this.props.colour}" stroke="${this.props.outlineColour}"
                    stroke-width="${this.props.outlineWidth}" rx="${this.props.corners}" ry="${this.props.corners}" 
                    style="cursor: pointer;" pointer-events="all" 
                    onmousedown="document.getElementById('${this.props.channel}').ComboBoxInstance.pointerDown()"></rect>
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


/***/ }),
/* 10 */
/***/ ((__unused_webpack_module, __webpack_exports__, __webpack_require__) => {

__webpack_require__.r(__webpack_exports__);
/* harmony export */ __webpack_require__.d(__webpack_exports__, {
/* harmony export */   Label: () => (/* binding */ Label)
/* harmony export */ });
/**
 * Label class
 */
class Label {
    constructor() {
        this.props = {
            "top": 0,
            "left": 0,
            "width": 100,
            "height": 30,
            "type": "label",
            "colour": "#888888",
            "channel": "label",
            "fontColour": "#dddddd",
            "fontFamily": "Verdana",
            "fontSize": 0,
            "corners": 4,
            "align": "centre",
            "visible": 1,
            "text": "Default Label"
        }

        this.panelSections = {
            "Properties": ["type"],
            "Bounds": ["left", "top", "width", "height"],
            "Text": ["text", "fontColour", "fontSize", "fontFamily", "align"],
            "Colours": ["colour"]
        };
    }

    addVsCodeEventListeners(widgetDiv, vs) {
        this.vscode = vs;
        widgetDiv.addEventListener("pointerdown", this.pointerDown.bind(this));
    }

    addEventListeners(widgetDiv) {
        widgetDiv.addEventListener("pointerdown", this.pointerDown.bind(this));
    }

    pointerDown() {
        console.log("Label clicked!");
    }

    getInnerHTML() {
        if (this.props.visible === 0) {
            return '';
        }
        
        const fontSize = this.props.fontSize > 0 ? this.props.fontSize : Math.max(this.props.height * 0.8, 12); // Ensuring font size doesn't get too small
        const alignMap = {
            'left': 'end',
            'center': 'middle',
            'centre': 'middle',
            'right': 'start',
        };
        const svgAlign = alignMap[this.props.align] || 'middle';
    
        return `
            <div style="position: relative; width: 100%; height: 100%;">
                <!-- Background SVG with preserveAspectRatio="none" -->
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${this.props.width} ${this.props.height}" width="100%" height="100%" preserveAspectRatio="none"
                     style="position: absolute; top: 0; left: 0;">
                    <rect width="${this.props.width}" height="${this.props.height}" x="0" y="0" rx="${this.props.corners}" ry="${this.props.corners}" fill="${this.props.colour}" 
                        pointer-events="all"></rect>
                </svg>
    
                <!-- Text SVG with proper alignment -->
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${this.props.width} ${this.props.height}" width="100%" height="100%" preserveAspectRatio="xMidYMid meet"
                     style="position: absolute; top: 0; left: 0;">
                    <text x="${this.props.align === 'left' ? '10%' : this.props.align === 'right' ? '90%' : '50%'}" y="50%" font-family="${this.props.fontFamily}" font-size="${fontSize}"
                        fill="${this.props.fontColour}" text-anchor="${svgAlign}" dominant-baseline="middle" alignment-baseline="middle" 
                        style="pointer-events: none;">${this.props.text}</text>
                </svg>
            </div>
        `;
    }
    
    
    
}


/***/ }),
/* 11 */
/***/ ((__unused_webpack_module, __webpack_exports__, __webpack_require__) => {

__webpack_require__.r(__webpack_exports__);
/* harmony export */ __webpack_require__.d(__webpack_exports__, {
/* harmony export */   CsoundOutput: () => (/* binding */ CsoundOutput)
/* harmony export */ });
/* harmony import */ var _utils_js__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(3);


/**
 * CsoundOutput class
 */
class CsoundOutput {
    constructor() {
        this.props = {
            "top": 0,
            "left": 0,
            "width": 200,
            "height": 300,
            "type": "label",
            "colour": "#000000",
            "channel": "csoundoutput",
            "fontColour": "#dddddd",
            "fontFamily": "Verdana",
            "fontSize": 14,
            "corners": 4,
            "align": "left",
            "visible": 1,
            "text": "Csound Output\n"
        };

        this.panelSections = {
            "Properties": ["type"],
            "Bounds": ["left", "top", "width", "height"],
            "Text": ["text", "fontColour", "fontSize", "fontFamily", "align"],
            "Colours": ["colour"]
        };
    }

    addVsCodeEventListeners(widgetDiv, vs) {
        this.vscode = vs;
    }

    addEventListeners(widgetDiv) {
        // Add any necessary event listeners here
    }

    getInnerHTML() {
        if (this.props.visible === 0) {
            return '';
        }

        const fontSize = this.props.fontSize > 0 ? this.props.fontSize : Math.max(this.props.height * 0.8, 12); // Ensuring font size doesn't get too small
        const alignMap = {
            'left': 'start',
            'center': 'center',
            'centre': 'center',
            'right': 'end',
        };
        const textAlign = alignMap[this.props.align] || 'start';

        return `
                <textarea style="width: 100%; height: 100%; background-color: ${this.props.colour}; 
                color: ${this.props.fontColour}; font-family: ${this.props.fontFamily}; font-size: ${fontSize}px; 
                text-align: ${textAlign}; padding: 10px; box-sizing: border-box; border: none; resize: none;">
${this.props.text}
                </textarea>
        `;
    }

    appendText(newText) {
        this.props.text += newText + '\n';
        const widgetDiv = _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageUtils.getWidgetDiv(this.props.channel);

        if (widgetDiv) {
            const textarea = widgetDiv.querySelector('textarea');
            if (textarea) {
                textarea.value += newText + '\n';
                console.log(textarea.value);
                textarea.scrollTop = textarea.scrollHeight; // Scroll to the bottom
            }
        }
    }
}


/***/ }),
/* 12 */
/***/ ((__unused_webpack_module, __webpack_exports__, __webpack_require__) => {

__webpack_require__.r(__webpack_exports__);
/* harmony export */ __webpack_require__.d(__webpack_exports__, {
/* harmony export */   MidiKeyboard: () => (/* binding */ MidiKeyboard)
/* harmony export */ });
/* harmony import */ var _cabbage_js__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(5);
/* harmony import */ var _utils_js__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(3);



/**
 * Form class
 */
class MidiKeyboard {
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
      "defaultValue": "36", // The leftmost note of the keyboard
      "fontFamily": "Verdana", // Font family for the text displayed on the keyboard
      "whiteNoteColour": "#fff", // Color of the white keys on the keyboard
      "keySeparatorColour": "#000", // Color of the separators between keys
      "arrowBackgroundColour": "#0295cf", // Background color of the arrow keys
      "mouseoverKeyColour": _utils_js__WEBPACK_IMPORTED_MODULE_1__.CabbageColours.getColour('green'), // Color of keys when hovered over
      "keydownColour": _utils_js__WEBPACK_IMPORTED_MODULE_1__.CabbageColours.getColour('green'), // Color of keys when pressed
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
      _cabbage_js__WEBPACK_IMPORTED_MODULE_0__.Cabbage.sendMidiMessageFromUI(0x90, this.noteMap[e.target.dataset.note], 127);
    }
  }


  pointerUp(e) {
    if (this.isMouseDown) {
      this.isMouseDown = false;
      if (e.target.classList.contains('white-key') || e.target.classList.contains('black-key')) {
        e.target.setAttribute('fill', e.target.classList.contains('white-key') ? this.props.whiteNoteColour : this.props.blackNoteColour);
        console.log(`Key up: ${this.noteMap[e.target.dataset.note]}`);
        _cabbage_js__WEBPACK_IMPORTED_MODULE_0__.Cabbage.sendMidiMessageFromUI(0x80, this.noteMap[e.target.dataset.note], 127);
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
    _utils_js__WEBPACK_IMPORTED_MODULE_1__.CabbageUtils.updateInnerHTML(this.props.channel, this);
  }

  addVsCodeEventListeners(widgetDiv, vscode) {
    this.vscode = vscode;
    this.addListeners(widgetDiv)
    _utils_js__WEBPACK_IMPORTED_MODULE_1__.CabbageUtils.updateInnerHTML(this.props.channel, this);
  }

  addEventListeners(widgetDiv) {
    this.addListeners(widgetDiv)
    _utils_js__WEBPACK_IMPORTED_MODULE_1__.CabbageUtils.updateInnerHTML(this.props.channel, this);
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


/***/ }),
/* 13 */
/***/ ((__unused_webpack_module, __webpack_exports__, __webpack_require__) => {

__webpack_require__.r(__webpack_exports__);
/* harmony export */ __webpack_require__.d(__webpack_exports__, {
/* harmony export */   Form: () => (/* binding */ Form)
/* harmony export */ });
/**
 * Form class
 */
class Form {
  constructor() {
    this.props = {
      "width": 600,
      "height": 300,
      "caption": "",
      "type": "form",
      "colour": "#888888",
      "channel": "MainForm"
    }

    this.panelSections = {
      "Properties": ["type"],
      "Bounds": ["width", "height"],
      "Text": ["caption"],
      "Colours": ["colour"]
    };
  }


  getInnerHTML() {
    return `
      <svg class="widget-svg" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${this.props.width} ${this.props.height}" width="100%" height="100%" preserveAspectRatio="none" style="position: relative; z-index: 0;">
        <rect width="${this.props.width}" height="${this.props.height}" x="0" y="0" rx="2" ry="2" fill="${this.props.colour}" />
      </svg>
    `;
  }

  updateSVG() {
    // Select the parent div using the channel property
    const parentDiv = document.getElementById(this.props.channel);
    
    if (!parentDiv) {
      console.error(`Parent div with id ${this.props.channel} not found.`);
      return;
    }

    // Check if an SVG element with the class 'widget-svg' already exists
    let svgElement = parentDiv.querySelector('.widget-svg');
    
    if (svgElement) {
      // Update the existing SVG element's outerHTML
      svgElement.outerHTML = this.getInnerHTML();
    } else {
      // Append the new SVG element if it doesn't exist
      parentDiv.insertAdjacentHTML('beforeend', this.getInnerHTML());
    }
  }
}


/***/ }),
/* 14 */
/***/ ((module) => {

module.exports = require("child_process");

/***/ }),
/* 15 */
/***/ ((module, __unused_webpack_exports, __webpack_require__) => {



const WebSocket = __webpack_require__(16);

WebSocket.createWebSocketStream = __webpack_require__(36);
WebSocket.Server = __webpack_require__(37);
WebSocket.Receiver = __webpack_require__(30);
WebSocket.Sender = __webpack_require__(33);

WebSocket.WebSocket = WebSocket;
WebSocket.WebSocketServer = WebSocket.Server;

module.exports = WebSocket;


/***/ }),
/* 16 */
/***/ ((module, __unused_webpack_exports, __webpack_require__) => {

/* eslint no-unused-vars: ["error", { "varsIgnorePattern": "^Duplex|Readable$" }] */



const EventEmitter = __webpack_require__(17);
const https = __webpack_require__(18);
const http = __webpack_require__(19);
const net = __webpack_require__(20);
const tls = __webpack_require__(21);
const { randomBytes, createHash } = __webpack_require__(22);
const { Duplex, Readable } = __webpack_require__(23);
const { URL } = __webpack_require__(24);

const PerMessageDeflate = __webpack_require__(25);
const Receiver = __webpack_require__(30);
const Sender = __webpack_require__(33);
const {
  BINARY_TYPES,
  EMPTY_BUFFER,
  GUID,
  kForOnEventAttribute,
  kListener,
  kStatusCode,
  kWebSocket,
  NOOP
} = __webpack_require__(28);
const {
  EventTarget: { addEventListener, removeEventListener }
} = __webpack_require__(34);
const { format, parse } = __webpack_require__(35);
const { toBuffer } = __webpack_require__(27);

const closeTimeout = 30 * 1000;
const kAborted = Symbol('kAborted');
const protocolVersions = [8, 13];
const readyStates = ['CONNECTING', 'OPEN', 'CLOSING', 'CLOSED'];
const subprotocolRegex = /^[!#$%&'*+\-.0-9A-Z^_`|a-z~]+$/;

/**
 * Class representing a WebSocket.
 *
 * @extends EventEmitter
 */
class WebSocket extends EventEmitter {
  /**
   * Create a new `WebSocket`.
   *
   * @param {(String|URL)} address The URL to which to connect
   * @param {(String|String[])} [protocols] The subprotocols
   * @param {Object} [options] Connection options
   */
  constructor(address, protocols, options) {
    super();

    this._binaryType = BINARY_TYPES[0];
    this._closeCode = 1006;
    this._closeFrameReceived = false;
    this._closeFrameSent = false;
    this._closeMessage = EMPTY_BUFFER;
    this._closeTimer = null;
    this._extensions = {};
    this._paused = false;
    this._protocol = '';
    this._readyState = WebSocket.CONNECTING;
    this._receiver = null;
    this._sender = null;
    this._socket = null;

    if (address !== null) {
      this._bufferedAmount = 0;
      this._isServer = false;
      this._redirects = 0;

      if (protocols === undefined) {
        protocols = [];
      } else if (!Array.isArray(protocols)) {
        if (typeof protocols === 'object' && protocols !== null) {
          options = protocols;
          protocols = [];
        } else {
          protocols = [protocols];
        }
      }

      initAsClient(this, address, protocols, options);
    } else {
      this._autoPong = options.autoPong;
      this._isServer = true;
    }
  }

  /**
   * This deviates from the WHATWG interface since ws doesn't support the
   * required default "blob" type (instead we define a custom "nodebuffer"
   * type).
   *
   * @type {String}
   */
  get binaryType() {
    return this._binaryType;
  }

  set binaryType(type) {
    if (!BINARY_TYPES.includes(type)) return;

    this._binaryType = type;

    //
    // Allow to change `binaryType` on the fly.
    //
    if (this._receiver) this._receiver._binaryType = type;
  }

  /**
   * @type {Number}
   */
  get bufferedAmount() {
    if (!this._socket) return this._bufferedAmount;

    return this._socket._writableState.length + this._sender._bufferedBytes;
  }

  /**
   * @type {String}
   */
  get extensions() {
    return Object.keys(this._extensions).join();
  }

  /**
   * @type {Boolean}
   */
  get isPaused() {
    return this._paused;
  }

  /**
   * @type {Function}
   */
  /* istanbul ignore next */
  get onclose() {
    return null;
  }

  /**
   * @type {Function}
   */
  /* istanbul ignore next */
  get onerror() {
    return null;
  }

  /**
   * @type {Function}
   */
  /* istanbul ignore next */
  get onopen() {
    return null;
  }

  /**
   * @type {Function}
   */
  /* istanbul ignore next */
  get onmessage() {
    return null;
  }

  /**
   * @type {String}
   */
  get protocol() {
    return this._protocol;
  }

  /**
   * @type {Number}
   */
  get readyState() {
    return this._readyState;
  }

  /**
   * @type {String}
   */
  get url() {
    return this._url;
  }

  /**
   * Set up the socket and the internal resources.
   *
   * @param {Duplex} socket The network socket between the server and client
   * @param {Buffer} head The first packet of the upgraded stream
   * @param {Object} options Options object
   * @param {Boolean} [options.allowSynchronousEvents=false] Specifies whether
   *     any of the `'message'`, `'ping'`, and `'pong'` events can be emitted
   *     multiple times in the same tick
   * @param {Function} [options.generateMask] The function used to generate the
   *     masking key
   * @param {Number} [options.maxPayload=0] The maximum allowed message size
   * @param {Boolean} [options.skipUTF8Validation=false] Specifies whether or
   *     not to skip UTF-8 validation for text and close messages
   * @private
   */
  setSocket(socket, head, options) {
    const receiver = new Receiver({
      allowSynchronousEvents: options.allowSynchronousEvents,
      binaryType: this.binaryType,
      extensions: this._extensions,
      isServer: this._isServer,
      maxPayload: options.maxPayload,
      skipUTF8Validation: options.skipUTF8Validation
    });

    this._sender = new Sender(socket, this._extensions, options.generateMask);
    this._receiver = receiver;
    this._socket = socket;

    receiver[kWebSocket] = this;
    socket[kWebSocket] = this;

    receiver.on('conclude', receiverOnConclude);
    receiver.on('drain', receiverOnDrain);
    receiver.on('error', receiverOnError);
    receiver.on('message', receiverOnMessage);
    receiver.on('ping', receiverOnPing);
    receiver.on('pong', receiverOnPong);

    //
    // These methods may not be available if `socket` is just a `Duplex`.
    //
    if (socket.setTimeout) socket.setTimeout(0);
    if (socket.setNoDelay) socket.setNoDelay();

    if (head.length > 0) socket.unshift(head);

    socket.on('close', socketOnClose);
    socket.on('data', socketOnData);
    socket.on('end', socketOnEnd);
    socket.on('error', socketOnError);

    this._readyState = WebSocket.OPEN;
    this.emit('open');
  }

  /**
   * Emit the `'close'` event.
   *
   * @private
   */
  emitClose() {
    if (!this._socket) {
      this._readyState = WebSocket.CLOSED;
      this.emit('close', this._closeCode, this._closeMessage);
      return;
    }

    if (this._extensions[PerMessageDeflate.extensionName]) {
      this._extensions[PerMessageDeflate.extensionName].cleanup();
    }

    this._receiver.removeAllListeners();
    this._readyState = WebSocket.CLOSED;
    this.emit('close', this._closeCode, this._closeMessage);
  }

  /**
   * Start a closing handshake.
   *
   *          +----------+   +-----------+   +----------+
   *     - - -|ws.close()|-->|close frame|-->|ws.close()|- - -
   *    |     +----------+   +-----------+   +----------+     |
   *          +----------+   +-----------+         |
   * CLOSING  |ws.close()|<--|close frame|<--+-----+       CLOSING
   *          +----------+   +-----------+   |
   *    |           |                        |   +---+        |
   *                +------------------------+-->|fin| - - - -
   *    |         +---+                      |   +---+
   *     - - - - -|fin|<---------------------+
   *              +---+
   *
   * @param {Number} [code] Status code explaining why the connection is closing
   * @param {(String|Buffer)} [data] The reason why the connection is
   *     closing
   * @public
   */
  close(code, data) {
    if (this.readyState === WebSocket.CLOSED) return;
    if (this.readyState === WebSocket.CONNECTING) {
      const msg = 'WebSocket was closed before the connection was established';
      abortHandshake(this, this._req, msg);
      return;
    }

    if (this.readyState === WebSocket.CLOSING) {
      if (
        this._closeFrameSent &&
        (this._closeFrameReceived || this._receiver._writableState.errorEmitted)
      ) {
        this._socket.end();
      }

      return;
    }

    this._readyState = WebSocket.CLOSING;
    this._sender.close(code, data, !this._isServer, (err) => {
      //
      // This error is handled by the `'error'` listener on the socket. We only
      // want to know if the close frame has been sent here.
      //
      if (err) return;

      this._closeFrameSent = true;

      if (
        this._closeFrameReceived ||
        this._receiver._writableState.errorEmitted
      ) {
        this._socket.end();
      }
    });

    //
    // Specify a timeout for the closing handshake to complete.
    //
    this._closeTimer = setTimeout(
      this._socket.destroy.bind(this._socket),
      closeTimeout
    );
  }

  /**
   * Pause the socket.
   *
   * @public
   */
  pause() {
    if (
      this.readyState === WebSocket.CONNECTING ||
      this.readyState === WebSocket.CLOSED
    ) {
      return;
    }

    this._paused = true;
    this._socket.pause();
  }

  /**
   * Send a ping.
   *
   * @param {*} [data] The data to send
   * @param {Boolean} [mask] Indicates whether or not to mask `data`
   * @param {Function} [cb] Callback which is executed when the ping is sent
   * @public
   */
  ping(data, mask, cb) {
    if (this.readyState === WebSocket.CONNECTING) {
      throw new Error('WebSocket is not open: readyState 0 (CONNECTING)');
    }

    if (typeof data === 'function') {
      cb = data;
      data = mask = undefined;
    } else if (typeof mask === 'function') {
      cb = mask;
      mask = undefined;
    }

    if (typeof data === 'number') data = data.toString();

    if (this.readyState !== WebSocket.OPEN) {
      sendAfterClose(this, data, cb);
      return;
    }

    if (mask === undefined) mask = !this._isServer;
    this._sender.ping(data || EMPTY_BUFFER, mask, cb);
  }

  /**
   * Send a pong.
   *
   * @param {*} [data] The data to send
   * @param {Boolean} [mask] Indicates whether or not to mask `data`
   * @param {Function} [cb] Callback which is executed when the pong is sent
   * @public
   */
  pong(data, mask, cb) {
    if (this.readyState === WebSocket.CONNECTING) {
      throw new Error('WebSocket is not open: readyState 0 (CONNECTING)');
    }

    if (typeof data === 'function') {
      cb = data;
      data = mask = undefined;
    } else if (typeof mask === 'function') {
      cb = mask;
      mask = undefined;
    }

    if (typeof data === 'number') data = data.toString();

    if (this.readyState !== WebSocket.OPEN) {
      sendAfterClose(this, data, cb);
      return;
    }

    if (mask === undefined) mask = !this._isServer;
    this._sender.pong(data || EMPTY_BUFFER, mask, cb);
  }

  /**
   * Resume the socket.
   *
   * @public
   */
  resume() {
    if (
      this.readyState === WebSocket.CONNECTING ||
      this.readyState === WebSocket.CLOSED
    ) {
      return;
    }

    this._paused = false;
    if (!this._receiver._writableState.needDrain) this._socket.resume();
  }

  /**
   * Send a data message.
   *
   * @param {*} data The message to send
   * @param {Object} [options] Options object
   * @param {Boolean} [options.binary] Specifies whether `data` is binary or
   *     text
   * @param {Boolean} [options.compress] Specifies whether or not to compress
   *     `data`
   * @param {Boolean} [options.fin=true] Specifies whether the fragment is the
   *     last one
   * @param {Boolean} [options.mask] Specifies whether or not to mask `data`
   * @param {Function} [cb] Callback which is executed when data is written out
   * @public
   */
  send(data, options, cb) {
    if (this.readyState === WebSocket.CONNECTING) {
      throw new Error('WebSocket is not open: readyState 0 (CONNECTING)');
    }

    if (typeof options === 'function') {
      cb = options;
      options = {};
    }

    if (typeof data === 'number') data = data.toString();

    if (this.readyState !== WebSocket.OPEN) {
      sendAfterClose(this, data, cb);
      return;
    }

    const opts = {
      binary: typeof data !== 'string',
      mask: !this._isServer,
      compress: true,
      fin: true,
      ...options
    };

    if (!this._extensions[PerMessageDeflate.extensionName]) {
      opts.compress = false;
    }

    this._sender.send(data || EMPTY_BUFFER, opts, cb);
  }

  /**
   * Forcibly close the connection.
   *
   * @public
   */
  terminate() {
    if (this.readyState === WebSocket.CLOSED) return;
    if (this.readyState === WebSocket.CONNECTING) {
      const msg = 'WebSocket was closed before the connection was established';
      abortHandshake(this, this._req, msg);
      return;
    }

    if (this._socket) {
      this._readyState = WebSocket.CLOSING;
      this._socket.destroy();
    }
  }
}

/**
 * @constant {Number} CONNECTING
 * @memberof WebSocket
 */
Object.defineProperty(WebSocket, 'CONNECTING', {
  enumerable: true,
  value: readyStates.indexOf('CONNECTING')
});

/**
 * @constant {Number} CONNECTING
 * @memberof WebSocket.prototype
 */
Object.defineProperty(WebSocket.prototype, 'CONNECTING', {
  enumerable: true,
  value: readyStates.indexOf('CONNECTING')
});

/**
 * @constant {Number} OPEN
 * @memberof WebSocket
 */
Object.defineProperty(WebSocket, 'OPEN', {
  enumerable: true,
  value: readyStates.indexOf('OPEN')
});

/**
 * @constant {Number} OPEN
 * @memberof WebSocket.prototype
 */
Object.defineProperty(WebSocket.prototype, 'OPEN', {
  enumerable: true,
  value: readyStates.indexOf('OPEN')
});

/**
 * @constant {Number} CLOSING
 * @memberof WebSocket
 */
Object.defineProperty(WebSocket, 'CLOSING', {
  enumerable: true,
  value: readyStates.indexOf('CLOSING')
});

/**
 * @constant {Number} CLOSING
 * @memberof WebSocket.prototype
 */
Object.defineProperty(WebSocket.prototype, 'CLOSING', {
  enumerable: true,
  value: readyStates.indexOf('CLOSING')
});

/**
 * @constant {Number} CLOSED
 * @memberof WebSocket
 */
Object.defineProperty(WebSocket, 'CLOSED', {
  enumerable: true,
  value: readyStates.indexOf('CLOSED')
});

/**
 * @constant {Number} CLOSED
 * @memberof WebSocket.prototype
 */
Object.defineProperty(WebSocket.prototype, 'CLOSED', {
  enumerable: true,
  value: readyStates.indexOf('CLOSED')
});

[
  'binaryType',
  'bufferedAmount',
  'extensions',
  'isPaused',
  'protocol',
  'readyState',
  'url'
].forEach((property) => {
  Object.defineProperty(WebSocket.prototype, property, { enumerable: true });
});

//
// Add the `onopen`, `onerror`, `onclose`, and `onmessage` attributes.
// See https://html.spec.whatwg.org/multipage/comms.html#the-websocket-interface
//
['open', 'error', 'close', 'message'].forEach((method) => {
  Object.defineProperty(WebSocket.prototype, `on${method}`, {
    enumerable: true,
    get() {
      for (const listener of this.listeners(method)) {
        if (listener[kForOnEventAttribute]) return listener[kListener];
      }

      return null;
    },
    set(handler) {
      for (const listener of this.listeners(method)) {
        if (listener[kForOnEventAttribute]) {
          this.removeListener(method, listener);
          break;
        }
      }

      if (typeof handler !== 'function') return;

      this.addEventListener(method, handler, {
        [kForOnEventAttribute]: true
      });
    }
  });
});

WebSocket.prototype.addEventListener = addEventListener;
WebSocket.prototype.removeEventListener = removeEventListener;

module.exports = WebSocket;

/**
 * Initialize a WebSocket client.
 *
 * @param {WebSocket} websocket The client to initialize
 * @param {(String|URL)} address The URL to which to connect
 * @param {Array} protocols The subprotocols
 * @param {Object} [options] Connection options
 * @param {Boolean} [options.allowSynchronousEvents=false] Specifies whether any
 *     of the `'message'`, `'ping'`, and `'pong'` events can be emitted multiple
 *     times in the same tick
 * @param {Boolean} [options.autoPong=true] Specifies whether or not to
 *     automatically send a pong in response to a ping
 * @param {Function} [options.finishRequest] A function which can be used to
 *     customize the headers of each http request before it is sent
 * @param {Boolean} [options.followRedirects=false] Whether or not to follow
 *     redirects
 * @param {Function} [options.generateMask] The function used to generate the
 *     masking key
 * @param {Number} [options.handshakeTimeout] Timeout in milliseconds for the
 *     handshake request
 * @param {Number} [options.maxPayload=104857600] The maximum allowed message
 *     size
 * @param {Number} [options.maxRedirects=10] The maximum number of redirects
 *     allowed
 * @param {String} [options.origin] Value of the `Origin` or
 *     `Sec-WebSocket-Origin` header
 * @param {(Boolean|Object)} [options.perMessageDeflate=true] Enable/disable
 *     permessage-deflate
 * @param {Number} [options.protocolVersion=13] Value of the
 *     `Sec-WebSocket-Version` header
 * @param {Boolean} [options.skipUTF8Validation=false] Specifies whether or
 *     not to skip UTF-8 validation for text and close messages
 * @private
 */
function initAsClient(websocket, address, protocols, options) {
  const opts = {
    allowSynchronousEvents: false,
    autoPong: true,
    protocolVersion: protocolVersions[1],
    maxPayload: 100 * 1024 * 1024,
    skipUTF8Validation: false,
    perMessageDeflate: true,
    followRedirects: false,
    maxRedirects: 10,
    ...options,
    createConnection: undefined,
    socketPath: undefined,
    hostname: undefined,
    protocol: undefined,
    timeout: undefined,
    method: 'GET',
    host: undefined,
    path: undefined,
    port: undefined
  };

  websocket._autoPong = opts.autoPong;

  if (!protocolVersions.includes(opts.protocolVersion)) {
    throw new RangeError(
      `Unsupported protocol version: ${opts.protocolVersion} ` +
        `(supported versions: ${protocolVersions.join(', ')})`
    );
  }

  let parsedUrl;

  if (address instanceof URL) {
    parsedUrl = address;
  } else {
    try {
      parsedUrl = new URL(address);
    } catch (e) {
      throw new SyntaxError(`Invalid URL: ${address}`);
    }
  }

  if (parsedUrl.protocol === 'http:') {
    parsedUrl.protocol = 'ws:';
  } else if (parsedUrl.protocol === 'https:') {
    parsedUrl.protocol = 'wss:';
  }

  websocket._url = parsedUrl.href;

  const isSecure = parsedUrl.protocol === 'wss:';
  const isIpcUrl = parsedUrl.protocol === 'ws+unix:';
  let invalidUrlMessage;

  if (parsedUrl.protocol !== 'ws:' && !isSecure && !isIpcUrl) {
    invalidUrlMessage =
      'The URL\'s protocol must be one of "ws:", "wss:", ' +
      '"http:", "https", or "ws+unix:"';
  } else if (isIpcUrl && !parsedUrl.pathname) {
    invalidUrlMessage = "The URL's pathname is empty";
  } else if (parsedUrl.hash) {
    invalidUrlMessage = 'The URL contains a fragment identifier';
  }

  if (invalidUrlMessage) {
    const err = new SyntaxError(invalidUrlMessage);

    if (websocket._redirects === 0) {
      throw err;
    } else {
      emitErrorAndClose(websocket, err);
      return;
    }
  }

  const defaultPort = isSecure ? 443 : 80;
  const key = randomBytes(16).toString('base64');
  const request = isSecure ? https.request : http.request;
  const protocolSet = new Set();
  let perMessageDeflate;

  opts.createConnection = isSecure ? tlsConnect : netConnect;
  opts.defaultPort = opts.defaultPort || defaultPort;
  opts.port = parsedUrl.port || defaultPort;
  opts.host = parsedUrl.hostname.startsWith('[')
    ? parsedUrl.hostname.slice(1, -1)
    : parsedUrl.hostname;
  opts.headers = {
    ...opts.headers,
    'Sec-WebSocket-Version': opts.protocolVersion,
    'Sec-WebSocket-Key': key,
    Connection: 'Upgrade',
    Upgrade: 'websocket'
  };
  opts.path = parsedUrl.pathname + parsedUrl.search;
  opts.timeout = opts.handshakeTimeout;

  if (opts.perMessageDeflate) {
    perMessageDeflate = new PerMessageDeflate(
      opts.perMessageDeflate !== true ? opts.perMessageDeflate : {},
      false,
      opts.maxPayload
    );
    opts.headers['Sec-WebSocket-Extensions'] = format({
      [PerMessageDeflate.extensionName]: perMessageDeflate.offer()
    });
  }
  if (protocols.length) {
    for (const protocol of protocols) {
      if (
        typeof protocol !== 'string' ||
        !subprotocolRegex.test(protocol) ||
        protocolSet.has(protocol)
      ) {
        throw new SyntaxError(
          'An invalid or duplicated subprotocol was specified'
        );
      }

      protocolSet.add(protocol);
    }

    opts.headers['Sec-WebSocket-Protocol'] = protocols.join(',');
  }
  if (opts.origin) {
    if (opts.protocolVersion < 13) {
      opts.headers['Sec-WebSocket-Origin'] = opts.origin;
    } else {
      opts.headers.Origin = opts.origin;
    }
  }
  if (parsedUrl.username || parsedUrl.password) {
    opts.auth = `${parsedUrl.username}:${parsedUrl.password}`;
  }

  if (isIpcUrl) {
    const parts = opts.path.split(':');

    opts.socketPath = parts[0];
    opts.path = parts[1];
  }

  let req;

  if (opts.followRedirects) {
    if (websocket._redirects === 0) {
      websocket._originalIpc = isIpcUrl;
      websocket._originalSecure = isSecure;
      websocket._originalHostOrSocketPath = isIpcUrl
        ? opts.socketPath
        : parsedUrl.host;

      const headers = options && options.headers;

      //
      // Shallow copy the user provided options so that headers can be changed
      // without mutating the original object.
      //
      options = { ...options, headers: {} };

      if (headers) {
        for (const [key, value] of Object.entries(headers)) {
          options.headers[key.toLowerCase()] = value;
        }
      }
    } else if (websocket.listenerCount('redirect') === 0) {
      const isSameHost = isIpcUrl
        ? websocket._originalIpc
          ? opts.socketPath === websocket._originalHostOrSocketPath
          : false
        : websocket._originalIpc
          ? false
          : parsedUrl.host === websocket._originalHostOrSocketPath;

      if (!isSameHost || (websocket._originalSecure && !isSecure)) {
        //
        // Match curl 7.77.0 behavior and drop the following headers. These
        // headers are also dropped when following a redirect to a subdomain.
        //
        delete opts.headers.authorization;
        delete opts.headers.cookie;

        if (!isSameHost) delete opts.headers.host;

        opts.auth = undefined;
      }
    }

    //
    // Match curl 7.77.0 behavior and make the first `Authorization` header win.
    // If the `Authorization` header is set, then there is nothing to do as it
    // will take precedence.
    //
    if (opts.auth && !options.headers.authorization) {
      options.headers.authorization =
        'Basic ' + Buffer.from(opts.auth).toString('base64');
    }

    req = websocket._req = request(opts);

    if (websocket._redirects) {
      //
      // Unlike what is done for the `'upgrade'` event, no early exit is
      // triggered here if the user calls `websocket.close()` or
      // `websocket.terminate()` from a listener of the `'redirect'` event. This
      // is because the user can also call `request.destroy()` with an error
      // before calling `websocket.close()` or `websocket.terminate()` and this
      // would result in an error being emitted on the `request` object with no
      // `'error'` event listeners attached.
      //
      websocket.emit('redirect', websocket.url, req);
    }
  } else {
    req = websocket._req = request(opts);
  }

  if (opts.timeout) {
    req.on('timeout', () => {
      abortHandshake(websocket, req, 'Opening handshake has timed out');
    });
  }

  req.on('error', (err) => {
    if (req === null || req[kAborted]) return;

    req = websocket._req = null;
    emitErrorAndClose(websocket, err);
  });

  req.on('response', (res) => {
    const location = res.headers.location;
    const statusCode = res.statusCode;

    if (
      location &&
      opts.followRedirects &&
      statusCode >= 300 &&
      statusCode < 400
    ) {
      if (++websocket._redirects > opts.maxRedirects) {
        abortHandshake(websocket, req, 'Maximum redirects exceeded');
        return;
      }

      req.abort();

      let addr;

      try {
        addr = new URL(location, address);
      } catch (e) {
        const err = new SyntaxError(`Invalid URL: ${location}`);
        emitErrorAndClose(websocket, err);
        return;
      }

      initAsClient(websocket, addr, protocols, options);
    } else if (!websocket.emit('unexpected-response', req, res)) {
      abortHandshake(
        websocket,
        req,
        `Unexpected server response: ${res.statusCode}`
      );
    }
  });

  req.on('upgrade', (res, socket, head) => {
    websocket.emit('upgrade', res);

    //
    // The user may have closed the connection from a listener of the
    // `'upgrade'` event.
    //
    if (websocket.readyState !== WebSocket.CONNECTING) return;

    req = websocket._req = null;

    if (res.headers.upgrade.toLowerCase() !== 'websocket') {
      abortHandshake(websocket, socket, 'Invalid Upgrade header');
      return;
    }

    const digest = createHash('sha1')
      .update(key + GUID)
      .digest('base64');

    if (res.headers['sec-websocket-accept'] !== digest) {
      abortHandshake(websocket, socket, 'Invalid Sec-WebSocket-Accept header');
      return;
    }

    const serverProt = res.headers['sec-websocket-protocol'];
    let protError;

    if (serverProt !== undefined) {
      if (!protocolSet.size) {
        protError = 'Server sent a subprotocol but none was requested';
      } else if (!protocolSet.has(serverProt)) {
        protError = 'Server sent an invalid subprotocol';
      }
    } else if (protocolSet.size) {
      protError = 'Server sent no subprotocol';
    }

    if (protError) {
      abortHandshake(websocket, socket, protError);
      return;
    }

    if (serverProt) websocket._protocol = serverProt;

    const secWebSocketExtensions = res.headers['sec-websocket-extensions'];

    if (secWebSocketExtensions !== undefined) {
      if (!perMessageDeflate) {
        const message =
          'Server sent a Sec-WebSocket-Extensions header but no extension ' +
          'was requested';
        abortHandshake(websocket, socket, message);
        return;
      }

      let extensions;

      try {
        extensions = parse(secWebSocketExtensions);
      } catch (err) {
        const message = 'Invalid Sec-WebSocket-Extensions header';
        abortHandshake(websocket, socket, message);
        return;
      }

      const extensionNames = Object.keys(extensions);

      if (
        extensionNames.length !== 1 ||
        extensionNames[0] !== PerMessageDeflate.extensionName
      ) {
        const message = 'Server indicated an extension that was not requested';
        abortHandshake(websocket, socket, message);
        return;
      }

      try {
        perMessageDeflate.accept(extensions[PerMessageDeflate.extensionName]);
      } catch (err) {
        const message = 'Invalid Sec-WebSocket-Extensions header';
        abortHandshake(websocket, socket, message);
        return;
      }

      websocket._extensions[PerMessageDeflate.extensionName] =
        perMessageDeflate;
    }

    websocket.setSocket(socket, head, {
      allowSynchronousEvents: opts.allowSynchronousEvents,
      generateMask: opts.generateMask,
      maxPayload: opts.maxPayload,
      skipUTF8Validation: opts.skipUTF8Validation
    });
  });

  if (opts.finishRequest) {
    opts.finishRequest(req, websocket);
  } else {
    req.end();
  }
}

/**
 * Emit the `'error'` and `'close'` events.
 *
 * @param {WebSocket} websocket The WebSocket instance
 * @param {Error} The error to emit
 * @private
 */
function emitErrorAndClose(websocket, err) {
  websocket._readyState = WebSocket.CLOSING;
  websocket.emit('error', err);
  websocket.emitClose();
}

/**
 * Create a `net.Socket` and initiate a connection.
 *
 * @param {Object} options Connection options
 * @return {net.Socket} The newly created socket used to start the connection
 * @private
 */
function netConnect(options) {
  options.path = options.socketPath;
  return net.connect(options);
}

/**
 * Create a `tls.TLSSocket` and initiate a connection.
 *
 * @param {Object} options Connection options
 * @return {tls.TLSSocket} The newly created socket used to start the connection
 * @private
 */
function tlsConnect(options) {
  options.path = undefined;

  if (!options.servername && options.servername !== '') {
    options.servername = net.isIP(options.host) ? '' : options.host;
  }

  return tls.connect(options);
}

/**
 * Abort the handshake and emit an error.
 *
 * @param {WebSocket} websocket The WebSocket instance
 * @param {(http.ClientRequest|net.Socket|tls.Socket)} stream The request to
 *     abort or the socket to destroy
 * @param {String} message The error message
 * @private
 */
function abortHandshake(websocket, stream, message) {
  websocket._readyState = WebSocket.CLOSING;

  const err = new Error(message);
  Error.captureStackTrace(err, abortHandshake);

  if (stream.setHeader) {
    stream[kAborted] = true;
    stream.abort();

    if (stream.socket && !stream.socket.destroyed) {
      //
      // On Node.js >= 14.3.0 `request.abort()` does not destroy the socket if
      // called after the request completed. See
      // https://github.com/websockets/ws/issues/1869.
      //
      stream.socket.destroy();
    }

    process.nextTick(emitErrorAndClose, websocket, err);
  } else {
    stream.destroy(err);
    stream.once('error', websocket.emit.bind(websocket, 'error'));
    stream.once('close', websocket.emitClose.bind(websocket));
  }
}

/**
 * Handle cases where the `ping()`, `pong()`, or `send()` methods are called
 * when the `readyState` attribute is `CLOSING` or `CLOSED`.
 *
 * @param {WebSocket} websocket The WebSocket instance
 * @param {*} [data] The data to send
 * @param {Function} [cb] Callback
 * @private
 */
function sendAfterClose(websocket, data, cb) {
  if (data) {
    const length = toBuffer(data).length;

    //
    // The `_bufferedAmount` property is used only when the peer is a client and
    // the opening handshake fails. Under these circumstances, in fact, the
    // `setSocket()` method is not called, so the `_socket` and `_sender`
    // properties are set to `null`.
    //
    if (websocket._socket) websocket._sender._bufferedBytes += length;
    else websocket._bufferedAmount += length;
  }

  if (cb) {
    const err = new Error(
      `WebSocket is not open: readyState ${websocket.readyState} ` +
        `(${readyStates[websocket.readyState]})`
    );
    process.nextTick(cb, err);
  }
}

/**
 * The listener of the `Receiver` `'conclude'` event.
 *
 * @param {Number} code The status code
 * @param {Buffer} reason The reason for closing
 * @private
 */
function receiverOnConclude(code, reason) {
  const websocket = this[kWebSocket];

  websocket._closeFrameReceived = true;
  websocket._closeMessage = reason;
  websocket._closeCode = code;

  if (websocket._socket[kWebSocket] === undefined) return;

  websocket._socket.removeListener('data', socketOnData);
  process.nextTick(resume, websocket._socket);

  if (code === 1005) websocket.close();
  else websocket.close(code, reason);
}

/**
 * The listener of the `Receiver` `'drain'` event.
 *
 * @private
 */
function receiverOnDrain() {
  const websocket = this[kWebSocket];

  if (!websocket.isPaused) websocket._socket.resume();
}

/**
 * The listener of the `Receiver` `'error'` event.
 *
 * @param {(RangeError|Error)} err The emitted error
 * @private
 */
function receiverOnError(err) {
  const websocket = this[kWebSocket];

  if (websocket._socket[kWebSocket] !== undefined) {
    websocket._socket.removeListener('data', socketOnData);

    //
    // On Node.js < 14.0.0 the `'error'` event is emitted synchronously. See
    // https://github.com/websockets/ws/issues/1940.
    //
    process.nextTick(resume, websocket._socket);

    websocket.close(err[kStatusCode]);
  }

  websocket.emit('error', err);
}

/**
 * The listener of the `Receiver` `'finish'` event.
 *
 * @private
 */
function receiverOnFinish() {
  this[kWebSocket].emitClose();
}

/**
 * The listener of the `Receiver` `'message'` event.
 *
 * @param {Buffer|ArrayBuffer|Buffer[])} data The message
 * @param {Boolean} isBinary Specifies whether the message is binary or not
 * @private
 */
function receiverOnMessage(data, isBinary) {
  this[kWebSocket].emit('message', data, isBinary);
}

/**
 * The listener of the `Receiver` `'ping'` event.
 *
 * @param {Buffer} data The data included in the ping frame
 * @private
 */
function receiverOnPing(data) {
  const websocket = this[kWebSocket];

  if (websocket._autoPong) websocket.pong(data, !this._isServer, NOOP);
  websocket.emit('ping', data);
}

/**
 * The listener of the `Receiver` `'pong'` event.
 *
 * @param {Buffer} data The data included in the pong frame
 * @private
 */
function receiverOnPong(data) {
  this[kWebSocket].emit('pong', data);
}

/**
 * Resume a readable stream
 *
 * @param {Readable} stream The readable stream
 * @private
 */
function resume(stream) {
  stream.resume();
}

/**
 * The listener of the socket `'close'` event.
 *
 * @private
 */
function socketOnClose() {
  const websocket = this[kWebSocket];

  this.removeListener('close', socketOnClose);
  this.removeListener('data', socketOnData);
  this.removeListener('end', socketOnEnd);

  websocket._readyState = WebSocket.CLOSING;

  let chunk;

  //
  // The close frame might not have been received or the `'end'` event emitted,
  // for example, if the socket was destroyed due to an error. Ensure that the
  // `receiver` stream is closed after writing any remaining buffered data to
  // it. If the readable side of the socket is in flowing mode then there is no
  // buffered data as everything has been already written and `readable.read()`
  // will return `null`. If instead, the socket is paused, any possible buffered
  // data will be read as a single chunk.
  //
  if (
    !this._readableState.endEmitted &&
    !websocket._closeFrameReceived &&
    !websocket._receiver._writableState.errorEmitted &&
    (chunk = websocket._socket.read()) !== null
  ) {
    websocket._receiver.write(chunk);
  }

  websocket._receiver.end();

  this[kWebSocket] = undefined;

  clearTimeout(websocket._closeTimer);

  if (
    websocket._receiver._writableState.finished ||
    websocket._receiver._writableState.errorEmitted
  ) {
    websocket.emitClose();
  } else {
    websocket._receiver.on('error', receiverOnFinish);
    websocket._receiver.on('finish', receiverOnFinish);
  }
}

/**
 * The listener of the socket `'data'` event.
 *
 * @param {Buffer} chunk A chunk of data
 * @private
 */
function socketOnData(chunk) {
  if (!this[kWebSocket]._receiver.write(chunk)) {
    this.pause();
  }
}

/**
 * The listener of the socket `'end'` event.
 *
 * @private
 */
function socketOnEnd() {
  const websocket = this[kWebSocket];

  websocket._readyState = WebSocket.CLOSING;
  websocket._receiver.end();
  this.end();
}

/**
 * The listener of the socket `'error'` event.
 *
 * @private
 */
function socketOnError() {
  const websocket = this[kWebSocket];

  this.removeListener('error', socketOnError);
  this.on('error', NOOP);

  if (websocket) {
    websocket._readyState = WebSocket.CLOSING;
    this.destroy();
  }
}


/***/ }),
/* 17 */
/***/ ((module) => {

module.exports = require("events");

/***/ }),
/* 18 */
/***/ ((module) => {

module.exports = require("https");

/***/ }),
/* 19 */
/***/ ((module) => {

module.exports = require("http");

/***/ }),
/* 20 */
/***/ ((module) => {

module.exports = require("net");

/***/ }),
/* 21 */
/***/ ((module) => {

module.exports = require("tls");

/***/ }),
/* 22 */
/***/ ((module) => {

module.exports = require("crypto");

/***/ }),
/* 23 */
/***/ ((module) => {

module.exports = require("stream");

/***/ }),
/* 24 */
/***/ ((module) => {

module.exports = require("url");

/***/ }),
/* 25 */
/***/ ((module, __unused_webpack_exports, __webpack_require__) => {



const zlib = __webpack_require__(26);

const bufferUtil = __webpack_require__(27);
const Limiter = __webpack_require__(29);
const { kStatusCode } = __webpack_require__(28);

const FastBuffer = Buffer[Symbol.species];
const TRAILER = Buffer.from([0x00, 0x00, 0xff, 0xff]);
const kPerMessageDeflate = Symbol('permessage-deflate');
const kTotalLength = Symbol('total-length');
const kCallback = Symbol('callback');
const kBuffers = Symbol('buffers');
const kError = Symbol('error');

//
// We limit zlib concurrency, which prevents severe memory fragmentation
// as documented in https://github.com/nodejs/node/issues/8871#issuecomment-250915913
// and https://github.com/websockets/ws/issues/1202
//
// Intentionally global; it's the global thread pool that's an issue.
//
let zlibLimiter;

/**
 * permessage-deflate implementation.
 */
class PerMessageDeflate {
  /**
   * Creates a PerMessageDeflate instance.
   *
   * @param {Object} [options] Configuration options
   * @param {(Boolean|Number)} [options.clientMaxWindowBits] Advertise support
   *     for, or request, a custom client window size
   * @param {Boolean} [options.clientNoContextTakeover=false] Advertise/
   *     acknowledge disabling of client context takeover
   * @param {Number} [options.concurrencyLimit=10] The number of concurrent
   *     calls to zlib
   * @param {(Boolean|Number)} [options.serverMaxWindowBits] Request/confirm the
   *     use of a custom server window size
   * @param {Boolean} [options.serverNoContextTakeover=false] Request/accept
   *     disabling of server context takeover
   * @param {Number} [options.threshold=1024] Size (in bytes) below which
   *     messages should not be compressed if context takeover is disabled
   * @param {Object} [options.zlibDeflateOptions] Options to pass to zlib on
   *     deflate
   * @param {Object} [options.zlibInflateOptions] Options to pass to zlib on
   *     inflate
   * @param {Boolean} [isServer=false] Create the instance in either server or
   *     client mode
   * @param {Number} [maxPayload=0] The maximum allowed message length
   */
  constructor(options, isServer, maxPayload) {
    this._maxPayload = maxPayload | 0;
    this._options = options || {};
    this._threshold =
      this._options.threshold !== undefined ? this._options.threshold : 1024;
    this._isServer = !!isServer;
    this._deflate = null;
    this._inflate = null;

    this.params = null;

    if (!zlibLimiter) {
      const concurrency =
        this._options.concurrencyLimit !== undefined
          ? this._options.concurrencyLimit
          : 10;
      zlibLimiter = new Limiter(concurrency);
    }
  }

  /**
   * @type {String}
   */
  static get extensionName() {
    return 'permessage-deflate';
  }

  /**
   * Create an extension negotiation offer.
   *
   * @return {Object} Extension parameters
   * @public
   */
  offer() {
    const params = {};

    if (this._options.serverNoContextTakeover) {
      params.server_no_context_takeover = true;
    }
    if (this._options.clientNoContextTakeover) {
      params.client_no_context_takeover = true;
    }
    if (this._options.serverMaxWindowBits) {
      params.server_max_window_bits = this._options.serverMaxWindowBits;
    }
    if (this._options.clientMaxWindowBits) {
      params.client_max_window_bits = this._options.clientMaxWindowBits;
    } else if (this._options.clientMaxWindowBits == null) {
      params.client_max_window_bits = true;
    }

    return params;
  }

  /**
   * Accept an extension negotiation offer/response.
   *
   * @param {Array} configurations The extension negotiation offers/reponse
   * @return {Object} Accepted configuration
   * @public
   */
  accept(configurations) {
    configurations = this.normalizeParams(configurations);

    this.params = this._isServer
      ? this.acceptAsServer(configurations)
      : this.acceptAsClient(configurations);

    return this.params;
  }

  /**
   * Releases all resources used by the extension.
   *
   * @public
   */
  cleanup() {
    if (this._inflate) {
      this._inflate.close();
      this._inflate = null;
    }

    if (this._deflate) {
      const callback = this._deflate[kCallback];

      this._deflate.close();
      this._deflate = null;

      if (callback) {
        callback(
          new Error(
            'The deflate stream was closed while data was being processed'
          )
        );
      }
    }
  }

  /**
   *  Accept an extension negotiation offer.
   *
   * @param {Array} offers The extension negotiation offers
   * @return {Object} Accepted configuration
   * @private
   */
  acceptAsServer(offers) {
    const opts = this._options;
    const accepted = offers.find((params) => {
      if (
        (opts.serverNoContextTakeover === false &&
          params.server_no_context_takeover) ||
        (params.server_max_window_bits &&
          (opts.serverMaxWindowBits === false ||
            (typeof opts.serverMaxWindowBits === 'number' &&
              opts.serverMaxWindowBits > params.server_max_window_bits))) ||
        (typeof opts.clientMaxWindowBits === 'number' &&
          !params.client_max_window_bits)
      ) {
        return false;
      }

      return true;
    });

    if (!accepted) {
      throw new Error('None of the extension offers can be accepted');
    }

    if (opts.serverNoContextTakeover) {
      accepted.server_no_context_takeover = true;
    }
    if (opts.clientNoContextTakeover) {
      accepted.client_no_context_takeover = true;
    }
    if (typeof opts.serverMaxWindowBits === 'number') {
      accepted.server_max_window_bits = opts.serverMaxWindowBits;
    }
    if (typeof opts.clientMaxWindowBits === 'number') {
      accepted.client_max_window_bits = opts.clientMaxWindowBits;
    } else if (
      accepted.client_max_window_bits === true ||
      opts.clientMaxWindowBits === false
    ) {
      delete accepted.client_max_window_bits;
    }

    return accepted;
  }

  /**
   * Accept the extension negotiation response.
   *
   * @param {Array} response The extension negotiation response
   * @return {Object} Accepted configuration
   * @private
   */
  acceptAsClient(response) {
    const params = response[0];

    if (
      this._options.clientNoContextTakeover === false &&
      params.client_no_context_takeover
    ) {
      throw new Error('Unexpected parameter "client_no_context_takeover"');
    }

    if (!params.client_max_window_bits) {
      if (typeof this._options.clientMaxWindowBits === 'number') {
        params.client_max_window_bits = this._options.clientMaxWindowBits;
      }
    } else if (
      this._options.clientMaxWindowBits === false ||
      (typeof this._options.clientMaxWindowBits === 'number' &&
        params.client_max_window_bits > this._options.clientMaxWindowBits)
    ) {
      throw new Error(
        'Unexpected or invalid parameter "client_max_window_bits"'
      );
    }

    return params;
  }

  /**
   * Normalize parameters.
   *
   * @param {Array} configurations The extension negotiation offers/reponse
   * @return {Array} The offers/response with normalized parameters
   * @private
   */
  normalizeParams(configurations) {
    configurations.forEach((params) => {
      Object.keys(params).forEach((key) => {
        let value = params[key];

        if (value.length > 1) {
          throw new Error(`Parameter "${key}" must have only a single value`);
        }

        value = value[0];

        if (key === 'client_max_window_bits') {
          if (value !== true) {
            const num = +value;
            if (!Number.isInteger(num) || num < 8 || num > 15) {
              throw new TypeError(
                `Invalid value for parameter "${key}": ${value}`
              );
            }
            value = num;
          } else if (!this._isServer) {
            throw new TypeError(
              `Invalid value for parameter "${key}": ${value}`
            );
          }
        } else if (key === 'server_max_window_bits') {
          const num = +value;
          if (!Number.isInteger(num) || num < 8 || num > 15) {
            throw new TypeError(
              `Invalid value for parameter "${key}": ${value}`
            );
          }
          value = num;
        } else if (
          key === 'client_no_context_takeover' ||
          key === 'server_no_context_takeover'
        ) {
          if (value !== true) {
            throw new TypeError(
              `Invalid value for parameter "${key}": ${value}`
            );
          }
        } else {
          throw new Error(`Unknown parameter "${key}"`);
        }

        params[key] = value;
      });
    });

    return configurations;
  }

  /**
   * Decompress data. Concurrency limited.
   *
   * @param {Buffer} data Compressed data
   * @param {Boolean} fin Specifies whether or not this is the last fragment
   * @param {Function} callback Callback
   * @public
   */
  decompress(data, fin, callback) {
    zlibLimiter.add((done) => {
      this._decompress(data, fin, (err, result) => {
        done();
        callback(err, result);
      });
    });
  }

  /**
   * Compress data. Concurrency limited.
   *
   * @param {(Buffer|String)} data Data to compress
   * @param {Boolean} fin Specifies whether or not this is the last fragment
   * @param {Function} callback Callback
   * @public
   */
  compress(data, fin, callback) {
    zlibLimiter.add((done) => {
      this._compress(data, fin, (err, result) => {
        done();
        callback(err, result);
      });
    });
  }

  /**
   * Decompress data.
   *
   * @param {Buffer} data Compressed data
   * @param {Boolean} fin Specifies whether or not this is the last fragment
   * @param {Function} callback Callback
   * @private
   */
  _decompress(data, fin, callback) {
    const endpoint = this._isServer ? 'client' : 'server';

    if (!this._inflate) {
      const key = `${endpoint}_max_window_bits`;
      const windowBits =
        typeof this.params[key] !== 'number'
          ? zlib.Z_DEFAULT_WINDOWBITS
          : this.params[key];

      this._inflate = zlib.createInflateRaw({
        ...this._options.zlibInflateOptions,
        windowBits
      });
      this._inflate[kPerMessageDeflate] = this;
      this._inflate[kTotalLength] = 0;
      this._inflate[kBuffers] = [];
      this._inflate.on('error', inflateOnError);
      this._inflate.on('data', inflateOnData);
    }

    this._inflate[kCallback] = callback;

    this._inflate.write(data);
    if (fin) this._inflate.write(TRAILER);

    this._inflate.flush(() => {
      const err = this._inflate[kError];

      if (err) {
        this._inflate.close();
        this._inflate = null;
        callback(err);
        return;
      }

      const data = bufferUtil.concat(
        this._inflate[kBuffers],
        this._inflate[kTotalLength]
      );

      if (this._inflate._readableState.endEmitted) {
        this._inflate.close();
        this._inflate = null;
      } else {
        this._inflate[kTotalLength] = 0;
        this._inflate[kBuffers] = [];

        if (fin && this.params[`${endpoint}_no_context_takeover`]) {
          this._inflate.reset();
        }
      }

      callback(null, data);
    });
  }

  /**
   * Compress data.
   *
   * @param {(Buffer|String)} data Data to compress
   * @param {Boolean} fin Specifies whether or not this is the last fragment
   * @param {Function} callback Callback
   * @private
   */
  _compress(data, fin, callback) {
    const endpoint = this._isServer ? 'server' : 'client';

    if (!this._deflate) {
      const key = `${endpoint}_max_window_bits`;
      const windowBits =
        typeof this.params[key] !== 'number'
          ? zlib.Z_DEFAULT_WINDOWBITS
          : this.params[key];

      this._deflate = zlib.createDeflateRaw({
        ...this._options.zlibDeflateOptions,
        windowBits
      });

      this._deflate[kTotalLength] = 0;
      this._deflate[kBuffers] = [];

      this._deflate.on('data', deflateOnData);
    }

    this._deflate[kCallback] = callback;

    this._deflate.write(data);
    this._deflate.flush(zlib.Z_SYNC_FLUSH, () => {
      if (!this._deflate) {
        //
        // The deflate stream was closed while data was being processed.
        //
        return;
      }

      let data = bufferUtil.concat(
        this._deflate[kBuffers],
        this._deflate[kTotalLength]
      );

      if (fin) {
        data = new FastBuffer(data.buffer, data.byteOffset, data.length - 4);
      }

      //
      // Ensure that the callback will not be called again in
      // `PerMessageDeflate#cleanup()`.
      //
      this._deflate[kCallback] = null;

      this._deflate[kTotalLength] = 0;
      this._deflate[kBuffers] = [];

      if (fin && this.params[`${endpoint}_no_context_takeover`]) {
        this._deflate.reset();
      }

      callback(null, data);
    });
  }
}

module.exports = PerMessageDeflate;

/**
 * The listener of the `zlib.DeflateRaw` stream `'data'` event.
 *
 * @param {Buffer} chunk A chunk of data
 * @private
 */
function deflateOnData(chunk) {
  this[kBuffers].push(chunk);
  this[kTotalLength] += chunk.length;
}

/**
 * The listener of the `zlib.InflateRaw` stream `'data'` event.
 *
 * @param {Buffer} chunk A chunk of data
 * @private
 */
function inflateOnData(chunk) {
  this[kTotalLength] += chunk.length;

  if (
    this[kPerMessageDeflate]._maxPayload < 1 ||
    this[kTotalLength] <= this[kPerMessageDeflate]._maxPayload
  ) {
    this[kBuffers].push(chunk);
    return;
  }

  this[kError] = new RangeError('Max payload size exceeded');
  this[kError].code = 'WS_ERR_UNSUPPORTED_MESSAGE_LENGTH';
  this[kError][kStatusCode] = 1009;
  this.removeListener('data', inflateOnData);
  this.reset();
}

/**
 * The listener of the `zlib.InflateRaw` stream `'error'` event.
 *
 * @param {Error} err The emitted error
 * @private
 */
function inflateOnError(err) {
  //
  // There is no need to call `Zlib#close()` as the handle is automatically
  // closed when an error is emitted.
  //
  this[kPerMessageDeflate]._inflate = null;
  err[kStatusCode] = 1007;
  this[kCallback](err);
}


/***/ }),
/* 26 */
/***/ ((module) => {

module.exports = require("zlib");

/***/ }),
/* 27 */
/***/ ((module, __unused_webpack_exports, __webpack_require__) => {



const { EMPTY_BUFFER } = __webpack_require__(28);

const FastBuffer = Buffer[Symbol.species];

/**
 * Merges an array of buffers into a new buffer.
 *
 * @param {Buffer[]} list The array of buffers to concat
 * @param {Number} totalLength The total length of buffers in the list
 * @return {Buffer} The resulting buffer
 * @public
 */
function concat(list, totalLength) {
  if (list.length === 0) return EMPTY_BUFFER;
  if (list.length === 1) return list[0];

  const target = Buffer.allocUnsafe(totalLength);
  let offset = 0;

  for (let i = 0; i < list.length; i++) {
    const buf = list[i];
    target.set(buf, offset);
    offset += buf.length;
  }

  if (offset < totalLength) {
    return new FastBuffer(target.buffer, target.byteOffset, offset);
  }

  return target;
}

/**
 * Masks a buffer using the given mask.
 *
 * @param {Buffer} source The buffer to mask
 * @param {Buffer} mask The mask to use
 * @param {Buffer} output The buffer where to store the result
 * @param {Number} offset The offset at which to start writing
 * @param {Number} length The number of bytes to mask.
 * @public
 */
function _mask(source, mask, output, offset, length) {
  for (let i = 0; i < length; i++) {
    output[offset + i] = source[i] ^ mask[i & 3];
  }
}

/**
 * Unmasks a buffer using the given mask.
 *
 * @param {Buffer} buffer The buffer to unmask
 * @param {Buffer} mask The mask to use
 * @public
 */
function _unmask(buffer, mask) {
  for (let i = 0; i < buffer.length; i++) {
    buffer[i] ^= mask[i & 3];
  }
}

/**
 * Converts a buffer to an `ArrayBuffer`.
 *
 * @param {Buffer} buf The buffer to convert
 * @return {ArrayBuffer} Converted buffer
 * @public
 */
function toArrayBuffer(buf) {
  if (buf.length === buf.buffer.byteLength) {
    return buf.buffer;
  }

  return buf.buffer.slice(buf.byteOffset, buf.byteOffset + buf.length);
}

/**
 * Converts `data` to a `Buffer`.
 *
 * @param {*} data The data to convert
 * @return {Buffer} The buffer
 * @throws {TypeError}
 * @public
 */
function toBuffer(data) {
  toBuffer.readOnly = true;

  if (Buffer.isBuffer(data)) return data;

  let buf;

  if (data instanceof ArrayBuffer) {
    buf = new FastBuffer(data);
  } else if (ArrayBuffer.isView(data)) {
    buf = new FastBuffer(data.buffer, data.byteOffset, data.byteLength);
  } else {
    buf = Buffer.from(data);
    toBuffer.readOnly = false;
  }

  return buf;
}

module.exports = {
  concat,
  mask: _mask,
  toArrayBuffer,
  toBuffer,
  unmask: _unmask
};

/* istanbul ignore else  */
if (!process.env.WS_NO_BUFFER_UTIL) {
  try {
    const bufferUtil = __webpack_require__(Object(function webpackMissingModule() { var e = new Error("Cannot find module 'bufferutil'"); e.code = 'MODULE_NOT_FOUND'; throw e; }()));

    module.exports.mask = function (source, mask, output, offset, length) {
      if (length < 48) _mask(source, mask, output, offset, length);
      else bufferUtil.mask(source, mask, output, offset, length);
    };

    module.exports.unmask = function (buffer, mask) {
      if (buffer.length < 32) _unmask(buffer, mask);
      else bufferUtil.unmask(buffer, mask);
    };
  } catch (e) {
    // Continue regardless of the error.
  }
}


/***/ }),
/* 28 */
/***/ ((module) => {



module.exports = {
  BINARY_TYPES: ['nodebuffer', 'arraybuffer', 'fragments'],
  EMPTY_BUFFER: Buffer.alloc(0),
  GUID: '258EAFA5-E914-47DA-95CA-C5AB0DC85B11',
  kForOnEventAttribute: Symbol('kIsForOnEventAttribute'),
  kListener: Symbol('kListener'),
  kStatusCode: Symbol('status-code'),
  kWebSocket: Symbol('websocket'),
  NOOP: () => {}
};


/***/ }),
/* 29 */
/***/ ((module) => {



const kDone = Symbol('kDone');
const kRun = Symbol('kRun');

/**
 * A very simple job queue with adjustable concurrency. Adapted from
 * https://github.com/STRML/async-limiter
 */
class Limiter {
  /**
   * Creates a new `Limiter`.
   *
   * @param {Number} [concurrency=Infinity] The maximum number of jobs allowed
   *     to run concurrently
   */
  constructor(concurrency) {
    this[kDone] = () => {
      this.pending--;
      this[kRun]();
    };
    this.concurrency = concurrency || Infinity;
    this.jobs = [];
    this.pending = 0;
  }

  /**
   * Adds a job to the queue.
   *
   * @param {Function} job The job to run
   * @public
   */
  add(job) {
    this.jobs.push(job);
    this[kRun]();
  }

  /**
   * Removes a job from the queue and runs it if possible.
   *
   * @private
   */
  [kRun]() {
    if (this.pending === this.concurrency) return;

    if (this.jobs.length) {
      const job = this.jobs.shift();

      this.pending++;
      job(this[kDone]);
    }
  }
}

module.exports = Limiter;


/***/ }),
/* 30 */
/***/ ((module, __unused_webpack_exports, __webpack_require__) => {



const { Writable } = __webpack_require__(23);

const PerMessageDeflate = __webpack_require__(25);
const {
  BINARY_TYPES,
  EMPTY_BUFFER,
  kStatusCode,
  kWebSocket
} = __webpack_require__(28);
const { concat, toArrayBuffer, unmask } = __webpack_require__(27);
const { isValidStatusCode, isValidUTF8 } = __webpack_require__(31);

const FastBuffer = Buffer[Symbol.species];
const promise = Promise.resolve();

//
// `queueMicrotask()` is not available in Node.js < 11.
//
const queueTask =
  typeof queueMicrotask === 'function' ? queueMicrotask : queueMicrotaskShim;

const GET_INFO = 0;
const GET_PAYLOAD_LENGTH_16 = 1;
const GET_PAYLOAD_LENGTH_64 = 2;
const GET_MASK = 3;
const GET_DATA = 4;
const INFLATING = 5;
const DEFER_EVENT = 6;

/**
 * HyBi Receiver implementation.
 *
 * @extends Writable
 */
class Receiver extends Writable {
  /**
   * Creates a Receiver instance.
   *
   * @param {Object} [options] Options object
   * @param {Boolean} [options.allowSynchronousEvents=false] Specifies whether
   *     any of the `'message'`, `'ping'`, and `'pong'` events can be emitted
   *     multiple times in the same tick
   * @param {String} [options.binaryType=nodebuffer] The type for binary data
   * @param {Object} [options.extensions] An object containing the negotiated
   *     extensions
   * @param {Boolean} [options.isServer=false] Specifies whether to operate in
   *     client or server mode
   * @param {Number} [options.maxPayload=0] The maximum allowed message length
   * @param {Boolean} [options.skipUTF8Validation=false] Specifies whether or
   *     not to skip UTF-8 validation for text and close messages
   */
  constructor(options = {}) {
    super();

    this._allowSynchronousEvents = !!options.allowSynchronousEvents;
    this._binaryType = options.binaryType || BINARY_TYPES[0];
    this._extensions = options.extensions || {};
    this._isServer = !!options.isServer;
    this._maxPayload = options.maxPayload | 0;
    this._skipUTF8Validation = !!options.skipUTF8Validation;
    this[kWebSocket] = undefined;

    this._bufferedBytes = 0;
    this._buffers = [];

    this._compressed = false;
    this._payloadLength = 0;
    this._mask = undefined;
    this._fragmented = 0;
    this._masked = false;
    this._fin = false;
    this._opcode = 0;

    this._totalPayloadLength = 0;
    this._messageLength = 0;
    this._fragments = [];

    this._errored = false;
    this._loop = false;
    this._state = GET_INFO;
  }

  /**
   * Implements `Writable.prototype._write()`.
   *
   * @param {Buffer} chunk The chunk of data to write
   * @param {String} encoding The character encoding of `chunk`
   * @param {Function} cb Callback
   * @private
   */
  _write(chunk, encoding, cb) {
    if (this._opcode === 0x08 && this._state == GET_INFO) return cb();

    this._bufferedBytes += chunk.length;
    this._buffers.push(chunk);
    this.startLoop(cb);
  }

  /**
   * Consumes `n` bytes from the buffered data.
   *
   * @param {Number} n The number of bytes to consume
   * @return {Buffer} The consumed bytes
   * @private
   */
  consume(n) {
    this._bufferedBytes -= n;

    if (n === this._buffers[0].length) return this._buffers.shift();

    if (n < this._buffers[0].length) {
      const buf = this._buffers[0];
      this._buffers[0] = new FastBuffer(
        buf.buffer,
        buf.byteOffset + n,
        buf.length - n
      );

      return new FastBuffer(buf.buffer, buf.byteOffset, n);
    }

    const dst = Buffer.allocUnsafe(n);

    do {
      const buf = this._buffers[0];
      const offset = dst.length - n;

      if (n >= buf.length) {
        dst.set(this._buffers.shift(), offset);
      } else {
        dst.set(new Uint8Array(buf.buffer, buf.byteOffset, n), offset);
        this._buffers[0] = new FastBuffer(
          buf.buffer,
          buf.byteOffset + n,
          buf.length - n
        );
      }

      n -= buf.length;
    } while (n > 0);

    return dst;
  }

  /**
   * Starts the parsing loop.
   *
   * @param {Function} cb Callback
   * @private
   */
  startLoop(cb) {
    this._loop = true;

    do {
      switch (this._state) {
        case GET_INFO:
          this.getInfo(cb);
          break;
        case GET_PAYLOAD_LENGTH_16:
          this.getPayloadLength16(cb);
          break;
        case GET_PAYLOAD_LENGTH_64:
          this.getPayloadLength64(cb);
          break;
        case GET_MASK:
          this.getMask();
          break;
        case GET_DATA:
          this.getData(cb);
          break;
        case INFLATING:
        case DEFER_EVENT:
          this._loop = false;
          return;
      }
    } while (this._loop);

    if (!this._errored) cb();
  }

  /**
   * Reads the first two bytes of a frame.
   *
   * @param {Function} cb Callback
   * @private
   */
  getInfo(cb) {
    if (this._bufferedBytes < 2) {
      this._loop = false;
      return;
    }

    const buf = this.consume(2);

    if ((buf[0] & 0x30) !== 0x00) {
      const error = this.createError(
        RangeError,
        'RSV2 and RSV3 must be clear',
        true,
        1002,
        'WS_ERR_UNEXPECTED_RSV_2_3'
      );

      cb(error);
      return;
    }

    const compressed = (buf[0] & 0x40) === 0x40;

    if (compressed && !this._extensions[PerMessageDeflate.extensionName]) {
      const error = this.createError(
        RangeError,
        'RSV1 must be clear',
        true,
        1002,
        'WS_ERR_UNEXPECTED_RSV_1'
      );

      cb(error);
      return;
    }

    this._fin = (buf[0] & 0x80) === 0x80;
    this._opcode = buf[0] & 0x0f;
    this._payloadLength = buf[1] & 0x7f;

    if (this._opcode === 0x00) {
      if (compressed) {
        const error = this.createError(
          RangeError,
          'RSV1 must be clear',
          true,
          1002,
          'WS_ERR_UNEXPECTED_RSV_1'
        );

        cb(error);
        return;
      }

      if (!this._fragmented) {
        const error = this.createError(
          RangeError,
          'invalid opcode 0',
          true,
          1002,
          'WS_ERR_INVALID_OPCODE'
        );

        cb(error);
        return;
      }

      this._opcode = this._fragmented;
    } else if (this._opcode === 0x01 || this._opcode === 0x02) {
      if (this._fragmented) {
        const error = this.createError(
          RangeError,
          `invalid opcode ${this._opcode}`,
          true,
          1002,
          'WS_ERR_INVALID_OPCODE'
        );

        cb(error);
        return;
      }

      this._compressed = compressed;
    } else if (this._opcode > 0x07 && this._opcode < 0x0b) {
      if (!this._fin) {
        const error = this.createError(
          RangeError,
          'FIN must be set',
          true,
          1002,
          'WS_ERR_EXPECTED_FIN'
        );

        cb(error);
        return;
      }

      if (compressed) {
        const error = this.createError(
          RangeError,
          'RSV1 must be clear',
          true,
          1002,
          'WS_ERR_UNEXPECTED_RSV_1'
        );

        cb(error);
        return;
      }

      if (
        this._payloadLength > 0x7d ||
        (this._opcode === 0x08 && this._payloadLength === 1)
      ) {
        const error = this.createError(
          RangeError,
          `invalid payload length ${this._payloadLength}`,
          true,
          1002,
          'WS_ERR_INVALID_CONTROL_PAYLOAD_LENGTH'
        );

        cb(error);
        return;
      }
    } else {
      const error = this.createError(
        RangeError,
        `invalid opcode ${this._opcode}`,
        true,
        1002,
        'WS_ERR_INVALID_OPCODE'
      );

      cb(error);
      return;
    }

    if (!this._fin && !this._fragmented) this._fragmented = this._opcode;
    this._masked = (buf[1] & 0x80) === 0x80;

    if (this._isServer) {
      if (!this._masked) {
        const error = this.createError(
          RangeError,
          'MASK must be set',
          true,
          1002,
          'WS_ERR_EXPECTED_MASK'
        );

        cb(error);
        return;
      }
    } else if (this._masked) {
      const error = this.createError(
        RangeError,
        'MASK must be clear',
        true,
        1002,
        'WS_ERR_UNEXPECTED_MASK'
      );

      cb(error);
      return;
    }

    if (this._payloadLength === 126) this._state = GET_PAYLOAD_LENGTH_16;
    else if (this._payloadLength === 127) this._state = GET_PAYLOAD_LENGTH_64;
    else this.haveLength(cb);
  }

  /**
   * Gets extended payload length (7+16).
   *
   * @param {Function} cb Callback
   * @private
   */
  getPayloadLength16(cb) {
    if (this._bufferedBytes < 2) {
      this._loop = false;
      return;
    }

    this._payloadLength = this.consume(2).readUInt16BE(0);
    this.haveLength(cb);
  }

  /**
   * Gets extended payload length (7+64).
   *
   * @param {Function} cb Callback
   * @private
   */
  getPayloadLength64(cb) {
    if (this._bufferedBytes < 8) {
      this._loop = false;
      return;
    }

    const buf = this.consume(8);
    const num = buf.readUInt32BE(0);

    //
    // The maximum safe integer in JavaScript is 2^53 - 1. An error is returned
    // if payload length is greater than this number.
    //
    if (num > Math.pow(2, 53 - 32) - 1) {
      const error = this.createError(
        RangeError,
        'Unsupported WebSocket frame: payload length > 2^53 - 1',
        false,
        1009,
        'WS_ERR_UNSUPPORTED_DATA_PAYLOAD_LENGTH'
      );

      cb(error);
      return;
    }

    this._payloadLength = num * Math.pow(2, 32) + buf.readUInt32BE(4);
    this.haveLength(cb);
  }

  /**
   * Payload length has been read.
   *
   * @param {Function} cb Callback
   * @private
   */
  haveLength(cb) {
    if (this._payloadLength && this._opcode < 0x08) {
      this._totalPayloadLength += this._payloadLength;
      if (this._totalPayloadLength > this._maxPayload && this._maxPayload > 0) {
        const error = this.createError(
          RangeError,
          'Max payload size exceeded',
          false,
          1009,
          'WS_ERR_UNSUPPORTED_MESSAGE_LENGTH'
        );

        cb(error);
        return;
      }
    }

    if (this._masked) this._state = GET_MASK;
    else this._state = GET_DATA;
  }

  /**
   * Reads mask bytes.
   *
   * @private
   */
  getMask() {
    if (this._bufferedBytes < 4) {
      this._loop = false;
      return;
    }

    this._mask = this.consume(4);
    this._state = GET_DATA;
  }

  /**
   * Reads data bytes.
   *
   * @param {Function} cb Callback
   * @private
   */
  getData(cb) {
    let data = EMPTY_BUFFER;

    if (this._payloadLength) {
      if (this._bufferedBytes < this._payloadLength) {
        this._loop = false;
        return;
      }

      data = this.consume(this._payloadLength);

      if (
        this._masked &&
        (this._mask[0] | this._mask[1] | this._mask[2] | this._mask[3]) !== 0
      ) {
        unmask(data, this._mask);
      }
    }

    if (this._opcode > 0x07) {
      this.controlMessage(data, cb);
      return;
    }

    if (this._compressed) {
      this._state = INFLATING;
      this.decompress(data, cb);
      return;
    }

    if (data.length) {
      //
      // This message is not compressed so its length is the sum of the payload
      // length of all fragments.
      //
      this._messageLength = this._totalPayloadLength;
      this._fragments.push(data);
    }

    this.dataMessage(cb);
  }

  /**
   * Decompresses data.
   *
   * @param {Buffer} data Compressed data
   * @param {Function} cb Callback
   * @private
   */
  decompress(data, cb) {
    const perMessageDeflate = this._extensions[PerMessageDeflate.extensionName];

    perMessageDeflate.decompress(data, this._fin, (err, buf) => {
      if (err) return cb(err);

      if (buf.length) {
        this._messageLength += buf.length;
        if (this._messageLength > this._maxPayload && this._maxPayload > 0) {
          const error = this.createError(
            RangeError,
            'Max payload size exceeded',
            false,
            1009,
            'WS_ERR_UNSUPPORTED_MESSAGE_LENGTH'
          );

          cb(error);
          return;
        }

        this._fragments.push(buf);
      }

      this.dataMessage(cb);
      if (this._state === GET_INFO) this.startLoop(cb);
    });
  }

  /**
   * Handles a data message.
   *
   * @param {Function} cb Callback
   * @private
   */
  dataMessage(cb) {
    if (!this._fin) {
      this._state = GET_INFO;
      return;
    }

    const messageLength = this._messageLength;
    const fragments = this._fragments;

    this._totalPayloadLength = 0;
    this._messageLength = 0;
    this._fragmented = 0;
    this._fragments = [];

    if (this._opcode === 2) {
      let data;

      if (this._binaryType === 'nodebuffer') {
        data = concat(fragments, messageLength);
      } else if (this._binaryType === 'arraybuffer') {
        data = toArrayBuffer(concat(fragments, messageLength));
      } else {
        data = fragments;
      }

      //
      // If the state is `INFLATING`, it means that the frame data was
      // decompressed asynchronously, so there is no need to defer the event
      // as it will be emitted asynchronously anyway.
      //
      if (this._state === INFLATING || this._allowSynchronousEvents) {
        this.emit('message', data, true);
        this._state = GET_INFO;
      } else {
        this._state = DEFER_EVENT;
        queueTask(() => {
          this.emit('message', data, true);
          this._state = GET_INFO;
          this.startLoop(cb);
        });
      }
    } else {
      const buf = concat(fragments, messageLength);

      if (!this._skipUTF8Validation && !isValidUTF8(buf)) {
        const error = this.createError(
          Error,
          'invalid UTF-8 sequence',
          true,
          1007,
          'WS_ERR_INVALID_UTF8'
        );

        cb(error);
        return;
      }

      if (this._state === INFLATING || this._allowSynchronousEvents) {
        this.emit('message', buf, false);
        this._state = GET_INFO;
      } else {
        this._state = DEFER_EVENT;
        queueTask(() => {
          this.emit('message', buf, false);
          this._state = GET_INFO;
          this.startLoop(cb);
        });
      }
    }
  }

  /**
   * Handles a control message.
   *
   * @param {Buffer} data Data to handle
   * @return {(Error|RangeError|undefined)} A possible error
   * @private
   */
  controlMessage(data, cb) {
    if (this._opcode === 0x08) {
      if (data.length === 0) {
        this._loop = false;
        this.emit('conclude', 1005, EMPTY_BUFFER);
        this.end();
      } else {
        const code = data.readUInt16BE(0);

        if (!isValidStatusCode(code)) {
          const error = this.createError(
            RangeError,
            `invalid status code ${code}`,
            true,
            1002,
            'WS_ERR_INVALID_CLOSE_CODE'
          );

          cb(error);
          return;
        }

        const buf = new FastBuffer(
          data.buffer,
          data.byteOffset + 2,
          data.length - 2
        );

        if (!this._skipUTF8Validation && !isValidUTF8(buf)) {
          const error = this.createError(
            Error,
            'invalid UTF-8 sequence',
            true,
            1007,
            'WS_ERR_INVALID_UTF8'
          );

          cb(error);
          return;
        }

        this._loop = false;
        this.emit('conclude', code, buf);
        this.end();
      }

      this._state = GET_INFO;
      return;
    }

    if (this._allowSynchronousEvents) {
      this.emit(this._opcode === 0x09 ? 'ping' : 'pong', data);
      this._state = GET_INFO;
    } else {
      this._state = DEFER_EVENT;
      queueTask(() => {
        this.emit(this._opcode === 0x09 ? 'ping' : 'pong', data);
        this._state = GET_INFO;
        this.startLoop(cb);
      });
    }
  }

  /**
   * Builds an error object.
   *
   * @param {function(new:Error|RangeError)} ErrorCtor The error constructor
   * @param {String} message The error message
   * @param {Boolean} prefix Specifies whether or not to add a default prefix to
   *     `message`
   * @param {Number} statusCode The status code
   * @param {String} errorCode The exposed error code
   * @return {(Error|RangeError)} The error
   * @private
   */
  createError(ErrorCtor, message, prefix, statusCode, errorCode) {
    this._loop = false;
    this._errored = true;

    const err = new ErrorCtor(
      prefix ? `Invalid WebSocket frame: ${message}` : message
    );

    Error.captureStackTrace(err, this.createError);
    err.code = errorCode;
    err[kStatusCode] = statusCode;
    return err;
  }
}

module.exports = Receiver;

/**
 * A shim for `queueMicrotask()`.
 *
 * @param {Function} cb Callback
 */
function queueMicrotaskShim(cb) {
  promise.then(cb).catch(throwErrorNextTick);
}

/**
 * Throws an error.
 *
 * @param {Error} err The error to throw
 * @private
 */
function throwError(err) {
  throw err;
}

/**
 * Throws an error in the next tick.
 *
 * @param {Error} err The error to throw
 * @private
 */
function throwErrorNextTick(err) {
  process.nextTick(throwError, err);
}


/***/ }),
/* 31 */
/***/ ((module, __unused_webpack_exports, __webpack_require__) => {



const { isUtf8 } = __webpack_require__(32);

//
// Allowed token characters:
//
// '!', '#', '$', '%', '&', ''', '*', '+', '-',
// '.', 0-9, A-Z, '^', '_', '`', a-z, '|', '~'
//
// tokenChars[32] === 0 // ' '
// tokenChars[33] === 1 // '!'
// tokenChars[34] === 0 // '"'
// ...
//
// prettier-ignore
const tokenChars = [
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0 - 15
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 16 - 31
  0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0, // 32 - 47
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, // 48 - 63
  0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 64 - 79
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, // 80 - 95
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 96 - 111
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0 // 112 - 127
];

/**
 * Checks if a status code is allowed in a close frame.
 *
 * @param {Number} code The status code
 * @return {Boolean} `true` if the status code is valid, else `false`
 * @public
 */
function isValidStatusCode(code) {
  return (
    (code >= 1000 &&
      code <= 1014 &&
      code !== 1004 &&
      code !== 1005 &&
      code !== 1006) ||
    (code >= 3000 && code <= 4999)
  );
}

/**
 * Checks if a given buffer contains only correct UTF-8.
 * Ported from https://www.cl.cam.ac.uk/%7Emgk25/ucs/utf8_check.c by
 * Markus Kuhn.
 *
 * @param {Buffer} buf The buffer to check
 * @return {Boolean} `true` if `buf` contains only correct UTF-8, else `false`
 * @public
 */
function _isValidUTF8(buf) {
  const len = buf.length;
  let i = 0;

  while (i < len) {
    if ((buf[i] & 0x80) === 0) {
      // 0xxxxxxx
      i++;
    } else if ((buf[i] & 0xe0) === 0xc0) {
      // 110xxxxx 10xxxxxx
      if (
        i + 1 === len ||
        (buf[i + 1] & 0xc0) !== 0x80 ||
        (buf[i] & 0xfe) === 0xc0 // Overlong
      ) {
        return false;
      }

      i += 2;
    } else if ((buf[i] & 0xf0) === 0xe0) {
      // 1110xxxx 10xxxxxx 10xxxxxx
      if (
        i + 2 >= len ||
        (buf[i + 1] & 0xc0) !== 0x80 ||
        (buf[i + 2] & 0xc0) !== 0x80 ||
        (buf[i] === 0xe0 && (buf[i + 1] & 0xe0) === 0x80) || // Overlong
        (buf[i] === 0xed && (buf[i + 1] & 0xe0) === 0xa0) // Surrogate (U+D800 - U+DFFF)
      ) {
        return false;
      }

      i += 3;
    } else if ((buf[i] & 0xf8) === 0xf0) {
      // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
      if (
        i + 3 >= len ||
        (buf[i + 1] & 0xc0) !== 0x80 ||
        (buf[i + 2] & 0xc0) !== 0x80 ||
        (buf[i + 3] & 0xc0) !== 0x80 ||
        (buf[i] === 0xf0 && (buf[i + 1] & 0xf0) === 0x80) || // Overlong
        (buf[i] === 0xf4 && buf[i + 1] > 0x8f) ||
        buf[i] > 0xf4 // > U+10FFFF
      ) {
        return false;
      }

      i += 4;
    } else {
      return false;
    }
  }

  return true;
}

module.exports = {
  isValidStatusCode,
  isValidUTF8: _isValidUTF8,
  tokenChars
};

if (isUtf8) {
  module.exports.isValidUTF8 = function (buf) {
    return buf.length < 24 ? _isValidUTF8(buf) : isUtf8(buf);
  };
} /* istanbul ignore else  */ else if (!process.env.WS_NO_UTF_8_VALIDATE) {
  try {
    const isValidUTF8 = __webpack_require__(Object(function webpackMissingModule() { var e = new Error("Cannot find module 'utf-8-validate'"); e.code = 'MODULE_NOT_FOUND'; throw e; }()));

    module.exports.isValidUTF8 = function (buf) {
      return buf.length < 32 ? _isValidUTF8(buf) : isValidUTF8(buf);
    };
  } catch (e) {
    // Continue regardless of the error.
  }
}


/***/ }),
/* 32 */
/***/ ((module) => {

module.exports = require("buffer");

/***/ }),
/* 33 */
/***/ ((module, __unused_webpack_exports, __webpack_require__) => {

/* eslint no-unused-vars: ["error", { "varsIgnorePattern": "^Duplex" }] */



const { Duplex } = __webpack_require__(23);
const { randomFillSync } = __webpack_require__(22);

const PerMessageDeflate = __webpack_require__(25);
const { EMPTY_BUFFER } = __webpack_require__(28);
const { isValidStatusCode } = __webpack_require__(31);
const { mask: applyMask, toBuffer } = __webpack_require__(27);

const kByteLength = Symbol('kByteLength');
const maskBuffer = Buffer.alloc(4);

/**
 * HyBi Sender implementation.
 */
class Sender {
  /**
   * Creates a Sender instance.
   *
   * @param {Duplex} socket The connection socket
   * @param {Object} [extensions] An object containing the negotiated extensions
   * @param {Function} [generateMask] The function used to generate the masking
   *     key
   */
  constructor(socket, extensions, generateMask) {
    this._extensions = extensions || {};

    if (generateMask) {
      this._generateMask = generateMask;
      this._maskBuffer = Buffer.alloc(4);
    }

    this._socket = socket;

    this._firstFragment = true;
    this._compress = false;

    this._bufferedBytes = 0;
    this._deflating = false;
    this._queue = [];
  }

  /**
   * Frames a piece of data according to the HyBi WebSocket protocol.
   *
   * @param {(Buffer|String)} data The data to frame
   * @param {Object} options Options object
   * @param {Boolean} [options.fin=false] Specifies whether or not to set the
   *     FIN bit
   * @param {Function} [options.generateMask] The function used to generate the
   *     masking key
   * @param {Boolean} [options.mask=false] Specifies whether or not to mask
   *     `data`
   * @param {Buffer} [options.maskBuffer] The buffer used to store the masking
   *     key
   * @param {Number} options.opcode The opcode
   * @param {Boolean} [options.readOnly=false] Specifies whether `data` can be
   *     modified
   * @param {Boolean} [options.rsv1=false] Specifies whether or not to set the
   *     RSV1 bit
   * @return {(Buffer|String)[]} The framed data
   * @public
   */
  static frame(data, options) {
    let mask;
    let merge = false;
    let offset = 2;
    let skipMasking = false;

    if (options.mask) {
      mask = options.maskBuffer || maskBuffer;

      if (options.generateMask) {
        options.generateMask(mask);
      } else {
        randomFillSync(mask, 0, 4);
      }

      skipMasking = (mask[0] | mask[1] | mask[2] | mask[3]) === 0;
      offset = 6;
    }

    let dataLength;

    if (typeof data === 'string') {
      if (
        (!options.mask || skipMasking) &&
        options[kByteLength] !== undefined
      ) {
        dataLength = options[kByteLength];
      } else {
        data = Buffer.from(data);
        dataLength = data.length;
      }
    } else {
      dataLength = data.length;
      merge = options.mask && options.readOnly && !skipMasking;
    }

    let payloadLength = dataLength;

    if (dataLength >= 65536) {
      offset += 8;
      payloadLength = 127;
    } else if (dataLength > 125) {
      offset += 2;
      payloadLength = 126;
    }

    const target = Buffer.allocUnsafe(merge ? dataLength + offset : offset);

    target[0] = options.fin ? options.opcode | 0x80 : options.opcode;
    if (options.rsv1) target[0] |= 0x40;

    target[1] = payloadLength;

    if (payloadLength === 126) {
      target.writeUInt16BE(dataLength, 2);
    } else if (payloadLength === 127) {
      target[2] = target[3] = 0;
      target.writeUIntBE(dataLength, 4, 6);
    }

    if (!options.mask) return [target, data];

    target[1] |= 0x80;
    target[offset - 4] = mask[0];
    target[offset - 3] = mask[1];
    target[offset - 2] = mask[2];
    target[offset - 1] = mask[3];

    if (skipMasking) return [target, data];

    if (merge) {
      applyMask(data, mask, target, offset, dataLength);
      return [target];
    }

    applyMask(data, mask, data, 0, dataLength);
    return [target, data];
  }

  /**
   * Sends a close message to the other peer.
   *
   * @param {Number} [code] The status code component of the body
   * @param {(String|Buffer)} [data] The message component of the body
   * @param {Boolean} [mask=false] Specifies whether or not to mask the message
   * @param {Function} [cb] Callback
   * @public
   */
  close(code, data, mask, cb) {
    let buf;

    if (code === undefined) {
      buf = EMPTY_BUFFER;
    } else if (typeof code !== 'number' || !isValidStatusCode(code)) {
      throw new TypeError('First argument must be a valid error code number');
    } else if (data === undefined || !data.length) {
      buf = Buffer.allocUnsafe(2);
      buf.writeUInt16BE(code, 0);
    } else {
      const length = Buffer.byteLength(data);

      if (length > 123) {
        throw new RangeError('The message must not be greater than 123 bytes');
      }

      buf = Buffer.allocUnsafe(2 + length);
      buf.writeUInt16BE(code, 0);

      if (typeof data === 'string') {
        buf.write(data, 2);
      } else {
        buf.set(data, 2);
      }
    }

    const options = {
      [kByteLength]: buf.length,
      fin: true,
      generateMask: this._generateMask,
      mask,
      maskBuffer: this._maskBuffer,
      opcode: 0x08,
      readOnly: false,
      rsv1: false
    };

    if (this._deflating) {
      this.enqueue([this.dispatch, buf, false, options, cb]);
    } else {
      this.sendFrame(Sender.frame(buf, options), cb);
    }
  }

  /**
   * Sends a ping message to the other peer.
   *
   * @param {*} data The message to send
   * @param {Boolean} [mask=false] Specifies whether or not to mask `data`
   * @param {Function} [cb] Callback
   * @public
   */
  ping(data, mask, cb) {
    let byteLength;
    let readOnly;

    if (typeof data === 'string') {
      byteLength = Buffer.byteLength(data);
      readOnly = false;
    } else {
      data = toBuffer(data);
      byteLength = data.length;
      readOnly = toBuffer.readOnly;
    }

    if (byteLength > 125) {
      throw new RangeError('The data size must not be greater than 125 bytes');
    }

    const options = {
      [kByteLength]: byteLength,
      fin: true,
      generateMask: this._generateMask,
      mask,
      maskBuffer: this._maskBuffer,
      opcode: 0x09,
      readOnly,
      rsv1: false
    };

    if (this._deflating) {
      this.enqueue([this.dispatch, data, false, options, cb]);
    } else {
      this.sendFrame(Sender.frame(data, options), cb);
    }
  }

  /**
   * Sends a pong message to the other peer.
   *
   * @param {*} data The message to send
   * @param {Boolean} [mask=false] Specifies whether or not to mask `data`
   * @param {Function} [cb] Callback
   * @public
   */
  pong(data, mask, cb) {
    let byteLength;
    let readOnly;

    if (typeof data === 'string') {
      byteLength = Buffer.byteLength(data);
      readOnly = false;
    } else {
      data = toBuffer(data);
      byteLength = data.length;
      readOnly = toBuffer.readOnly;
    }

    if (byteLength > 125) {
      throw new RangeError('The data size must not be greater than 125 bytes');
    }

    const options = {
      [kByteLength]: byteLength,
      fin: true,
      generateMask: this._generateMask,
      mask,
      maskBuffer: this._maskBuffer,
      opcode: 0x0a,
      readOnly,
      rsv1: false
    };

    if (this._deflating) {
      this.enqueue([this.dispatch, data, false, options, cb]);
    } else {
      this.sendFrame(Sender.frame(data, options), cb);
    }
  }

  /**
   * Sends a data message to the other peer.
   *
   * @param {*} data The message to send
   * @param {Object} options Options object
   * @param {Boolean} [options.binary=false] Specifies whether `data` is binary
   *     or text
   * @param {Boolean} [options.compress=false] Specifies whether or not to
   *     compress `data`
   * @param {Boolean} [options.fin=false] Specifies whether the fragment is the
   *     last one
   * @param {Boolean} [options.mask=false] Specifies whether or not to mask
   *     `data`
   * @param {Function} [cb] Callback
   * @public
   */
  send(data, options, cb) {
    const perMessageDeflate = this._extensions[PerMessageDeflate.extensionName];
    let opcode = options.binary ? 2 : 1;
    let rsv1 = options.compress;

    let byteLength;
    let readOnly;

    if (typeof data === 'string') {
      byteLength = Buffer.byteLength(data);
      readOnly = false;
    } else {
      data = toBuffer(data);
      byteLength = data.length;
      readOnly = toBuffer.readOnly;
    }

    if (this._firstFragment) {
      this._firstFragment = false;
      if (
        rsv1 &&
        perMessageDeflate &&
        perMessageDeflate.params[
          perMessageDeflate._isServer
            ? 'server_no_context_takeover'
            : 'client_no_context_takeover'
        ]
      ) {
        rsv1 = byteLength >= perMessageDeflate._threshold;
      }
      this._compress = rsv1;
    } else {
      rsv1 = false;
      opcode = 0;
    }

    if (options.fin) this._firstFragment = true;

    if (perMessageDeflate) {
      const opts = {
        [kByteLength]: byteLength,
        fin: options.fin,
        generateMask: this._generateMask,
        mask: options.mask,
        maskBuffer: this._maskBuffer,
        opcode,
        readOnly,
        rsv1
      };

      if (this._deflating) {
        this.enqueue([this.dispatch, data, this._compress, opts, cb]);
      } else {
        this.dispatch(data, this._compress, opts, cb);
      }
    } else {
      this.sendFrame(
        Sender.frame(data, {
          [kByteLength]: byteLength,
          fin: options.fin,
          generateMask: this._generateMask,
          mask: options.mask,
          maskBuffer: this._maskBuffer,
          opcode,
          readOnly,
          rsv1: false
        }),
        cb
      );
    }
  }

  /**
   * Dispatches a message.
   *
   * @param {(Buffer|String)} data The message to send
   * @param {Boolean} [compress=false] Specifies whether or not to compress
   *     `data`
   * @param {Object} options Options object
   * @param {Boolean} [options.fin=false] Specifies whether or not to set the
   *     FIN bit
   * @param {Function} [options.generateMask] The function used to generate the
   *     masking key
   * @param {Boolean} [options.mask=false] Specifies whether or not to mask
   *     `data`
   * @param {Buffer} [options.maskBuffer] The buffer used to store the masking
   *     key
   * @param {Number} options.opcode The opcode
   * @param {Boolean} [options.readOnly=false] Specifies whether `data` can be
   *     modified
   * @param {Boolean} [options.rsv1=false] Specifies whether or not to set the
   *     RSV1 bit
   * @param {Function} [cb] Callback
   * @private
   */
  dispatch(data, compress, options, cb) {
    if (!compress) {
      this.sendFrame(Sender.frame(data, options), cb);
      return;
    }

    const perMessageDeflate = this._extensions[PerMessageDeflate.extensionName];

    this._bufferedBytes += options[kByteLength];
    this._deflating = true;
    perMessageDeflate.compress(data, options.fin, (_, buf) => {
      if (this._socket.destroyed) {
        const err = new Error(
          'The socket was closed while data was being compressed'
        );

        if (typeof cb === 'function') cb(err);

        for (let i = 0; i < this._queue.length; i++) {
          const params = this._queue[i];
          const callback = params[params.length - 1];

          if (typeof callback === 'function') callback(err);
        }

        return;
      }

      this._bufferedBytes -= options[kByteLength];
      this._deflating = false;
      options.readOnly = false;
      this.sendFrame(Sender.frame(buf, options), cb);
      this.dequeue();
    });
  }

  /**
   * Executes queued send operations.
   *
   * @private
   */
  dequeue() {
    while (!this._deflating && this._queue.length) {
      const params = this._queue.shift();

      this._bufferedBytes -= params[3][kByteLength];
      Reflect.apply(params[0], this, params.slice(1));
    }
  }

  /**
   * Enqueues a send operation.
   *
   * @param {Array} params Send operation parameters.
   * @private
   */
  enqueue(params) {
    this._bufferedBytes += params[3][kByteLength];
    this._queue.push(params);
  }

  /**
   * Sends a frame.
   *
   * @param {Buffer[]} list The frame to send
   * @param {Function} [cb] Callback
   * @private
   */
  sendFrame(list, cb) {
    if (list.length === 2) {
      this._socket.cork();
      this._socket.write(list[0]);
      this._socket.write(list[1], cb);
      this._socket.uncork();
    } else {
      this._socket.write(list[0], cb);
    }
  }
}

module.exports = Sender;


/***/ }),
/* 34 */
/***/ ((module, __unused_webpack_exports, __webpack_require__) => {



const { kForOnEventAttribute, kListener } = __webpack_require__(28);

const kCode = Symbol('kCode');
const kData = Symbol('kData');
const kError = Symbol('kError');
const kMessage = Symbol('kMessage');
const kReason = Symbol('kReason');
const kTarget = Symbol('kTarget');
const kType = Symbol('kType');
const kWasClean = Symbol('kWasClean');

/**
 * Class representing an event.
 */
class Event {
  /**
   * Create a new `Event`.
   *
   * @param {String} type The name of the event
   * @throws {TypeError} If the `type` argument is not specified
   */
  constructor(type) {
    this[kTarget] = null;
    this[kType] = type;
  }

  /**
   * @type {*}
   */
  get target() {
    return this[kTarget];
  }

  /**
   * @type {String}
   */
  get type() {
    return this[kType];
  }
}

Object.defineProperty(Event.prototype, 'target', { enumerable: true });
Object.defineProperty(Event.prototype, 'type', { enumerable: true });

/**
 * Class representing a close event.
 *
 * @extends Event
 */
class CloseEvent extends Event {
  /**
   * Create a new `CloseEvent`.
   *
   * @param {String} type The name of the event
   * @param {Object} [options] A dictionary object that allows for setting
   *     attributes via object members of the same name
   * @param {Number} [options.code=0] The status code explaining why the
   *     connection was closed
   * @param {String} [options.reason=''] A human-readable string explaining why
   *     the connection was closed
   * @param {Boolean} [options.wasClean=false] Indicates whether or not the
   *     connection was cleanly closed
   */
  constructor(type, options = {}) {
    super(type);

    this[kCode] = options.code === undefined ? 0 : options.code;
    this[kReason] = options.reason === undefined ? '' : options.reason;
    this[kWasClean] = options.wasClean === undefined ? false : options.wasClean;
  }

  /**
   * @type {Number}
   */
  get code() {
    return this[kCode];
  }

  /**
   * @type {String}
   */
  get reason() {
    return this[kReason];
  }

  /**
   * @type {Boolean}
   */
  get wasClean() {
    return this[kWasClean];
  }
}

Object.defineProperty(CloseEvent.prototype, 'code', { enumerable: true });
Object.defineProperty(CloseEvent.prototype, 'reason', { enumerable: true });
Object.defineProperty(CloseEvent.prototype, 'wasClean', { enumerable: true });

/**
 * Class representing an error event.
 *
 * @extends Event
 */
class ErrorEvent extends Event {
  /**
   * Create a new `ErrorEvent`.
   *
   * @param {String} type The name of the event
   * @param {Object} [options] A dictionary object that allows for setting
   *     attributes via object members of the same name
   * @param {*} [options.error=null] The error that generated this event
   * @param {String} [options.message=''] The error message
   */
  constructor(type, options = {}) {
    super(type);

    this[kError] = options.error === undefined ? null : options.error;
    this[kMessage] = options.message === undefined ? '' : options.message;
  }

  /**
   * @type {*}
   */
  get error() {
    return this[kError];
  }

  /**
   * @type {String}
   */
  get message() {
    return this[kMessage];
  }
}

Object.defineProperty(ErrorEvent.prototype, 'error', { enumerable: true });
Object.defineProperty(ErrorEvent.prototype, 'message', { enumerable: true });

/**
 * Class representing a message event.
 *
 * @extends Event
 */
class MessageEvent extends Event {
  /**
   * Create a new `MessageEvent`.
   *
   * @param {String} type The name of the event
   * @param {Object} [options] A dictionary object that allows for setting
   *     attributes via object members of the same name
   * @param {*} [options.data=null] The message content
   */
  constructor(type, options = {}) {
    super(type);

    this[kData] = options.data === undefined ? null : options.data;
  }

  /**
   * @type {*}
   */
  get data() {
    return this[kData];
  }
}

Object.defineProperty(MessageEvent.prototype, 'data', { enumerable: true });

/**
 * This provides methods for emulating the `EventTarget` interface. It's not
 * meant to be used directly.
 *
 * @mixin
 */
const EventTarget = {
  /**
   * Register an event listener.
   *
   * @param {String} type A string representing the event type to listen for
   * @param {(Function|Object)} handler The listener to add
   * @param {Object} [options] An options object specifies characteristics about
   *     the event listener
   * @param {Boolean} [options.once=false] A `Boolean` indicating that the
   *     listener should be invoked at most once after being added. If `true`,
   *     the listener would be automatically removed when invoked.
   * @public
   */
  addEventListener(type, handler, options = {}) {
    for (const listener of this.listeners(type)) {
      if (
        !options[kForOnEventAttribute] &&
        listener[kListener] === handler &&
        !listener[kForOnEventAttribute]
      ) {
        return;
      }
    }

    let wrapper;

    if (type === 'message') {
      wrapper = function onMessage(data, isBinary) {
        const event = new MessageEvent('message', {
          data: isBinary ? data : data.toString()
        });

        event[kTarget] = this;
        callListener(handler, this, event);
      };
    } else if (type === 'close') {
      wrapper = function onClose(code, message) {
        const event = new CloseEvent('close', {
          code,
          reason: message.toString(),
          wasClean: this._closeFrameReceived && this._closeFrameSent
        });

        event[kTarget] = this;
        callListener(handler, this, event);
      };
    } else if (type === 'error') {
      wrapper = function onError(error) {
        const event = new ErrorEvent('error', {
          error,
          message: error.message
        });

        event[kTarget] = this;
        callListener(handler, this, event);
      };
    } else if (type === 'open') {
      wrapper = function onOpen() {
        const event = new Event('open');

        event[kTarget] = this;
        callListener(handler, this, event);
      };
    } else {
      return;
    }

    wrapper[kForOnEventAttribute] = !!options[kForOnEventAttribute];
    wrapper[kListener] = handler;

    if (options.once) {
      this.once(type, wrapper);
    } else {
      this.on(type, wrapper);
    }
  },

  /**
   * Remove an event listener.
   *
   * @param {String} type A string representing the event type to remove
   * @param {(Function|Object)} handler The listener to remove
   * @public
   */
  removeEventListener(type, handler) {
    for (const listener of this.listeners(type)) {
      if (listener[kListener] === handler && !listener[kForOnEventAttribute]) {
        this.removeListener(type, listener);
        break;
      }
    }
  }
};

module.exports = {
  CloseEvent,
  ErrorEvent,
  Event,
  EventTarget,
  MessageEvent
};

/**
 * Call an event listener
 *
 * @param {(Function|Object)} listener The listener to call
 * @param {*} thisArg The value to use as `this`` when calling the listener
 * @param {Event} event The event to pass to the listener
 * @private
 */
function callListener(listener, thisArg, event) {
  if (typeof listener === 'object' && listener.handleEvent) {
    listener.handleEvent.call(listener, event);
  } else {
    listener.call(thisArg, event);
  }
}


/***/ }),
/* 35 */
/***/ ((module, __unused_webpack_exports, __webpack_require__) => {



const { tokenChars } = __webpack_require__(31);

/**
 * Adds an offer to the map of extension offers or a parameter to the map of
 * parameters.
 *
 * @param {Object} dest The map of extension offers or parameters
 * @param {String} name The extension or parameter name
 * @param {(Object|Boolean|String)} elem The extension parameters or the
 *     parameter value
 * @private
 */
function push(dest, name, elem) {
  if (dest[name] === undefined) dest[name] = [elem];
  else dest[name].push(elem);
}

/**
 * Parses the `Sec-WebSocket-Extensions` header into an object.
 *
 * @param {String} header The field value of the header
 * @return {Object} The parsed object
 * @public
 */
function parse(header) {
  const offers = Object.create(null);
  let params = Object.create(null);
  let mustUnescape = false;
  let isEscaping = false;
  let inQuotes = false;
  let extensionName;
  let paramName;
  let start = -1;
  let code = -1;
  let end = -1;
  let i = 0;

  for (; i < header.length; i++) {
    code = header.charCodeAt(i);

    if (extensionName === undefined) {
      if (end === -1 && tokenChars[code] === 1) {
        if (start === -1) start = i;
      } else if (
        i !== 0 &&
        (code === 0x20 /* ' ' */ || code === 0x09) /* '\t' */
      ) {
        if (end === -1 && start !== -1) end = i;
      } else if (code === 0x3b /* ';' */ || code === 0x2c /* ',' */) {
        if (start === -1) {
          throw new SyntaxError(`Unexpected character at index ${i}`);
        }

        if (end === -1) end = i;
        const name = header.slice(start, end);
        if (code === 0x2c) {
          push(offers, name, params);
          params = Object.create(null);
        } else {
          extensionName = name;
        }

        start = end = -1;
      } else {
        throw new SyntaxError(`Unexpected character at index ${i}`);
      }
    } else if (paramName === undefined) {
      if (end === -1 && tokenChars[code] === 1) {
        if (start === -1) start = i;
      } else if (code === 0x20 || code === 0x09) {
        if (end === -1 && start !== -1) end = i;
      } else if (code === 0x3b || code === 0x2c) {
        if (start === -1) {
          throw new SyntaxError(`Unexpected character at index ${i}`);
        }

        if (end === -1) end = i;
        push(params, header.slice(start, end), true);
        if (code === 0x2c) {
          push(offers, extensionName, params);
          params = Object.create(null);
          extensionName = undefined;
        }

        start = end = -1;
      } else if (code === 0x3d /* '=' */ && start !== -1 && end === -1) {
        paramName = header.slice(start, i);
        start = end = -1;
      } else {
        throw new SyntaxError(`Unexpected character at index ${i}`);
      }
    } else {
      //
      // The value of a quoted-string after unescaping must conform to the
      // token ABNF, so only token characters are valid.
      // Ref: https://tools.ietf.org/html/rfc6455#section-9.1
      //
      if (isEscaping) {
        if (tokenChars[code] !== 1) {
          throw new SyntaxError(`Unexpected character at index ${i}`);
        }
        if (start === -1) start = i;
        else if (!mustUnescape) mustUnescape = true;
        isEscaping = false;
      } else if (inQuotes) {
        if (tokenChars[code] === 1) {
          if (start === -1) start = i;
        } else if (code === 0x22 /* '"' */ && start !== -1) {
          inQuotes = false;
          end = i;
        } else if (code === 0x5c /* '\' */) {
          isEscaping = true;
        } else {
          throw new SyntaxError(`Unexpected character at index ${i}`);
        }
      } else if (code === 0x22 && header.charCodeAt(i - 1) === 0x3d) {
        inQuotes = true;
      } else if (end === -1 && tokenChars[code] === 1) {
        if (start === -1) start = i;
      } else if (start !== -1 && (code === 0x20 || code === 0x09)) {
        if (end === -1) end = i;
      } else if (code === 0x3b || code === 0x2c) {
        if (start === -1) {
          throw new SyntaxError(`Unexpected character at index ${i}`);
        }

        if (end === -1) end = i;
        let value = header.slice(start, end);
        if (mustUnescape) {
          value = value.replace(/\\/g, '');
          mustUnescape = false;
        }
        push(params, paramName, value);
        if (code === 0x2c) {
          push(offers, extensionName, params);
          params = Object.create(null);
          extensionName = undefined;
        }

        paramName = undefined;
        start = end = -1;
      } else {
        throw new SyntaxError(`Unexpected character at index ${i}`);
      }
    }
  }

  if (start === -1 || inQuotes || code === 0x20 || code === 0x09) {
    throw new SyntaxError('Unexpected end of input');
  }

  if (end === -1) end = i;
  const token = header.slice(start, end);
  if (extensionName === undefined) {
    push(offers, token, params);
  } else {
    if (paramName === undefined) {
      push(params, token, true);
    } else if (mustUnescape) {
      push(params, paramName, token.replace(/\\/g, ''));
    } else {
      push(params, paramName, token);
    }
    push(offers, extensionName, params);
  }

  return offers;
}

/**
 * Builds the `Sec-WebSocket-Extensions` header field value.
 *
 * @param {Object} extensions The map of extensions and parameters to format
 * @return {String} A string representing the given object
 * @public
 */
function format(extensions) {
  return Object.keys(extensions)
    .map((extension) => {
      let configurations = extensions[extension];
      if (!Array.isArray(configurations)) configurations = [configurations];
      return configurations
        .map((params) => {
          return [extension]
            .concat(
              Object.keys(params).map((k) => {
                let values = params[k];
                if (!Array.isArray(values)) values = [values];
                return values
                  .map((v) => (v === true ? k : `${k}=${v}`))
                  .join('; ');
              })
            )
            .join('; ');
        })
        .join(', ');
    })
    .join(', ');
}

module.exports = { format, parse };


/***/ }),
/* 36 */
/***/ ((module, __unused_webpack_exports, __webpack_require__) => {



const { Duplex } = __webpack_require__(23);

/**
 * Emits the `'close'` event on a stream.
 *
 * @param {Duplex} stream The stream.
 * @private
 */
function emitClose(stream) {
  stream.emit('close');
}

/**
 * The listener of the `'end'` event.
 *
 * @private
 */
function duplexOnEnd() {
  if (!this.destroyed && this._writableState.finished) {
    this.destroy();
  }
}

/**
 * The listener of the `'error'` event.
 *
 * @param {Error} err The error
 * @private
 */
function duplexOnError(err) {
  this.removeListener('error', duplexOnError);
  this.destroy();
  if (this.listenerCount('error') === 0) {
    // Do not suppress the throwing behavior.
    this.emit('error', err);
  }
}

/**
 * Wraps a `WebSocket` in a duplex stream.
 *
 * @param {WebSocket} ws The `WebSocket` to wrap
 * @param {Object} [options] The options for the `Duplex` constructor
 * @return {Duplex} The duplex stream
 * @public
 */
function createWebSocketStream(ws, options) {
  let terminateOnDestroy = true;

  const duplex = new Duplex({
    ...options,
    autoDestroy: false,
    emitClose: false,
    objectMode: false,
    writableObjectMode: false
  });

  ws.on('message', function message(msg, isBinary) {
    const data =
      !isBinary && duplex._readableState.objectMode ? msg.toString() : msg;

    if (!duplex.push(data)) ws.pause();
  });

  ws.once('error', function error(err) {
    if (duplex.destroyed) return;

    // Prevent `ws.terminate()` from being called by `duplex._destroy()`.
    //
    // - If the `'error'` event is emitted before the `'open'` event, then
    //   `ws.terminate()` is a noop as no socket is assigned.
    // - Otherwise, the error is re-emitted by the listener of the `'error'`
    //   event of the `Receiver` object. The listener already closes the
    //   connection by calling `ws.close()`. This allows a close frame to be
    //   sent to the other peer. If `ws.terminate()` is called right after this,
    //   then the close frame might not be sent.
    terminateOnDestroy = false;
    duplex.destroy(err);
  });

  ws.once('close', function close() {
    if (duplex.destroyed) return;

    duplex.push(null);
  });

  duplex._destroy = function (err, callback) {
    if (ws.readyState === ws.CLOSED) {
      callback(err);
      process.nextTick(emitClose, duplex);
      return;
    }

    let called = false;

    ws.once('error', function error(err) {
      called = true;
      callback(err);
    });

    ws.once('close', function close() {
      if (!called) callback(err);
      process.nextTick(emitClose, duplex);
    });

    if (terminateOnDestroy) ws.terminate();
  };

  duplex._final = function (callback) {
    if (ws.readyState === ws.CONNECTING) {
      ws.once('open', function open() {
        duplex._final(callback);
      });
      return;
    }

    // If the value of the `_socket` property is `null` it means that `ws` is a
    // client websocket and the handshake failed. In fact, when this happens, a
    // socket is never assigned to the websocket. Wait for the `'error'` event
    // that will be emitted by the websocket.
    if (ws._socket === null) return;

    if (ws._socket._writableState.finished) {
      callback();
      if (duplex._readableState.endEmitted) duplex.destroy();
    } else {
      ws._socket.once('finish', function finish() {
        // `duplex` is not destroyed here because the `'end'` event will be
        // emitted on `duplex` after this `'finish'` event. The EOF signaling
        // `null` chunk is, in fact, pushed when the websocket emits `'close'`.
        callback();
      });
      ws.close();
    }
  };

  duplex._read = function () {
    if (ws.isPaused) ws.resume();
  };

  duplex._write = function (chunk, encoding, callback) {
    if (ws.readyState === ws.CONNECTING) {
      ws.once('open', function open() {
        duplex._write(chunk, encoding, callback);
      });
      return;
    }

    ws.send(chunk, callback);
  };

  duplex.on('end', duplexOnEnd);
  duplex.on('error', duplexOnError);
  return duplex;
}

module.exports = createWebSocketStream;


/***/ }),
/* 37 */
/***/ ((module, __unused_webpack_exports, __webpack_require__) => {

/* eslint no-unused-vars: ["error", { "varsIgnorePattern": "^Duplex$" }] */



const EventEmitter = __webpack_require__(17);
const http = __webpack_require__(19);
const { Duplex } = __webpack_require__(23);
const { createHash } = __webpack_require__(22);

const extension = __webpack_require__(35);
const PerMessageDeflate = __webpack_require__(25);
const subprotocol = __webpack_require__(38);
const WebSocket = __webpack_require__(16);
const { GUID, kWebSocket } = __webpack_require__(28);

const keyRegex = /^[+/0-9A-Za-z]{22}==$/;

const RUNNING = 0;
const CLOSING = 1;
const CLOSED = 2;

/**
 * Class representing a WebSocket server.
 *
 * @extends EventEmitter
 */
class WebSocketServer extends EventEmitter {
  /**
   * Create a `WebSocketServer` instance.
   *
   * @param {Object} options Configuration options
   * @param {Boolean} [options.allowSynchronousEvents=false] Specifies whether
   *     any of the `'message'`, `'ping'`, and `'pong'` events can be emitted
   *     multiple times in the same tick
   * @param {Boolean} [options.autoPong=true] Specifies whether or not to
   *     automatically send a pong in response to a ping
   * @param {Number} [options.backlog=511] The maximum length of the queue of
   *     pending connections
   * @param {Boolean} [options.clientTracking=true] Specifies whether or not to
   *     track clients
   * @param {Function} [options.handleProtocols] A hook to handle protocols
   * @param {String} [options.host] The hostname where to bind the server
   * @param {Number} [options.maxPayload=104857600] The maximum allowed message
   *     size
   * @param {Boolean} [options.noServer=false] Enable no server mode
   * @param {String} [options.path] Accept only connections matching this path
   * @param {(Boolean|Object)} [options.perMessageDeflate=false] Enable/disable
   *     permessage-deflate
   * @param {Number} [options.port] The port where to bind the server
   * @param {(http.Server|https.Server)} [options.server] A pre-created HTTP/S
   *     server to use
   * @param {Boolean} [options.skipUTF8Validation=false] Specifies whether or
   *     not to skip UTF-8 validation for text and close messages
   * @param {Function} [options.verifyClient] A hook to reject connections
   * @param {Function} [options.WebSocket=WebSocket] Specifies the `WebSocket`
   *     class to use. It must be the `WebSocket` class or class that extends it
   * @param {Function} [callback] A listener for the `listening` event
   */
  constructor(options, callback) {
    super();

    options = {
      allowSynchronousEvents: false,
      autoPong: true,
      maxPayload: 100 * 1024 * 1024,
      skipUTF8Validation: false,
      perMessageDeflate: false,
      handleProtocols: null,
      clientTracking: true,
      verifyClient: null,
      noServer: false,
      backlog: null, // use default (511 as implemented in net.js)
      server: null,
      host: null,
      path: null,
      port: null,
      WebSocket,
      ...options
    };

    if (
      (options.port == null && !options.server && !options.noServer) ||
      (options.port != null && (options.server || options.noServer)) ||
      (options.server && options.noServer)
    ) {
      throw new TypeError(
        'One and only one of the "port", "server", or "noServer" options ' +
          'must be specified'
      );
    }

    if (options.port != null) {
      this._server = http.createServer((req, res) => {
        const body = http.STATUS_CODES[426];

        res.writeHead(426, {
          'Content-Length': body.length,
          'Content-Type': 'text/plain'
        });
        res.end(body);
      });
      this._server.listen(
        options.port,
        options.host,
        options.backlog,
        callback
      );
    } else if (options.server) {
      this._server = options.server;
    }

    if (this._server) {
      const emitConnection = this.emit.bind(this, 'connection');

      this._removeListeners = addListeners(this._server, {
        listening: this.emit.bind(this, 'listening'),
        error: this.emit.bind(this, 'error'),
        upgrade: (req, socket, head) => {
          this.handleUpgrade(req, socket, head, emitConnection);
        }
      });
    }

    if (options.perMessageDeflate === true) options.perMessageDeflate = {};
    if (options.clientTracking) {
      this.clients = new Set();
      this._shouldEmitClose = false;
    }

    this.options = options;
    this._state = RUNNING;
  }

  /**
   * Returns the bound address, the address family name, and port of the server
   * as reported by the operating system if listening on an IP socket.
   * If the server is listening on a pipe or UNIX domain socket, the name is
   * returned as a string.
   *
   * @return {(Object|String|null)} The address of the server
   * @public
   */
  address() {
    if (this.options.noServer) {
      throw new Error('The server is operating in "noServer" mode');
    }

    if (!this._server) return null;
    return this._server.address();
  }

  /**
   * Stop the server from accepting new connections and emit the `'close'` event
   * when all existing connections are closed.
   *
   * @param {Function} [cb] A one-time listener for the `'close'` event
   * @public
   */
  close(cb) {
    if (this._state === CLOSED) {
      if (cb) {
        this.once('close', () => {
          cb(new Error('The server is not running'));
        });
      }

      process.nextTick(emitClose, this);
      return;
    }

    if (cb) this.once('close', cb);

    if (this._state === CLOSING) return;
    this._state = CLOSING;

    if (this.options.noServer || this.options.server) {
      if (this._server) {
        this._removeListeners();
        this._removeListeners = this._server = null;
      }

      if (this.clients) {
        if (!this.clients.size) {
          process.nextTick(emitClose, this);
        } else {
          this._shouldEmitClose = true;
        }
      } else {
        process.nextTick(emitClose, this);
      }
    } else {
      const server = this._server;

      this._removeListeners();
      this._removeListeners = this._server = null;

      //
      // The HTTP/S server was created internally. Close it, and rely on its
      // `'close'` event.
      //
      server.close(() => {
        emitClose(this);
      });
    }
  }

  /**
   * See if a given request should be handled by this server instance.
   *
   * @param {http.IncomingMessage} req Request object to inspect
   * @return {Boolean} `true` if the request is valid, else `false`
   * @public
   */
  shouldHandle(req) {
    if (this.options.path) {
      const index = req.url.indexOf('?');
      const pathname = index !== -1 ? req.url.slice(0, index) : req.url;

      if (pathname !== this.options.path) return false;
    }

    return true;
  }

  /**
   * Handle a HTTP Upgrade request.
   *
   * @param {http.IncomingMessage} req The request object
   * @param {Duplex} socket The network socket between the server and client
   * @param {Buffer} head The first packet of the upgraded stream
   * @param {Function} cb Callback
   * @public
   */
  handleUpgrade(req, socket, head, cb) {
    socket.on('error', socketOnError);

    const key = req.headers['sec-websocket-key'];
    const version = +req.headers['sec-websocket-version'];

    if (req.method !== 'GET') {
      const message = 'Invalid HTTP method';
      abortHandshakeOrEmitwsClientError(this, req, socket, 405, message);
      return;
    }

    if (req.headers.upgrade.toLowerCase() !== 'websocket') {
      const message = 'Invalid Upgrade header';
      abortHandshakeOrEmitwsClientError(this, req, socket, 400, message);
      return;
    }

    if (!key || !keyRegex.test(key)) {
      const message = 'Missing or invalid Sec-WebSocket-Key header';
      abortHandshakeOrEmitwsClientError(this, req, socket, 400, message);
      return;
    }

    if (version !== 8 && version !== 13) {
      const message = 'Missing or invalid Sec-WebSocket-Version header';
      abortHandshakeOrEmitwsClientError(this, req, socket, 400, message);
      return;
    }

    if (!this.shouldHandle(req)) {
      abortHandshake(socket, 400);
      return;
    }

    const secWebSocketProtocol = req.headers['sec-websocket-protocol'];
    let protocols = new Set();

    if (secWebSocketProtocol !== undefined) {
      try {
        protocols = subprotocol.parse(secWebSocketProtocol);
      } catch (err) {
        const message = 'Invalid Sec-WebSocket-Protocol header';
        abortHandshakeOrEmitwsClientError(this, req, socket, 400, message);
        return;
      }
    }

    const secWebSocketExtensions = req.headers['sec-websocket-extensions'];
    const extensions = {};

    if (
      this.options.perMessageDeflate &&
      secWebSocketExtensions !== undefined
    ) {
      const perMessageDeflate = new PerMessageDeflate(
        this.options.perMessageDeflate,
        true,
        this.options.maxPayload
      );

      try {
        const offers = extension.parse(secWebSocketExtensions);

        if (offers[PerMessageDeflate.extensionName]) {
          perMessageDeflate.accept(offers[PerMessageDeflate.extensionName]);
          extensions[PerMessageDeflate.extensionName] = perMessageDeflate;
        }
      } catch (err) {
        const message =
          'Invalid or unacceptable Sec-WebSocket-Extensions header';
        abortHandshakeOrEmitwsClientError(this, req, socket, 400, message);
        return;
      }
    }

    //
    // Optionally call external client verification handler.
    //
    if (this.options.verifyClient) {
      const info = {
        origin:
          req.headers[`${version === 8 ? 'sec-websocket-origin' : 'origin'}`],
        secure: !!(req.socket.authorized || req.socket.encrypted),
        req
      };

      if (this.options.verifyClient.length === 2) {
        this.options.verifyClient(info, (verified, code, message, headers) => {
          if (!verified) {
            return abortHandshake(socket, code || 401, message, headers);
          }

          this.completeUpgrade(
            extensions,
            key,
            protocols,
            req,
            socket,
            head,
            cb
          );
        });
        return;
      }

      if (!this.options.verifyClient(info)) return abortHandshake(socket, 401);
    }

    this.completeUpgrade(extensions, key, protocols, req, socket, head, cb);
  }

  /**
   * Upgrade the connection to WebSocket.
   *
   * @param {Object} extensions The accepted extensions
   * @param {String} key The value of the `Sec-WebSocket-Key` header
   * @param {Set} protocols The subprotocols
   * @param {http.IncomingMessage} req The request object
   * @param {Duplex} socket The network socket between the server and client
   * @param {Buffer} head The first packet of the upgraded stream
   * @param {Function} cb Callback
   * @throws {Error} If called more than once with the same socket
   * @private
   */
  completeUpgrade(extensions, key, protocols, req, socket, head, cb) {
    //
    // Destroy the socket if the client has already sent a FIN packet.
    //
    if (!socket.readable || !socket.writable) return socket.destroy();

    if (socket[kWebSocket]) {
      throw new Error(
        'server.handleUpgrade() was called more than once with the same ' +
          'socket, possibly due to a misconfiguration'
      );
    }

    if (this._state > RUNNING) return abortHandshake(socket, 503);

    const digest = createHash('sha1')
      .update(key + GUID)
      .digest('base64');

    const headers = [
      'HTTP/1.1 101 Switching Protocols',
      'Upgrade: websocket',
      'Connection: Upgrade',
      `Sec-WebSocket-Accept: ${digest}`
    ];

    const ws = new this.options.WebSocket(null, undefined, this.options);

    if (protocols.size) {
      //
      // Optionally call external protocol selection handler.
      //
      const protocol = this.options.handleProtocols
        ? this.options.handleProtocols(protocols, req)
        : protocols.values().next().value;

      if (protocol) {
        headers.push(`Sec-WebSocket-Protocol: ${protocol}`);
        ws._protocol = protocol;
      }
    }

    if (extensions[PerMessageDeflate.extensionName]) {
      const params = extensions[PerMessageDeflate.extensionName].params;
      const value = extension.format({
        [PerMessageDeflate.extensionName]: [params]
      });
      headers.push(`Sec-WebSocket-Extensions: ${value}`);
      ws._extensions = extensions;
    }

    //
    // Allow external modification/inspection of handshake headers.
    //
    this.emit('headers', headers, req);

    socket.write(headers.concat('\r\n').join('\r\n'));
    socket.removeListener('error', socketOnError);

    ws.setSocket(socket, head, {
      allowSynchronousEvents: this.options.allowSynchronousEvents,
      maxPayload: this.options.maxPayload,
      skipUTF8Validation: this.options.skipUTF8Validation
    });

    if (this.clients) {
      this.clients.add(ws);
      ws.on('close', () => {
        this.clients.delete(ws);

        if (this._shouldEmitClose && !this.clients.size) {
          process.nextTick(emitClose, this);
        }
      });
    }

    cb(ws, req);
  }
}

module.exports = WebSocketServer;

/**
 * Add event listeners on an `EventEmitter` using a map of <event, listener>
 * pairs.
 *
 * @param {EventEmitter} server The event emitter
 * @param {Object.<String, Function>} map The listeners to add
 * @return {Function} A function that will remove the added listeners when
 *     called
 * @private
 */
function addListeners(server, map) {
  for (const event of Object.keys(map)) server.on(event, map[event]);

  return function removeListeners() {
    for (const event of Object.keys(map)) {
      server.removeListener(event, map[event]);
    }
  };
}

/**
 * Emit a `'close'` event on an `EventEmitter`.
 *
 * @param {EventEmitter} server The event emitter
 * @private
 */
function emitClose(server) {
  server._state = CLOSED;
  server.emit('close');
}

/**
 * Handle socket errors.
 *
 * @private
 */
function socketOnError() {
  this.destroy();
}

/**
 * Close the connection when preconditions are not fulfilled.
 *
 * @param {Duplex} socket The socket of the upgrade request
 * @param {Number} code The HTTP response status code
 * @param {String} [message] The HTTP response body
 * @param {Object} [headers] Additional HTTP response headers
 * @private
 */
function abortHandshake(socket, code, message, headers) {
  //
  // The socket is writable unless the user destroyed or ended it before calling
  // `server.handleUpgrade()` or in the `verifyClient` function, which is a user
  // error. Handling this does not make much sense as the worst that can happen
  // is that some of the data written by the user might be discarded due to the
  // call to `socket.end()` below, which triggers an `'error'` event that in
  // turn causes the socket to be destroyed.
  //
  message = message || http.STATUS_CODES[code];
  headers = {
    Connection: 'close',
    'Content-Type': 'text/html',
    'Content-Length': Buffer.byteLength(message),
    ...headers
  };

  socket.once('finish', socket.destroy);

  socket.end(
    `HTTP/1.1 ${code} ${http.STATUS_CODES[code]}\r\n` +
      Object.keys(headers)
        .map((h) => `${h}: ${headers[h]}`)
        .join('\r\n') +
      '\r\n\r\n' +
      message
  );
}

/**
 * Emit a `'wsClientError'` event on a `WebSocketServer` if there is at least
 * one listener for it, otherwise call `abortHandshake()`.
 *
 * @param {WebSocketServer} server The WebSocket server
 * @param {http.IncomingMessage} req The request object
 * @param {Duplex} socket The socket of the upgrade request
 * @param {Number} code The HTTP response status code
 * @param {String} message The HTTP response body
 * @private
 */
function abortHandshakeOrEmitwsClientError(server, req, socket, code, message) {
  if (server.listenerCount('wsClientError')) {
    const err = new Error(message);
    Error.captureStackTrace(err, abortHandshakeOrEmitwsClientError);

    server.emit('wsClientError', err, socket, req);
  } else {
    abortHandshake(socket, code, message);
  }
}


/***/ }),
/* 38 */
/***/ ((module, __unused_webpack_exports, __webpack_require__) => {



const { tokenChars } = __webpack_require__(31);

/**
 * Parses the `Sec-WebSocket-Protocol` header into a set of subprotocol names.
 *
 * @param {String} header The field value of the header
 * @return {Set} The subprotocol names
 * @public
 */
function parse(header) {
  const protocols = new Set();
  let start = -1;
  let end = -1;
  let i = 0;

  for (i; i < header.length; i++) {
    const code = header.charCodeAt(i);

    if (end === -1 && tokenChars[code] === 1) {
      if (start === -1) start = i;
    } else if (
      i !== 0 &&
      (code === 0x20 /* ' ' */ || code === 0x09) /* '\t' */
    ) {
      if (end === -1 && start !== -1) end = i;
    } else if (code === 0x2c /* ',' */) {
      if (start === -1) {
        throw new SyntaxError(`Unexpected character at index ${i}`);
      }

      if (end === -1) end = i;

      const protocol = header.slice(start, end);

      if (protocols.has(protocol)) {
        throw new SyntaxError(`The "${protocol}" subprotocol is duplicated`);
      }

      protocols.add(protocol);
      start = end = -1;
    } else {
      throw new SyntaxError(`Unexpected character at index ${i}`);
    }
  }

  if (start === -1 || end !== -1) {
    throw new SyntaxError('Unexpected end of input');
  }

  const protocol = header.slice(start, i);

  if (protocols.has(protocol)) {
    throw new SyntaxError(`The "${protocol}" subprotocol is duplicated`);
  }

  protocols.add(protocol);
  return protocols;
}

module.exports = { parse };


/***/ }),
/* 39 */
/***/ ((__unused_webpack_module, __webpack_exports__, __webpack_require__) => {

__webpack_require__.r(__webpack_exports__);
/* harmony export */ __webpack_require__.d(__webpack_exports__, {
/* harmony export */   GenTable: () => (/* binding */ GenTable)
/* harmony export */ });
/* harmony import */ var _utils_js__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(3);
/**
 * GenTable class
 */




class GenTable {
    constructor() {
        this.props = {
            "top": 0,
            "left": 0,
            "width": 200,
            "height": 100,
            "type": "gentable",
            "colour": "#888888",
            "outlineColour": "#dddddd",
            "outlineWidth": 1,
            "channel": "gentable",
            "backgroundColour": "#a8d388",
            "fontColour": "#dddddd",
            "fontFamily": "Verdana",
            "fontSize": 0,
            "corners": 4,
            "align": "centre",
            "visible": 1,
            "text": "Default Label",
            "tableNumber": 1,
            "samples": []
        }

        this.panelSections = {
            "Properties": ["type"],
            "Bounds": ["left", "top", "width", "height"],
            "Text": ["text", "fontColour", "fontSize", "fontFamily", "align"],
            "Colours": ["colour", "outlineColour", "backgroundColour"]
        };

        // Create canvas element during initialization
        this.canvas = document.createElement('canvas');
        this.ctx = this.canvas.getContext('2d');
    }

    addVsCodeEventListeners(widgetDiv, vs) {
        this.vscode = vs;
        widgetDiv.addEventListener("pointerdown", this.pointerDown.bind(this));
    }

    addEventListeners(widgetDiv) {
        widgetDiv.addEventListener("pointerdown", this.pointerDown.bind(this));
    }

    pointerDown() {
        console.log("Label clicked!");
    }

    getInnerHTML() {
        return ``;
    }

    updateTable(obj) {
        this.props.samples = obj["data"];
        this.canvas.width = this.props.width;
        this.canvas.height = this.props.height;
        // Clear canvas
        this.ctx.clearRect(0, 0, this.props.width, this.props.height);

        // Draw background
        this.ctx.fillStyle = this.props.backgroundColour;
        this.ctx.fillRect(0, 0, this.props.width, this.props.height);

        // Draw waveform
        this.ctx.strokeStyle = this.props.colour;
        this.ctx.lineWidth = this.props.outlineWidth;
        this.ctx.beginPath();
        this.ctx.moveTo(0, this.props.height / 2);

        if (!Array.isArray(this.props.samples) || this.props.samples.length === 0) {
            console.warn('No samples to draw.');
        } else {
            for (let i = 0; i < this.props.samples.length; i++) {
                const x = _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageUtils.map(i, 0, this.props.samples.length, 0, this.props.width);
                const y = _utils_js__WEBPACK_IMPORTED_MODULE_0__.CabbageUtils.map(this.props.samples[i], -1, 1, this.props.height, 0);
                this.ctx.lineTo(x, y);
            }
        }

        this.ctx.stroke();
        this.ctx.closePath();

        // Update DOM with the canvas
        const widgetElement = document.getElementById(this.props.channel);
        if (widgetElement) {
            widgetElement.style.transform = `translate(${this.props.left}px, ${this.props.top}px)`;
            widgetElement.setAttribute('data-x', this.props.left);
            widgetElement.setAttribute('data-y', this.props.top);
            widgetElement.style.top = `${this.props.top}px`;
            widgetElement.style.left = `${this.props.left}px`;

            widgetElement.innerHTML = ''; // Clear existing content
            widgetElement.appendChild(this.canvas); // Append canvas
        } else {
            console.error(`Element with channel ${this.props.channel} not found.`);
        }

    }


}


/***/ })
/******/ 	]);
/************************************************************************/
/******/ 	// The module cache
/******/ 	var __webpack_module_cache__ = {};
/******/ 	
/******/ 	// The require function
/******/ 	function __webpack_require__(moduleId) {
/******/ 		// Check if module is in cache
/******/ 		var cachedModule = __webpack_module_cache__[moduleId];
/******/ 		if (cachedModule !== undefined) {
/******/ 			return cachedModule.exports;
/******/ 		}
/******/ 		// Create a new module (and put it into the cache)
/******/ 		var module = __webpack_module_cache__[moduleId] = {
/******/ 			// no module.id needed
/******/ 			// no module.loaded needed
/******/ 			exports: {}
/******/ 		};
/******/ 	
/******/ 		// Execute the module function
/******/ 		__webpack_modules__[moduleId].call(module.exports, module, module.exports, __webpack_require__);
/******/ 	
/******/ 		// Return the exports of the module
/******/ 		return module.exports;
/******/ 	}
/******/ 	
/************************************************************************/
/******/ 	/* webpack/runtime/define property getters */
/******/ 	(() => {
/******/ 		// define getter functions for harmony exports
/******/ 		__webpack_require__.d = (exports, definition) => {
/******/ 			for(var key in definition) {
/******/ 				if(__webpack_require__.o(definition, key) && !__webpack_require__.o(exports, key)) {
/******/ 					Object.defineProperty(exports, key, { enumerable: true, get: definition[key] });
/******/ 				}
/******/ 			}
/******/ 		};
/******/ 	})();
/******/ 	
/******/ 	/* webpack/runtime/hasOwnProperty shorthand */
/******/ 	(() => {
/******/ 		__webpack_require__.o = (obj, prop) => (Object.prototype.hasOwnProperty.call(obj, prop))
/******/ 	})();
/******/ 	
/******/ 	/* webpack/runtime/make namespace object */
/******/ 	(() => {
/******/ 		// define __esModule on exports
/******/ 		__webpack_require__.r = (exports) => {
/******/ 			if(typeof Symbol !== 'undefined' && Symbol.toStringTag) {
/******/ 				Object.defineProperty(exports, Symbol.toStringTag, { value: 'Module' });
/******/ 			}
/******/ 			Object.defineProperty(exports, '__esModule', { value: true });
/******/ 		};
/******/ 	})();
/******/ 	
/************************************************************************/
/******/ 	
/******/ 	// startup
/******/ 	// Load entry module and return exports
/******/ 	// This entry module is referenced by other modules so it can't be inlined
/******/ 	var __webpack_exports__ = __webpack_require__(0);
/******/ 	module.exports = __webpack_exports__;
/******/ 	
/******/ })()
;
//# sourceMappingURL=extension.js.map