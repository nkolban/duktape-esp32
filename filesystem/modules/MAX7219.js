/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
// https://github.com/victorporof/MAX7219.js

var SPI = require("spi");

/**
 * MAX7219 abstraction.
 * Please read the datasheet: https://www.adafruit.com/datasheets/MAX7219.pdf
 *
 * Example use:
 *  var disp = new MAX7219(spi);
 *  disp.setDecodeNone();
 *  disp.setScanLimit(8);
 *  disp.startup();
 *  disp.setDigitSegments(0, [0, 0, 1, 1, 0, 1, 1, 1]);
 *  disp.setDigitSegments(1, [0, 1, 0, 0, 1, 1, 1, 1]);
 *  disp.setDigitSegments(2, [0, 0, 0, 0, 1, 1, 1, 0]);
 *  disp.setDigitSegments(3, [0, 1, 1, 0, 0, 1, 1, 1]);
 *
 * Alternate use:
 *  var disp = new MAX7219(spi);
 *  disp.setDecodeAll();
 *  disp.setScanLimit(8);
 *  disp.startup();
 *  disp.setDigitSymbol(0, "H");
 *  disp.setDigitSymbol(1, "E");
 *  disp.setDigitSymbol(2, "L");
 *  disp.setDigitSymbol(3, "P");
 *
 * @param string device
 *        The SPI device on which the controller is wired.
 *        For example, "/dev/spidev1.0".
 * @param number count [optional]
 *        The total number of controllers when daisy-chained. Defaults to 1.
 * @param function callback [optional]
 *        Invoked once the connection to the SPI device is finished.
 */
function MAX7219(spiDevice) {
  this._activeController = 0;
  this._totalControllers = 1;
  this._buffer = new Buffer(this._totalControllers * 2);

  this._spi = spiDevice;
}

/**
 * Controller registers, as specified in the datasheet.
 * Don't modify this.
 */
MAX7219._Registers = {
  NoOp: 0x00,
  Digit0: 0x01,
  Digit1: 0x02,
  Digit2: 0x03,
  Digit3: 0x04,
  Digit4: 0x05,
  Digit5: 0x06,
  Digit6: 0x07,
  Digit7: 0x08,
  DecodeMode: 0x09,
  Intensity: 0x0a,
  ScanLimit: 0x0b,
  Shutdown: 0x0c,
  DisplayTest: 0x0f
};

/**
 * Controller BCD code font, as specified in the datasheet.
 * Don't modify this.
 */
MAX7219._Font = {
  "0": 0x00,
  "1": 0x01,
  "2": 0x02,
  "3": 0x03,
  "4": 0x04,
  "5": 0x05,
  "6": 0x06,
  "7": 0x07,
  "8": 0x08,
  "9": 0x09,
  "-": 0x0a,
  "E": 0x0b,
  "H": 0x0c,
  "L": 0x0d,
  "P": 0x0e,
  " ": 0x0f
};

