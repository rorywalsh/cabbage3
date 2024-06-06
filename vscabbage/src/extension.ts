// The module 'vscode' contains the VS Code extensibility API
// Import the module and reference it with the alias vscode in your code below
import * as vscode from 'vscode';

// @ts-ignore
import { RotarySlider } from "./rotarySlider.js";
// @ts-ignore
import { HorizontalSlider } from "./horizontalSlider.js";
// @ts-ignore
import { VerticalSlider } from "./verticalSlider.js";
// @ts-ignore
import { Form } from "./form.js";
import * as cp from "child_process";

let textEditor: vscode.TextEditor | undefined;
let output: vscode.OutputChannel;
let panel: vscode.WebviewPanel | undefined = undefined;

import WebSocket from 'ws';

const wss = new WebSocket.Server({ port: 9991 });
let websocket: WebSocket;
let cabbageMode = "play";

wss.on('connection', (ws) => {
	console.log('Client connected');
	websocket = ws;
	ws.on('message', (message) => {
		const msg = JSON.parse(message.toString());
		// console.log(JSON.stringify(msg["widgetUpdate"], null, 2));
		if (panel)
			panel.webview.postMessage({ command: "widgetUpdate", text: JSON.stringify(msg["widgetUpdate"]) })

	});

	ws.on('close', () => {
		console.log('Client disconnected');
	});

});


let processes: (cp.ChildProcess | undefined)[] = [];

// This method is called when your extension is activated
// Your extension is activated the very first time the command is executed
export function activate(context: vscode.ExtensionContext) {

	if (output === undefined) {
		output = vscode.window.createOutputChannel("Cabbage output");
	}

	output.clear();
	output.show(true); // true means keep focus in the editor window

	// Use the console to output diagnostic information (console.log) and errors (console.error)
	// This line of code will only be executed once when your extension is activated
	console.log('Congratulations, your extension "cabbage" is now active!');


	//send text to webview for parsing if file has an extension of csd and contains valid Cabbage tags
	function sendTextToWebView(editor: vscode.TextDocument | undefined, command: string) {
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
		panel = vscode.window.createWebviewPanel(
			'cabbageUIEditor',
			'Cabbage UI Editor',
			//load in second column, I guess this could be controlled by settings
			vscode.ViewColumn.Two,
			{}
		);

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
		onDiskPath = vscode.Uri.joinPath(context.extensionUri, 'src', 'widgets.js');
		const widgetSVGs = panel.webview.asWebviewUri(onDiskPath);
		onDiskPath = vscode.Uri.joinPath(context.extensionUri, 'src', 'widgetWrapper.js');
		const widgetWrapper = panel.webview.asWebviewUri(onDiskPath);

		//add widget types to menu
		const widgetTypes = ["hslider", "rslider", "vslider"];

		let menuItems = "";
		widgetTypes.forEach((widget) => {
			menuItems += `
			<li class="menuItem">
			<span>${widget}</span>
	  		</li>
			`;
		});


		// set webview HTML content and options
		panel.webview.html = getWebviewContent(mainJS, styles, cabbageStyles, interactJS, widgetWrapper, menuItems);
		panel.webview.options = { enableScripts: true };

		//assign current textEditor so we can track it even if focus changes to the webview
		panel.onDidChangeViewState(() => {
			textEditor = vscode.window.activeTextEditor;
		})

		vscode.workspace.onDidChangeTextDocument((editor) => {
			// sendTextToWebView(editor.document, 'onFileChanged');
		})

		//notify webview when various updates take place in editor
		vscode.workspace.onDidSaveTextDocument(async (editor) => {
			sendTextToWebView(editor, 'onFileChanged');
			cabbageMode = "play";
			const command = config.get("pathToCabbageExecutable") + '/CabbageApp.app/Contents/MacOS/CabbageApp';
			const path = vscode.Uri.file(command);
			try {
				// Attempt to read the directory (or file)
				await vscode.workspace.fs.stat(path);
				output.append("Found Cabbage service app...")
			} catch (error) {
				// If an error is thrown, it means the path does not exist
				output.append(`ERROR: Could not locate Cabbage service app at ${path.fsPath}. Please check the path in the Cabbage extension settings.\n`);
				return;
			}

			processes.forEach((p) => {
				p?.kill("SIGKILL");
			})
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
		panel.webview.onDidReceiveMessage(
			message => {
				switch (message.command) {
					case 'widgetUpdate':
						if(cabbageMode != "play")
							updateText(message.text);
						return;
					case 'channelUpdate':
						if(websocket)
							websocket.send(JSON.stringify(message.text));
					// console.log(message.text);
					case 'ready': //trigger when webview is open
						if (panel)
							panel.webview.postMessage({ command: "snapToSize", text: config.get("snapToSize") });
				}
			},
			undefined,
			context.subscriptions
		);
	})
	);


	context.subscriptions.push(vscode.commands.registerCommand('cabbage.editMode', () => {
		// The code you place here will be executed every time your command is executed
		// Place holder - but these commands will eventually launch Cabbage standalone 
		// with the file currently in focus. 
		if (!panel) {
			return;
		}
		const msg = { event: "stopCsound" };
		if (websocket)
			websocket.send(JSON.stringify(msg));
		processes.forEach((p) => {
			p?.kill("SIGKILL");
		})
		sendTextToWebView(textEditor?.document, 'onEnterEditMode');
		cabbageMode = "draggable";
	})
	);

}

