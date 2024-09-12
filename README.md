| Supported Targets | ESP32 | ESP32-C3 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- |

# Mesh Network Administration and Testing Example

This example shows how to use the API of the mesh network administration and testing module. You can send and receive messages to manage or test your mesh network and verify that events arrive at the desired node without interfering with the application's message buffer.

Features Demonstrated

- sending test messages

- test events reception

- messages application reception


Open project configuration menu (`idf.py menuconfig`) to configure the mesh network channel, router SSID, router password and mesh softAP settings.

When the mesh network is established and if this example is run, the root node starts sending random messages in bradcast mode. The other nodes receive the application messages and test events. All this is displayed on the console. 

