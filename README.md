# MQTT OpenHAB Controller

This is a work in progress. The ideas is to have a touch screen controller that fits in a double width switch plate. The controller will ask MQTT for a list of lights and then subscribe to updates (using the MQTT event bus in openHAB). OpenHAB rules are required to support this, a simple contact with a rule that publishes to an MQTT topic will do. 