/**
 * This uses a simple regex pattern to get tokens from a line of Cabbage code
 */
function getTokens(text: string) {
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

/**
 * This function will return an identifier in the form of ident(param) from an incoming
 * JSON object of properties
 */
function getIdentifierFromJson(json: string, name: string): string {
	const obj = JSON.parse(json);
	let syntax = '';

	if (name === 'range' && obj['type'].indexOf('slider') > -1) {
		const { min, max, defaultValue, skew, increment } = obj;
		syntax = `range(${min}, ${max}, ${defaultValue}, ${skew}, ${increment})`;
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
function findUpdatedIdentifiers(initial: string, current: string) {
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

/**
 * This function will update the text associated with a widget
 */
async function updateText(jsonText: string) {
	if(cabbageMode === "play")
		return;

	const props = JSON.parse(jsonText);
	// console.log(JSON.stringify(props, null, 2));

	if (textEditor) {
		const document = textEditor.document;
		let lineNumber = 0;
		//get default props so we can compare them to incoming ones and display any that are different
		let defaultProps = {};
		switch (props.type) {
			case 'rslider':
				defaultProps = new RotarySlider().props;
				break;
			case 'hslider':
				defaultProps = new HorizontalSlider().props;
				break;
			case 'vslider':
				defaultProps = new VerticalSlider().props;
				break;
			case 'form':
				defaultProps = new Form().props;
				break;
			default:
				break;
		}


		const internalIdentifiers: string[] = ['top', 'left', 'width', 'defaultValue', 'name', 'height', 'increment', 'min', 'max', 'skew', 'index'];
		if (props.type.indexOf('slider') != -1)
			internalIdentifiers.push('value');

		await textEditor.edit(async editBuilder => {
			if (textEditor) {

				let foundChannel = false;
				let lines = document.getText().split(/\r?\n/);
				for (let i = 0; i < lines.length; i++) {
					let tokens = getTokens(lines[i]);
					const index = tokens.findIndex(({ token }) => token === 'channel');

					if (index != -1) {
						const channel = tokens[index].values[0].replace(/"/g, "");
						if (channel == props.channel) {
							foundChannel = true;
							//found entry - now update bounds
							const updatedIdentifiers = findUpdatedIdentifiers(JSON.stringify(defaultProps), jsonText);

							updatedIdentifiers.forEach((ident) => {
								// Only want to display user-accessible identifiers...
								if (!internalIdentifiers.includes(ident)) {
									const newIndex = tokens.findIndex(({ token }) => token === ident);

									// Create the data array with the appropriate type based on props[ident]
									let data: any[] = [];

									if (Array.isArray(props[ident])) {
										data = [...props[ident]];
									} else {
										data.push(props[ident]);
									}

									if (newIndex === -1) {
										const identifier: string = ident;
										tokens.push({ token: identifier, values: data });
									} else {
										tokens[newIndex].values = data;
									}
								}
							});

							if (props.type.indexOf('slider') > -1) {
								const rangeIndex = tokens.findIndex(({ token }) => token === 'range');
								if(rangeIndex != -1)
									tokens[rangeIndex].values = [props.min, props.max, props.defaultValue, props.skew, props.increment];
							}

							const boundsIndex = tokens.findIndex(({ token }) => token === 'bounds');
							tokens[boundsIndex].values = [props.left, props.top, props.width, props.height];


							lines[i] = `${lines[i].split(' ')[0]} ` + tokens.map(({ token, values }) =>
								typeof values[0] === 'string' ? `${token}("${values.join(', ')}")` : `${token}(${values.join(', ')})`
							).join(', ');

							// console.log(lines[i]);
							await editBuilder.replace(new vscode.Range(document.lineAt(i).range.start, document.lineAt(i).range.end), lines[i]);
							textEditor.selection = new vscode.Selection(i, 0, i, 10000);
						}
						else {

						}
					}
					if (lines[i] === '</Cabbage>')
						break;
				}

				let count = 0;
				lines.forEach((line) => {
					if (line.trimStart().startsWith("</Cabbage>"))
						lineNumber = count;
					count++;
				})

				//this is called when we create a widgets from the popup menu in the UI builder
				if (!foundChannel && props.type != "form") {
					let newLine = `${props.type} bounds(${props.left}, ${props.top}, ${props.width}, ${props.height}), ${getIdentifierFromJson(jsonText, "channel")}`;

					if (props.type.indexOf('slider') > -1) {
						newLine += ` ${getIdentifierFromJson(jsonText, "range")}`
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
function getWebviewContent(mainJS: vscode.Uri, styles: vscode.Uri, cabbageStyles: vscode.Uri, interactJS: vscode.Uri, widgetWrapper: vscode.Uri, menu: string) {
	return `
<!doctype html>
<html lang="en">

<head>
  <meta charset="UTF-8" />
  <link rel="icon" type="image/svg+xml" href="/vite.svg" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <script type="module" src="${interactJS}"></script>
  <link href="${styles}" rel="stylesheet">
  <link href="${cabbageStyles}" rel="stylesheet">  
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

</html>`}

