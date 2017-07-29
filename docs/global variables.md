# Global variables
ESP32-Duktape needs to maintain state.  Since the ESP32-Duktape framework is primarily written
in JavaScript, this state is maintained in global variables.  We have tried to choose the names
of these variables such that they are unlikely to occur in user written code.  Later on we will
work to hide these variables so that they can't be touched.

Here is a list and quick description of each one.

## _dukf_callbackStash
To be written.

## _isr_gpio
An object that contains callbacks to be invoked when an ISR is detected on a GPIO.  This object
is keyed by pin number each value is of the form:
```
{
   callback: function(pin) {...}
}
```

## _sockets
To be written.


## _timers
Holds the state of all the active timers in the environment.  A timer is created with calls to
`setInterval()` or `setTimeout()`.  The structure of the object is:

```
{
   timerEntries: [
      callback: // The callback function to be invoked when the timer fires.
      fire:     // The time when the timer will next fire (msecs).
      id:       // The ID of the timer.
      interval: // The interval duration when the timer will fire again after the next fire.
                // This will be 0 for one shot (setTimeout()) timers.
   ]
}
```


# Non Volatile Storage
This project also uses some Non Volatile Storage areas:

* esp32duktape -> start - string - Name of program to load at start.
* esp32duktape -> useSerial - uint8_t - 1 if we are going to use serial processor (uart_processor.js).