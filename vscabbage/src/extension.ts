// The module 'vscode' contains the VS Code extensibility API
// Import the module and reference it with the alias vscode in your code below
import * as vscode from 'vscode';

import { Form, RotarySlider } from "./widgets.js";
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
		panel.webview.html = getWebviewContent(mainJS, styles, cabbageStyles, interactJS, widgetSVGs, widgetWrapper, menuItems);
		panel.webview.options = { enableScripts: true };

		//assign current textEditor so we can track it even if focus changes to the webview
		panel.onDidChangeViewState(() => {
			textEditor = vscode.window.activeTextEditor;
		})

		vscode.workspace.onDidChangeTextDocument((editor)=>{
			// sendTextToWebView(editor.document, 'onFileChanged');
		})

		//notify webview when various updates take place in editor
		vscode.workspace.onDidSaveTextDocument((editor) => {
			sendTextToWebView(editor, 'onFileChanged');
			const config = vscode.workspace.getConfiguration("cabbage");
			const command = config.get("pathToCabbageExecutable") + '/CabbageApp.app/Contents/MacOS/CabbageApp';
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
        const { min, max, value, sliderSkew, increment } = obj;
        syntax = `range(${min}, ${max}, ${value}, ${sliderSkew}, ${increment})`;
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
 * array for any identifiers that are different to their default values
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
		updatedIdentifiers.push('sliderSkew');
		updatedIdentifiers.push('increment');

	}
	return updatedIdentifiers;
}

/**
 * This function will update the text associated with a widget
 */
function updateText(jsonText: string) {
	const props = JSON.parse(jsonText);
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


		const internalIdentifiers:string[] = ['top', 'left', 'width', 'name', 'height', 'increment', 'min', 'max', 'sliderSkew'];
		textEditor.edit(editBuilder => {
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
								//only want to display user-accessible identifiers...
								if (!internalIdentifiers.includes(ident)) {
									const newIndex = tokens.findIndex(({ token }) => token == ident);
									//each token has an array of values with it..
									const data: string[] = [];
									data.push(props[ident])
									if (newIndex == -1) {
										const identifier: string = ident;
										tokens.push({ token: identifier, values: data });
									}
									else {
										tokens[newIndex].values = data;
									}
								}
							})

							if(props.type.indexOf('slider')>-1){
								const rangeIndex = tokens.findIndex(({ token }) => token === 'range');
								tokens[rangeIndex].values = [props.min, props.max, props.value, props.sliderSkew, props.increment];
							}

							const boundsIndex = tokens.findIndex(({ token }) => token === 'bounds');
							tokens[boundsIndex].values = [props.left, props.top, props.width, props.height];
							
							lines[i] = `${lines[i].split(' ')[0]} ` + tokens.map(({ token, values }) =>
								typeof values[0] === 'string' ? `${token}("${values.join(', ')}")` : `${token}(${values.join(', ')})`
							).join(', ');
							editBuilder.replace(new vscode.Range(document.lineAt(i).range.start, document.lineAt(i).range.end), lines[i]);
							textEditor.selection = new vscode.Selection(i, 0, i, 10000);
						}
						else {
							console.log('found a widget without a channel...')
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
function getWebviewContent(mainJS: vscode.Uri, styles: vscode.Uri, cabbageStyles: vscode.Uri, interactJS: vscode.Uri, widgetSVGs: vscode.Uri, widgetWrapper: vscode.Uri, menu: string) {
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
  <div id="LeftCol" class="full-height-div">
    <div id="MainForm" class="form editMode">
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
  <div id="RightCol" class="full-height-div">
    <div class="property-panel full-height-div">
      <!-- Properties will be dynamically added here -->
    </div>
  </div>
</div>
  <script>var vscodeMode = true; </script>
  <script type="module" src="${widgetSVGs}"></script>
  <script type="module" src="${widgetWrapper}"></script>
  <script type="module" src="${mainJS}"></script>
</body>

</html>`}




// 	`<!DOCTYPE html>
//   <html lang="en">
//   <head>
// 	  <meta charset="UTF-8">
// 	  <meta name="viewport" content="width=device-width, initial-scale=1.0">
// 	  <title>Cat Coding</title>
//   </head>
//   <body>
// 	  <h1>Hello rory</h1>
// 	  <script>
//   (function() {
// 	  const vscode = acquireVsCodeApi();
// 	  addEventListener("mousedown", (event) => {
// 		vscode.postMessage({
// 			command: 'updateText',
// 			text: 'Update to fuck!'
// 		})
// 	  });
//   }())
// </script>
//   </body>

//   </html>
//   `;
// }

// This method is called when your extension is deactivated
export function deactivate() { }
