#!/bin/bash
nc 192.168.1.99 8001 < webserver.js 
nc 192.168.1.99 8001 < tests/test_ws.js

