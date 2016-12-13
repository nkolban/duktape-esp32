log("About to gc");
log("Heap before: " + ESP32.getState().heapSize);
ESP32.gc();
log("Heap after: " + ESP32.getState().heapSize);