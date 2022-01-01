# Onxy-Resin
A project that intends to control power going to a 3D resin printer (Currently tested with an Elegoo Saturn) and chamber heating including feedback from various sensors.

# Intentions
This project started as a learning excerise for me and to get into IoT schtuff. 

A few of my goals are:
* Remote control of powering on or off the printer
* Remote control of chamber heater
* Live readout of resin and ambient air temperature 
* Control via a Web Interface

## How is it going so far?
Lot's of teething issues but:
* We can read resin and ambient air temperature using a contactless temperature sensor
* Can detect when a print has finished via a photo interrupter sensor but it's a little flakey and will likely remove it in favour of just having a separate web camera feed
* I've personally hit the limits of the ATMega Arduino controller i'm using and ready to move on to something more powerful that can handle camera feeds directly.
* for the web server, the project relies on a ethernet shield with an SD card that serves HTML from it. It's ok but would like something better. Will do for now.

# What's next?
As I've mentioned above, it needs a little more processing power which means later we will have to move onto a more powerful microcontroller however, it's currently great for switching on/off the printer & heater and getting some temperature data.

