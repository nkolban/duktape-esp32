$(document).ready(function() {
	var consoleConnected = false; // Is the console web socket connected?
	var settings = {
		esp32_host: "192.168.1.99",
		esp32_port: 80
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
		var ws = new WebSocket("ws://" + settings.esp32_host + ":" + settings.esp32_port + "/console");
		// When a console message arrives, append it to the console log and scroll so that it is visible.
		ws.onmessage = function(event) {
			$("#console").val($("#console").val() + event.data);
			$('#console').scrollTop($('#console')[0].scrollHeight);
		}
		ws.onclose = function() {
			$("#newWebSocket").button("enable");
			$("#console").prop("disabled", true);
			consoleConnect = false;
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
	editor.setTheme("ace/theme/eclipse");
	editor.getSession().setMode("ace/mode/javascript");
	editor.setFontSize(16);
	editor.setOption("showPrintMargin", false);
	
	// Handle the run button
	$("#run").button({
		icon: "ui-icon-play"
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
		icon: "ui-icon-trash"
	}).click(function() {
		$("#console").val("");
	});
	
	// Handle the settings button.
	$("#settings").button({
		icon: "ui-icon-wrench"
	}).click(function() {
		$("#settingsHost").val(settings.esp32_host);
		$("#settingsPort").val(settings.esp32_port);
		$("#fontSize").val(editor.getFontSize());
		$("#settingsDialog").dialog("open");
	});
	
	// Create and handle the settings dialog.
	$("#settingsDialog").dialog({
		autoOpen: false,
		modal: true,
		buttons: [
			{
				text: "OK",
				click: function() {
					settings.esp32_host = $("#settingsHost").val();
					settings.esp32_port = $("#settingsPort").val();
					//editor.setFontSize($("#fontSize").val());
					editor.setOptions({
						fontSize: $("#fontSize").val + "pt"
					});
					$(this).dialog("close");
				}
			},
			{
				text: "Cancel",
				click: function() {
					$(this).dialog("close");
				}
			}
		] // Array of buttons
	}); // Settings dialog
}); // onReady.