MAX7219.prototype = {
  /**
   * When daisy-chaining MAX7219s, specifies which chip is currently controlled.
   *
   * @param number index
   *        The index of the chip to control.
   */
  setActiveController: function(index) {
    if (index < 0 || index >= this._totalControllers) {
      throw "Controller index is out of bounds";
    }
    this._activeController = index;
  },

  /**
   * Returns which chip is currently controlled.
   */
  getActiveController: function() {
    return this._activeController;
  },

  /**
   * Sets this controller in normal operation mode.
   *
   * On initial power-up, all control registers are reset, the display is
   * blanked, and the MAX7219 enters shutdown mode. This method sets
   * the controller back in normal operation mode.
   *
   * @param function callback [optional]
   *        Invoked once the write to the SPI device finishes.
   */
  startup: function(callback) {
    this._shiftOut(MAX7219._Registers.Shutdown, 0x01, callback);
  },

  /**
   * Sets this controller in shutdown mode.
   *
   * When the MAX7219 is in shutdown mode, the scan oscillator is halted, all
   * segment current sources are pulled to ground, and the display is blanked.
   *
   * @param function callback [optional]
   *        Invoked once the write to the SPI device finishes.
   */
  shutdown: function(callback) {
    this._shiftOut(MAX7219._Registers.Shutdown, 0x00, callback);
  },

  /**
   * Sets this controller in display-test mode.
   *
   * Display-test mode turns all LEDs on by overriding, but not altering, all
   * controls and digit registers (including the shutdown register).
   *
   * @param function callback [optional]
   *        Invoked once the write to the SPI device finishes.
   */
  startDisplayTest: function(callback) {
    this._shiftOut(MAX7219._Registers.DisplayTest, 0x01, callback);
  },

  /**
   * Sets this controller back into the previous operation mode.
   *
   * @param function callback [optional]
   *        Invoked once the write to the SPI device finishes.
   */
  stopDisplayTest: function(callback) {
    this._shiftOut(MAX7219._Registers.DisplayTest, 0x00, callback);
  },

  /**
   * Sets this controller's decode mode, specifying how the segments controlled
   * by the MAX7219 are set on/off.
   *
   * When no-decode is selected, data bits correspond to the segments directly.
   * When decode mode is selected, certain symbols (only 0-9, E, H, L, P, and -)
   * are encoded in a specific way. This is useful for BCD 7 segment displays.
   *
   * @param array modes
   *        An array of decode/no-decode modes for each digit.
   *        E.g., to set decode mode for digits 0–3 and no-decode for 4–7,
   *        modes would be [1,1,1,1,0,0,0,0].
   *
   * @param function callback [optional]
   *        Invoked once the write to the SPI device finishes.
   */
  setDecodeMode: function(modes, callback) {
    if (modes.length != 8) {
      throw "Invalid decode mode array";
    }
    this._decodeModes = modes;
    this._shiftOut(MAX7219._Registers.DecodeMode, this.encodeByte(modes), callback);
  },

  /**
   * Shortcut for specifying that all digits are in no-decode mode.
   *
   * @param function callback [optional]
   *        Invoked once the write to the SPI device finishes.
   */
  setDecodeNone: function(callback) {
    this.setDecodeMode([0,0,0,0,0,0,0,0], callback);
  },

  /**
   * Shortcut for specifying that all digits are in decode mode.
   *
   * @param function callback [optional]
   *        Invoked once the write to the SPI device finishes.
   */
  setDecodeAll: function(callback) {
    this.setDecodeMode([1,1,1,1,1,1,1,1], callback);
  },

  /**
   * Sets each segment in a digit on/off.
   * For this to work properly, the digit should be in no-decode mode.
   *
   * The segments are identified as follows:
   *    _a_
   *  f|   |b
   *   |_g_|
   *   |   |
   *  e|___|c  dp (decimal point)
   *     d    *
   *
   * @param number n
   *        The digit number, from 0 up to and including 7.
   * @param array segments
   *        A list specifying whether segments are on and off.
   *        E.g., to specify dp, c, d, e and g on, and a, b, f off,
   *        segments would be [1, 0, 0, 1, 1, 1, 0, 1], corresponding
   *        to the structure [dp, a, b, c, d, e, f, g].
   *
   * @param function callback [optional]
   *        Invoked once the write to the SPI device finishes.
   */
  setDigitSegments: function(n, segments, callback) {
    if (n < 0 || n > 7) {
      throw "Invalid digit number";
    }
    if (segments.length != 8) {
      throw "Invalid segments array";
    }
    this.setDigitSegmentsByte(n, this.encodeByte(segments), callback);
  },

  /**
   * Same as setDigitSegments, but it takes a byte instead of an array of bits.
   *
   * @param function callback [optional]
   *        Invoked once the write to the SPI device finishes.
   */
  setDigitSegmentsByte: function(n, byte, callback) {
    this._shiftOut(MAX7219._Registers["Digit" + n], byte, callback);
  },

  /**
   * Sets the symbol displayed in a digit.
   * For this to work properly, the digit should be in decode mode.
   *
   * @param number n
   *        The digit number, from 0 up to and including 7.
   * @param string symbol
   *        The symbol do display: "0".."9", "E", "H", "L", "P", "-" or " ".
   * @param boolean dp
   *        Specifies if the decimal point should be on or off.
   *
   * @param function callback [optional]
   *        Invoked once the write to the SPI device finishes.
   */
  setDigitSymbol: function(n, symbol, dp, callback) {
    if (n < 0 || n > 7) {
      throw "Invalid digit number";
    }
    if (!(symbol in MAX7219._Font)) {
      throw "Invalid symbol string";
    }
    var byte = MAX7219._Font[symbol] | (dp ? (1 << 7) : 0);
    this._shiftOut(MAX7219._Registers["Digit" + n], byte, callback);
  },

  /**
   * Sets all segments for all digits off.
   *
   * Shortcut for manually calling setDigitSegments or setDigitSymbol
   * with the appropriate params. If a decode mode wasn't specifically set
   * beforehand, no-decode mode is assumed.
   *
   * @param function callback [optional]
   *        Invoked once the write to the SPI device finishes.
   */
  clearDisplay: function(callback) {
    if (!this._decodeModes) {
      this.setDecodeNone();
    }

    for (var i = 0; i < this._decodeModes.length; i++) {
      var mode = this._decodeModes[i];
      if (mode == 0) {
        this.setDigitSegmentsByte(i, 0x00, callback);
      } else {
        this.setDigitSymbol(i, " ", false, callback);
      }
    }
  },

  /**
   * Sets digital control of display brightness.
   *
   * @param number brightness
   *        The brightness from 0 (dimmest) up to and including 15 (brightest).
   *
   * @param function callback [optional]
   *        Invoked once the write to the SPI device finishes.
   */
  setDisplayIntensity: function(brightness, callback) {
    if (brightness < 0 || brightness > 15) {
      throw "Invalid brightness number";
    }
    this._shiftOut(MAX7219._Registers.Intensity, brightness, callback);
  },

  /**
   * Sets how many digits are displayed, from 1 digit to 8 digits.
   *
   * @param number limit
   *        The number of digits displayed, counting from first to last.
   *        E.g., to display only the first digit, limit would be 1.
   *        E.g., to display only digits 0, 1 and 2, limit would be 3.
   *
   * @param function callback [optional]
   *        Invoked once the write to the SPI device finishes.
   */
  setScanLimit: function(limit, callback) {
    if (limit < 1 || limit > 8) {
      throw "Invalid scan limit number";
    }
    this._shiftOut(MAX7219._Registers.ScanLimit, limit - 1, callback);
  },
  
  setNumber: function(num) {
		num = Math.floor(num) % 100000000;
		var s = num.toString();
		var d = s.length - 1;
		var i;
		for (i=0; i<8; i++) {
			if (i >= s.length) {
				this.setDigitSymbol(i, ' ');
			} else {
				this.setDigitSymbol(i, s.charAt(d-i));
			}
		}
	},

  /**
   * Utility function. Returns a byte having the specified bits.
   *
   * @param array bits
   *        An array of 7 bits.
   * @return number
   *         The corresponding byte.
   *         E.g., [1,1,0,1,0,1,0,1] returns 213, or "11010101" in binary.
   */
  encodeByte: function(bits) {
    return bits[0] +
          (bits[1] << 1) +
          (bits[2] << 2) +
          (bits[3] << 3) +
          (bits[4] << 4) +
          (bits[5] << 5) +
          (bits[6] << 6) +
          (bits[7] << 7);
  },

  /**
   * Shifts two bytes to the SPI device.
   *
   * @param number firstByte
   *        The first byte, as a number.
   * @param number secondByte
   *        The second byte, as a number.
   * @param function callback [optional]
   *        Invoked once the write to the SPI device finishes.
   */
  _shiftOut: function(firstByte, secondByte, callback) {
    if (!this._spi) {
      throw "SPI device not initialized";
    }

    for (var i = 0; i < this._buffer.length; i += 2) {
      this._buffer[i] = MAX7219._Registers.NoOp;
      this._buffer[i + 1] = 0x00;
    }

    var offset = this._activeController * 2;
    this._buffer[offset] = firstByte;
    this._buffer[offset + 1] = secondByte;

    this._spi.transmit(this._buffer);
    if (callback) {
   	 callback();
    }
  }
};

module.exports = MAX7219;