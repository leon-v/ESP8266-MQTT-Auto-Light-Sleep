## Low power real time ESP8266 + MQTT.

### THIS PROJEST IS STILL COMING OUT OF PROOF-OF-CONCEPT


#### The Environment:
So you understand this build environment, here is its description.

I am using Windows with the Linux bash shell. crosstoon-NG is installed in /opt/Espresif.
There is a script in /src/misc/getcompiler.sh to install dependencies and install required tools.

I am editing with Sublime text 3 and have a build script to simply execute gen_misc.sh in the bash shell.

You may well need to edit gen_misc.sh to suit your device. I have an ESP12E using the generic FTDI RS232 to USB module.


#### The Software:
This firmware uses the MQTT provided by Espressif

The ADC gets sent as the 'test' topic on ever interrupt.

MQTT client working! Well too.
The example code that Espressif supplied with the SDK is spot on. They did a good job of it.
I has trouble finding one online and was about to make my own in the same detail that this one had.

The current battery life isn't looking great, but i only have 1 minute of data and i will be running this for the next few days to see how long it lasts.

#### The Hardware.
I am using an ESP12E soldered onto a DIP breakout which has requisite pull down/ups.

The ESP8266 will go into Auto Light sleep, and needs to be woken via an external interrupt.
The interrupt is triggered by a discharging capacitor on GPIO14.
After a lot of testing with various ways to trigger the wake, including using my scope/sig-gen to test many different signals,
The best results by far were by using a capacitor on GPIO14.
You can't just put the cap directly on GPIO14, there must be a resistor in between. RC filter style.

I am using a 10Kohm resistor on GPIO14, through a 4.7uF capacitor to ground. With a 1Mohm across it to discharge the capacitor.
^ this needs revising. Likely wrong

The IC will crash if it gets a current surge on GPIO14, so don't go below 10K attached to it.
The IC will crash due to many many finer details that i should have ironed out.
Other resistor & capacitor values can be changed to get your timing right.
Charge delay can be adjusted in software to modify sleep time.
I found that a 5 second delay is OK. The delay should only affect the amount.
MQTT is brilliant at getting packets back and forth, so time could be increased significantly for extra battery life.