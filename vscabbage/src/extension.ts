// The module 'vscode' contains the VS Code extensibility API
// Import the module and reference it with the alias vscode in your code below
import * as vscode from 'vscode';

// @ts-ignore
import { RotarySlider } from "./rotarySlider.js";
// @ts-ignore
import { Form } from "./form.js";
import * as cp from "child_process";

let textEditor: vscode.TextEditor | undefined;
let output: vscode.OutputChannel;
let panel: vscode.WebviewPanel | undefined = undefined;

import WebSocket from 'ws';

const wss = new WebSocket.Server({ port: 9991 });
let websocket: WebSocket;

wss.on('connection', (ws) => {
	console.log('Client connected');
	websocket = ws;
	ws.on('message', (message) => {
		const msg = JSON.parse(message.toString());
		// console.log(JSON.stringify(msg["widgetUpdate"], null, 2));
		if(panel)
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
		const widgetTypes = ["button", "optionbutton", "checkbox", "combobox", "csoundoutput", "encoder", "fftdisplay", "filebutton", "presetbutton", "form",
			"gentable", "groupbox", "hmeter", "hrange", "hslider", "image", "webview", "infobutton", "keyboard", "label", "listbox", "nslider",
			"rslider", "signaldisplay", "soundfiler", "textbox", "texteditor", "vmeter", "vrange", "vslider", "xypad"];

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

		vscode.workspace.onDidChangeTextDocument((editor)=>{
			// sendTextToWebView(editor.document, 'onFileChanged');
		})

		//notify webview when various updates take place in editor
		vscode.workspace.onDidSaveTextDocument(async (editor) => {
			sendTextToWebView(editor, 'onFileChanged');
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
						updateText(message.text);
						return;
					case 'channelUpdate':
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
		if(websocket)
			websocket.send(JSON.stringify(msg));
		processes.forEach((p) => {
			p?.kill("SIGKILL");
		})
		sendTextToWebView(textEditor?.document, 'onEnterEditMode');
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
        const { min, max, value, skew, increment } = obj;
        syntax = `range(${min}, ${max}, ${value}, ${skew}, ${increment})`;
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

	
	if (currentWidgetObj['type'].indexOf('slider') > -1){
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
			case 'form':
				defaultProps = new Form().props;
				break;
			default:
				break;
		}


		const internalIdentifiers:string[] = ['top', 'left', 'width', 'name', 'height', 'increment', 'min', 'max', 'skew', 'index'];
		if(props.type.indexOf('slider')!=-1)
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

							if(props.type.indexOf('slider')>-1){
								const rangeIndex = tokens.findIndex(({ token }) => token === 'range');
								tokens[rangeIndex].values = [props.min, props.max, props.value, props.skew, props.increment];
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
					
					if(props.type.indexOf('slider') > -1){
						newLine+=` ${getIdentifierFromJson(jsonText, "range")}`
					}

					editBuilder.insert(new vscode.Position(lineNumber, 0), newLine+'\n');
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
  <svg width="420" height="400" viewBox="0 0 420 400" fill="none" xmlns="http://www.w3.org/2000/svg">
<g clip-path="url(#clip0_543_2)">
<path d="M250.474 31.0422C317.909 43.973 362.949 90.9013 387.59 150.201L400.749 188.562L410.806 202.947C417.963 214.552 418.248 220.338 418.098 233.317C417.746 260.601 394.597 280.708 366.134 279.606C324.514 278.007 305.84 230.232 328.905 199.751C339.885 185.221 354.92 180.618 372.839 180.57L357.234 139.012C340.136 103.417 314.272 76.6758 278.97 56.8559C262.04 47.3616 241.188 41.016 221.978 37.3557C212.944 35.6295 198.679 36.3648 193.482 28.7245C216.078 26.9823 227.761 26.6786 250.474 31.0422ZM77.8728 91.0612C68.1004 100.955 59.6689 112.447 53.2992 124.627C45.1192 140.259 37.9785 164.73 36.2017 182.169L34.5422 204.546C34.3746 210.236 36.2184 213.257 34.5422 218.931C23.0935 192.206 18.7521 166.728 34.5422 140.611C41.6997 129.838 46.9295 125.314 56.0315 116.635C45.9573 119.848 39.42 122.006 30.9551 128.655C8.19183 146.525 1.53718 180.602 12.1645 206.144C20.6798 226.683 30.8042 227.626 37.5929 241.309C0.112389 220.434 -6.13996 165.577 20.026 134.217C25.2894 127.904 31.7764 122.965 39.2692 119.241C44.432 116.667 51.0531 115.372 54.9755 111.361L68.1674 92.6596C79.3143 78.7377 81.9963 78.1143 94.5848 67.0855C94.1322 76.0205 84.0078 84.8595 77.8728 91.0612ZM399.592 293.863C368.716 318.11 322.469 312.005 298.968 281.268C285.457 263.622 281.585 239.982 286.53 218.931C291.659 197.129 305.941 177.102 327.581 167.623C333.163 165.178 351.283 158.992 356.513 162.972C359.077 164.922 359.077 168.183 359.429 170.98C349.69 171.811 342.533 174.305 334.286 179.371C316.015 190.576 302.572 210.588 302.572 231.718C302.572 263.67 335.359 296.117 369.487 293.767C394.681 292.025 401.184 282.706 419.774 270.079C414.192 279.973 408.761 286.67 399.592 293.863Z" fill="#FCFFFF"/>
<path d="M234.675 370.909H231.077L222.081 371.481L217.283 370.962C181.915 368.548 143.153 357.834 114.124 337.074C73.8325 308.272 53.2549 263.867 51.1257 215.3L50.55 208.742C50.334 190.183 53.2549 166.15 59.1565 148.526C64.5783 132.351 71.7634 117.381 82.2711 103.811C92.6409 90.4144 100.726 85.919 109.176 69.2314L126.833 35.2479C128.195 33.0301 132.441 24.7667 134.114 23.7592C135.817 22.7277 137.389 24.1348 138.714 25.1245C140.849 26.7103 143.776 29.3634 146.511 29.6198C149.672 29.9119 151.981 27.396 153.684 25.1125C154.728 23.7234 155.819 21.4697 157.931 22.2329C159.256 22.7098 161.637 25.2437 162.705 26.3049C165.919 29.4946 172.978 37.5195 177.099 38.4853C180.367 39.2544 183.234 37.2273 185.489 36.8338C188.854 36.2496 191.091 40.0712 192.776 42.4023L200.147 53.1339L208.082 63.8656L219.879 81.7516C230.615 97.7596 240.409 111.21 245.813 130.044C249.04 141.288 249.202 148.973 249.07 160.45C248.992 166.758 246.551 179.66 245.123 186.087C240.625 206.34 233.752 226.098 232.222 246.899L231.677 252.861V261.208C231.713 283.697 242.262 310.406 254.282 329.175L269.989 351.234L278.458 361.966C268.55 366.288 245.549 370.778 234.675 370.909ZM172.9 25.1125L172.301 25.7087V25.1125H172.9ZM294.051 33.4593L293.452 34.0555V33.4593H294.051ZM295.251 34.0555L294.651 34.6517V34.0555H295.251ZM296.45 34.6517L295.851 35.2479V34.6517H296.45ZM297.65 35.2479L297.05 35.8441V35.2479H297.65ZM298.85 35.8441L298.25 36.4403V35.8441H298.85ZM300.049 36.4403L299.449 37.0365V36.4403H300.049ZM301.249 37.0365L300.649 37.6327V37.0365H301.249ZM302.448 37.6327L301.848 38.2289V37.6327H302.448ZM303.648 38.2289L303.048 38.8251V38.2289H303.648ZM337.234 59.4894L349.481 69.7143C350.453 69.9766 351.79 69.8216 352.828 69.7143C362.184 70.0124 369.111 77.1072 374.161 84.1364C384.699 98.7969 390.091 115.753 394.433 133.025C396.646 141.825 398.139 150.839 399.105 159.854C399.399 162.578 400.478 168.219 399.609 170.585C399.027 163.699 393.815 148.824 391.242 141.968C380.53 113.404 361.788 85.0307 339.033 64.5631C332.37 58.5713 325.449 52.8597 318.042 47.7801C314.779 45.5443 307.798 41.7584 305.447 39.4213C316.069 43.9704 328.136 52.3768 337.234 59.4894ZM225.679 48.9605C227.808 49.7654 230.261 50.1231 231.677 51.9356H219.682C216.767 51.9773 210.781 53.277 208.394 51.9356C206.637 50.934 204.502 47.1839 203.488 45.3833C209.066 45.6397 220.605 47.0467 225.679 48.9605ZM235.275 48.9605L234.675 49.5567V48.9605H235.275ZM234.675 54.9226C243.852 54.9344 250.065 58.5475 258.066 62.5241C272.43 69.6666 286.165 78.4427 298.25 88.9954C304.985 94.8859 308.745 98.7314 314.491 105.6C315.811 107.18 319.049 110.733 319.127 112.754C319.181 114.394 317.226 117.321 316.326 118.716C313.262 123.444 309.909 128.106 306.293 132.429C293.41 147.852 282.65 155.358 268.262 168.302C264.555 171.635 256.093 181.532 253.268 183.106C254.569 176.017 256.255 168.236 256.267 161.046V149.122C256.249 136.107 250.887 119.378 244.685 107.984L221.073 71.02C218.848 67.5143 213.102 59.2748 211.885 56.115L234.675 54.9226ZM279.657 70.4238L279.058 71.02V70.4238H279.657ZM281.457 71.6162L280.857 72.2124V71.6162H281.457ZM324.039 118.12C331.548 129.316 338.787 142.349 343.232 155.084C334.283 157.439 329.761 157.94 321.041 162.394C301.656 172.291 286.836 191.226 280.275 211.723C267.488 251.675 287.532 298.655 328.837 312.171C337.378 314.967 342.776 315.462 351.628 315.462C348.623 320.572 340.749 327.255 336.034 331.208C324.555 340.836 313.652 347.591 300.049 353.804C295.407 355.92 290.657 358.418 285.655 359.581L276.311 348.253C268.568 338.386 260.609 327.148 254.755 316.058C238.712 285.634 235.473 255.735 243.594 222.455C245.897 213.023 247.384 201.272 252.872 193.241C271.986 165.273 305.195 150.899 322.84 118.12H324.039Z" fill="#93D200"/>
</g>
<defs>
<clipPath id="clip0_543_2">
<rect width="420" height="400" fill="white"/>
</clipPath>
</defs>
</svg>

</div>
  	<script>
  		var vscodeMode = true; 
	</script>
  <script type="module" src="${widgetWrapper}"></script>
  <script type="module" src="${mainJS}"></script>
</body>

</html>`}

