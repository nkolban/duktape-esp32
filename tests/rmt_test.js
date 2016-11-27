console.log("Settings initially: " + JSON.stringify(RMT.getState()) + "\n");
RMT.txConfig(0, {
    gpio: 21,
    memBlocks: 1,
    idleLevel: false,
    clockDiv: 80
});
console.log("Settings after configuration: " + JSON.stringify(RMT.getState()) + "\n");