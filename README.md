# ESP8266 DID NOT PROVIDE LOW ENOUGH POWER FOR TASK: ABANDOING.
New project: https://github.com/leon-v/MQTT-ESP8266-nRF24L01-Solar-Battery-I-O-Network

## Low power real time ESP8266 + MQTT.

#### The Environment:
So you understand this build environment, here is its description.

I am using Windows with the Linux bash shell. crosstoon-NG is installed in /opt/Espresif.
There is a script in /src/misc/getcompiler.sh to install dependencies and install required tools.

I am editing with Sublime text 3 and have a build script to simply execute gen_misc.sh in the bash shell.

You may well need to edit gen_misc.sh to suit your device. I have an ESP12E using the generic FTDI RS232 to USB module.


#### The Software:
This firmware uses the MQTT provided by Espressif (Which is excellent! I have cleaned it up a little though, i like more readable code)

The ADC gets sent as the 'test' topic on every interrupt.

MQTT client finally working! Well too.
The example code that Espressif supplied with the SDK is spot on. They did a good job of it.
I has trouble finding one online and was about to make my own in the same detail that this one had.

#### The Hardware.
2018-01-11: Have been working on a board for this project.
https://easyeda.com/minn0w/ESP8266_18650-7a7763891ea04e61a1e84073071af4b8 (PCB With 4015 + OpAmp - Surface (V2))
The idea is to build it so it can be assembled by OSHPark, almost all parts are part of their standard BOM.
Still prototyping it. But my last spin is yielding positive results and i will be working on the software again once the hardware is more complete.

I am using an ESP12E soldered onto a DIP breakout which has requisite pull down/ups.

The ESP8266 will go into Auto Light sleep, and needs to be woken via an external interrupt.
The interrupt is triggered by a discharging capacitor on GPIO14.
After a lot of testing with various ways to trigger the wake, including using my scope/sig-gen to test many different signals,
The best results by far were by using a capacitor on GPIO14.

I have a 10uF capacitor on GPIO14 to ground. With a 1Mohm across it to discharge the capacitor.
Wake time is about 10 seconds between.

The IC crashes easily if you do not have adequate decoupling.
Other resistor & capacitor values can be changed to get your timing right.
Capacitor charge delay can be adjusted in software to modify sleep time.
I found that a 5 second delay is OK. The delay should only affect the amount.
MQTT is brilliant at getting packets back and forth, so time could be increased significantly for extra battery life.


#### Testing Log...

The current battery life isn't looking great, but i only have 1 minute of data and i will be running this for the next few days to see how long it lasts.

First real world test sending MQTT data every 5 seonds showed a 100mV drop after 24 hours.
Given that LiPo batteries drop 1000mv drop per charge would make about 10 days battery life.
Not too bad, not too great.
Scope is measuring 38mW. Will check later if that matches


I currenly have the circuit running for days.
I am getting re-connection issues. The IC stays awake and tries to send packets, fills its buffer, then crashes.
I think i need to get MQTT ping working better.
I need to check Wifi connectivity

I have been runnign for 48 houes, and the 18650 dropped from 3900mV to 3700mV.
So 200mV drop. Assuming 1V drop of total battery life, i should get 10 days.

This is looking on par with what i need to get it to run from a solar panel
