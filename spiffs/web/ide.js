/**
 * This is the IDE for ESP32-Duktape
 */
$(document).ready(function() {
	
	var consoleConnected = true; // Is the console web socket connected?
	var settings;
	
	if (localStorage.settings) {
		settings = JSON.parse(localStorage.settings);
	} else {
		var settings = {
			esp32_host: "192.168.1.99",
			esp32_port: 80,
			theme: "eclipse"
		}	
	}
	
	/**
	 * Run a script on the ESP32 Duktape environment by sending in a POST request
	 * with the body of the message being the script to execute.
	 * 
	 * @param script A string representation of the script to run.
	 * @returns N/A
	 */
	function runScript(script) {
		$.ajax({
			url: "http://" + settings.esp32_host + ":" + settings.esp32_port + "/run",
			contentType: "application/javascript",
			method: "POST",
			data: script,
			success: function() {
				console.log("All sent");
			}
		});
	} // runScript
	
	
	/**
	 * Connect to the ESP32 for the console websocket.
	 * @param onOpen A callback function to be invoked when the socket has been established.
	 * @returns N/A
	 */
	function createConsoleWebSocket(onOpen) {
		return;
		var ws = new WebSocket("ws://" + settings.esp32_host + ":" + settings.esp32_port + "/console");
		// When a console message arrives, append it to the console log and scroll so that it is visible.
		ws.onmessage = function(event) {
			$("#console").val($("#console").val() + event.data);
			$('#console').scrollTop($('#console')[0].scrollHeight);
		}
		ws.onclose = function() {
			$("#newWebSocket").button("enable");
			$("#console").prop("disabled", true);
			consoleConnected = false;
		}
		ws.onopen = function() {
			consoleConnected = true;
			$("#console").prop("disabled", false);
			if (onOpen != null) {
				onOpen();
			}
		}		
	} // createConsoleWebSocket
	
	// Create the page layout.
	$("#c1").layout({
		applyDefaultStyles: true,
		// The center pane is the editor.
		center: {
			onresize: function() {
				editor.resize();
			}
		},
		// The south pane is the console.
		south: {
			size: 200, // Set its initial height.
			closable: true // Flag the console pane as closable.
		}
	});

	// Create the ace editor
	var editor = ace.edit("editor");
	editor.setTheme("ace/theme/" + settings.theme);
	editor.getSession().setMode("ace/mode/javascript"); // Set the language to be JavaScript
	editor.setFontSize(16);
	editor.setOption("showPrintMargin", false);
	
	// Handle the run button
	$("#run").button({
		icon: "ui-icon-play" // Set the icon on the button to be the play icon.
	}).click(function() {
		if (consoleConnected) {
			runScript(editor.getValue());
		} else {
			createConsoleWebSocket(function() {
				runScript(editor.getValue());
			});
		}
	});
	
	// Handle the clear console button.
	$("#clearConsole").button({
		icon: "ui-icon-trash" // Set the icon on the button to be the trash can.
	}).click(function() {
		$("#console").val("");
	});
	
	// Handle the settings button.
	$("#settings").button({
		icon: "ui-icon-wrench" // Set the icon on the button to be the wrench.
	}).click(function() {
		$("#settingsHost").val(settings.esp32_host);
		$("#settingsPort").val(settings.esp32_port);
		$("#fontSize").val(editor.getFontSize());
		$("#theme").val(settings.theme);
		$("#settingsDialog").dialog("open");
	});
	
	$("#info").click(function(){
		console.log("Info!");
		open("https://github.com/nkolban/duktape-esp32");
	});
	
	$("#repl").on("keypress", function(e) {
		if (e.keyCode == 13) {
			runScript($("#repl").val());
			return false;
		}
	});
	
	// Create and handle the settings dialog.
	$("#settingsDialog").dialog({
		autoOpen: false, // Do not auto open the dialog
		modal: true, // Dialog should be model.
		width: 400, // Width of dialog.
		buttons: [
			// The OK button is clicked to confirm the entries.
			{
				text: "OK",
				click: function() {
					settings.esp32_host = $("#settingsHost").val();
					settings.esp32_port = $("#settingsPort").val();
					settings.theme = $("#theme option:selected").val();
					localStorage = JSON.stringify(settings);
					
					editor.setFontSize($("#fontSize").val());
					editor.setTheme("ace/theme/" + settings.theme);
					$(this).dialog("close");
				}
			},
			// The Cancel button is clicked to cancel any settings changes.
			{
				text: "Cancel",
				click: function() {
					$(this).dialog("close");
				}
			}
		] // Array of buttons
	}); // Settings dialog
}); // onReady.