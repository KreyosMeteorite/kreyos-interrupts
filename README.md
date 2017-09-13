# kreyos-interrupts
Examples of using interrupts in Kreyos Meteor

This is how to set up and use interrupts for each of the four buttons,
as well as pin P1.2, used for receiving serial communication.

The buttons toggle the backlight on and off, and the P1.2 interrupt
copies P1.2 state, for rising and falling edges, to P1.1, thereby
acting as a serial port echo, working up to about 19200 bps.
