#Browser based JavaScript editor
When ESP32-Duktape starts we can point a browser at it and we will be presented with
a JavaScript editor in which we can enter and run JavaScript programs.  Here is what the browser
environment looks like:

![Editor](./images/editor.jpg)

At the top of the window we have a full function JavaScript editor with syntax hilighting and error
checking.  Beneath that we have the console output from our JavaScript environment.

The button bar at the bottom provides the following:

* Run - Run the script currently shown in the editor by sending it to the ESP32 for execution.
* Clear - Clear the console log output.
* Settings - Open a settings dialog to supply configuration settings.