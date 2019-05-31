# ebc-bike-master-controller
Documentation and code for the master controller unit for the Cry Baby electric bike control system. This project requires slave controllers to operate the bike itself. Slave controllers simply consist of factory controllers, with Arduino nanos in them (and a bit of other stuff).

## Setup
The pinouts and wiring are explained in the main file for the master controller.
In the future, refer to SETUP.md to build and debug the hardware.

## Operation
Once the unit is built and programmed, operation is fairly self-explanatory.
The important things to know about are the throttle button and the right brake lever.

Press the throttle button when moving to engage cruise control. Press any brake lever to disengage cruise control. Press the throttle button when stopped to enter the settings menu. While in the settings menu, press the throttle button to select an item and twist the throttle to go back.

Press the right brake lever three times in quick succession to access the "quick commands" menu, from which you can toggle the headlight, begin MPPT charging off a solar panel, and activate various other features.
