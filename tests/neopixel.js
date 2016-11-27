function addHigh(data) {
    data.push({level: true, duration: 13});
    data.push({level: false, duration: 4});
}

function addLow(data) {
    data.push({level: true, duration: 4});
    data.push({level: false, duration: 8}); 
}

function addByte(neoPixelData, val) {
    console.log(val + " - ");
    for (var i=0; i<8; i++) {
        if ((val & 0x80) !== 0) {
            console.log("1");
            addHigh(neoPixelData);
        } else {
            console.log("0");
            addLow(neoPixelData);
        }
        val = val << 1;
    } // End of loop
    console.log("\n");
}

console.log("Settings initially: " + JSON.stringify(RMT.getState()) + "\n");
RMT.txConfig(0, {
    gpio: 21,
    memBlocks: 1,
    idleLevel: false,
    clockDiv: 8
});
console.log("Settings after configuration: " + JSON.stringify(RMT.getState()) + "\n");

var neoPixelData = [];
addByte(neoPixelData, 0x00);
addByte(neoPixelData, 0x00);
addByte(neoPixelData, 0xff);
RMT.write(0, neoPixelData);