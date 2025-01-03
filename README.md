Simple code utilizing Arduino framework made in VS Code with Platform.io to implement relay switch based on M5Stack Atom 

**Why?** 
1. A quick search on github provided no such example.
2. I had some christmas decoration to control and wall socket with USB charger.
3. M5Atom has a small 5 x 5 led matrix suitable for some basic animation.

**How to use:**
1. Configure VS Code with Platform.io.
2. Clone repo and use "open folder" option in VS Code.
3. Edit config.h: add your wifi ssid, password and mqtt connection details.
4. Attach to USB port an M5Atom with Mini 3A Relay Unit and click upload.
5. Each time button/screen is clicked or on/off message is received on mqtt channel m5/custom_id/relay/0/command the relay toggles and sends message to mqtt channel m5/custom_id/relay/0 with on/off payload
6. Hack the relay unit by exposing GND and 5V (e.g. 5V trough COM to NC/NO contacts) and conenct your load between NO and GND. In my case the load consists of 4 LED "tee" lamps connected via 30/100 Ohm voltage divider between NO and GND. 

