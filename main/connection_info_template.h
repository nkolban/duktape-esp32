/*
 * connection_info.h
 *
 *  Created on: Nov 15, 2016
 *      Author: kolban
 */

#ifndef MAIN_CONNECTION_INFO_H_
#define MAIN_CONNECTION_INFO_H_
// The IP address that we want our device to have.
#define DEVICE_IP          "192.168.5.2"

// The Gateway address where we wish to send packets.
// This will commonly be our access point.
#define DEVICE_GW          "192.168.5.1"

// The netmask specification.
#define DEVICE_NETMASK     "255.255.255.0"

// The identity of the access point to which we wish to connect.
#define AP_TARGET_SSID     "RASPI3"

// The password we need to supply to the access point for authorization.
#define AP_TARGET_PASSWORD "password"


#endif /* MAIN_CONNECTION_INFO_H_ */
