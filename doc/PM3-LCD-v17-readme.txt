PM3-LCD

* Introduction

PM3-LCD is a redesign of Jonathan Westhues's proxmark3 original RFID instrument.
At a glance, the improvements are:
 * 2 layer PCB for easier manufacturability
 * joystick user control (5 button)
 * Color 128x128 LCD
 * microSD for added user storage
 * Single cell LiPo battery powered for portability
 * on-board LiPo charger
 * simplified analog stage
 * Spartan 3 250K FPGA
 * PWM sound output

* Schematic description

Page 1 - power supply

Power from a mini USB is always available to the battery charger, based on a
LTC4053 standard reference design. The charge current is set via R106 to 370mA
and this is the maximum current available to charge the LiPo battery. This was
experimentally set so that the sum of this current and that drawn by the board
does not exceed the 500mA limit of the USB bus.

There are two indicator LEDs, orange D104 indicates the LiPo is being charged
when lit and red D105 indicating a fault condition when lit. Fault conditions
can occur for a number of reasons, ie the battery is too drained to be charged
(undervolt) or the battery voltage fails to rise when charged for a certain
amount of time. Refer to the datasheet for detailed information.

The battery connects to SV102 and it has a provision for an NTC thermal sensor.
If the sensor is not available you must solder bridge pins 2 and 3 of SV102
shorting LTC4053's NTC pin to ground.

Power from the mini USB port is optionally available to the rest of the board
depending on the setting of S1. The PCB layout was done such that togling S1
towards the USB connector selects USB power, while togling S1 away from the
USB connector selects battery power.

IC102 is a standard boost converter based on a LT3465A chip. It raises the
input voltage which can vary between 3 to 5 volts to about 7 volts required
by the LCD backlight. The output is current limited as set by R101. The CTL
input can enable (high) or disable (low) the chip output voltage and can be
pulse width modulated. See datasheet for info.

The remaining four chips IC103 to IC106 are standard LDOs rated at over 250mA
each. IC103 is always on and powers all the circuitry on page 3 (ARM micro, SD
card, LCD) while the other three LDOs can be switched on/off by the ARM and
power everything else (page 2 FPGA + page 4 analog section). By default they
are supposed to be turned off via R102 at power up.

Except for the special 1.2v and 2.5v FPGA supplies, and the antenna drivers
IC501 and IC502, the entire board runs at a 3v system voltage (instead of the
more common 3.3v). All the components are rated to run at this voltage and
this allows us to squeeze a bit more juice out of the battery while still being
within specs.

The antenna drivers IC501 and IC502 run from VBAT which depending on the S1
setting is either the full battery voltage or the USB voltage. This allows
them to utilise the highest possible voltage available to them when driving
the antenna.

Page 2 - FPGA

The FPGA was upgraded from a Spartan 2 in the original PM3 design to a newer
XC3S250E Spartan 3 family FPGA. In addition to having x5 the number of logic
elements as the one in the original designe, the Spartan 3 family is supported
by the latest Xilinx ISE tools unlike the Spartan 2 family which is becoming
obsolete.

The FPGA JTAG interface is no longer brought out to a connector as it's not
really required but is instead routed to test pads TP11-TP15. I had originally
tried to chain the ARM and FPGA together but this caused me some issues due to
the FPGA JTAG running at 2.5v and requiring extra components to chain to the
ARM which turned out to be too much effort given the tight board layout.

Most of the original design elements of the PM3 are still kept though the FPGA
pins are different. This means that the original FPGA source code can still be
used unchanged with the exception of the .ucf file which maps the FPGA logic
functions to physical pins.

********************************************************************************
*     The binary FPGA image compiled for the Spartan 2 is absolutely not       *
*     compatible with the Spartan 3 and the source code must be recompiled     *
********************************************************************************

The FPGA is initialized by the ARM through the FPGA_* pins DIN, DOUT, CLK, INITB
PROGB and DONE.

An external oscillator is implemented as before with IC203 and fed to two FPGA
global clock pins though I believe only one is needed. Additionally I believe any
clock frequency could be fed to these pins and the final required frequency be
synthesized internally to the FPGA via it's Digital Clock Modules (DCM).

The usual SSP and SPI buses are still present as before however a notable change
is the fact that the ARM no longer drives the analog multiplexer IC403 directly
but instead via the FPGA pins MUX_LO and MUX_HI. This was done in order to free
up more pins on the ARM.

The antenna drivers are driven by the pins PRW_LO, PWR_HI and PWR_OE1..4, so no
change there.

Finally a new ADC rated to run down to 3v is used in the form of the IC202
ADC08060 chip. This A/D can sample up to 70MSPS according to the datasheet.
Also despite the datasheet claiming a minimum sample rate of 20MSPS in practice
it seems to do a good job when driver in LF mode at 125KSPS.

Page 3 - Microcontroller

The micro used as IC302 is the same as before, either use a AT91SAM7S256 (256K
flash part) or a AT91SAM7S512 (512k flash part). Most people would tend to use
the larger flash part.

IC301 is the reset supervisor and holds the chip in reset until the supply
voltage rises to 3v. This might appear to be walking a fine line since the ARM
also runs at 3v, and to some extent it is borderline, but it relies on the fact
that the 3v supervisor trips at about 2.9v. This was found to work fine in
practice and might only be an issue when running off a fairly flat battery.
A workaround would be using 3.3v LDOs for IC103 and IC104 thus increasing the
overall system voltage to 3.3v

The ARM JTAG interface is brought to a 6 pin header with a Xilinx JTAG pinout
instead of the usual 2x10 ARM JTAG pin header. The main reason for that is that
the 2x10 pin header takes a lot of board real estate and the JTAG interface is
mostly only used once. For ongoing use of JTAG is not hard to make a small 2x10
pin to 6 pin adaptor just as if you were to connect to a FPGA JTAG.

The LCD output is controlled by PWM output PA0 and can be modulated to achieve
a range of LCD brightess values between just fully on or off.

An audio output is implemented on PA1 which conveniently is a PWM output. The
buzzer used in an inductive type (no piezzo) with a nominal 16 ohm resistance
and a single transistor is used as an amplifier. A range of frequencies can be
output by programming the PWM accordingly.

A lot of the ARM pins have been reassigned compared to the original design and
this would require a change to the config_gpio.h file, however no major source
code change is required as the peripheral SSP, SPI buses remain on their
original pins.

The old single button was replaced with a 5 position joystick (up, down, left,
right and select). The indicator LEDs were all removed as an LCD is now
available for status output. If required, a single "LED" status like indicator
can be implemented by driving the LCD backlight on or off prior to overall
system initialization or alternatively using various tones on the audio output.

The ARM I2C bus is not used in this design but the I2C pins PA3 and PA4 are
brough out to test pads. The SPI which originally only connected to the FPGA
has been expanded to also control the LCD and the microSD card.

Spare SPI chip select NCS3 is brought out to test pad TP9

The system voltage can be sampled through AD6 via the resistor divider.

Page 4 - Analog section

Some significant changes have occured to the analog section, most notably,
the raw signal path and associated components was removed as it didn't seem
to have been used or needed so far. All optional components were also removed.

The antenna coils are now ground referenced.

Since only one analog channel is ever routed through the IC403 mux, the op-amps
for each channel were combined into one at the muxs output elliminating another
chip. The analog section therefore only needs a single chip dual op-amp, in the
form of IC401 AD8052. If this part is not available an AD8062 can be substituted
as it is pin and function compatible. Half of IC401, is used to generate the
VBIAS = 1.5v the other half is used as an op-amp with approximately x2 gain.

Finally the TLV3502 comparator chip IC402 with a SOT23-8 footprint was
designed with an alternative part option in the form of IC404 LT1720 with SOIC8
footprint. This is to cope with occasional shortages experienced with the
TVL3502 part. Only one of these chips needs to be fitted to the board.
While the TLV3502 and LT1720 are functionally equivalent, there is a minor pin
discrepancy in that pins 3 and 4 on the LT1720 are swapped compared to the
TLV3502. To accomodate this resistors R417 through R420 accomplish the pin swap

When using a  LT1720 DO NOT PLACE resistors R417 and R418
When using a TLV3502 DO NOT PLACE resistors R419 and R420

Placing all four resistors will disable the CROSS_HI comparator, by grounding
both inputs but will not damage it.

Page 5 - Antenna drivers

These are the antenna drivers, using 74AC244 which can operate from a range of
supply voltages. In this instance, depending on switch S1, they are powered
from either the 5v USB bus or the LiPo battery 3v to 4v range. The design of
the antenna drivers hasn't otherwise changed from the original.

* Printed Circuit Board

Note that IC402 and IC404 have overlapping footprints as they are mutually
exclusive, ie only fit one or the other. See "Page 4 - Analog section" about
fitting these chips and associated resistors R417 through R420.

Due to the tight component placement, some component values cannot be placed
immediately next to the component they represent. They are therefore placed
at the nearest possible location practicable and in the same exact order
as the components they represent.

When the above isn't at all possible, a straight line is drawn next to the
components and routed to a place on the board where the labels can be found.
Again the labels are in the same exact order as the components they represent.
When assembling the board, if you're ever in doubt, use the component value
designator which is always placed inside the component boundary.

The board can and has been assembled by hand soldering with no issues. The only
potential issue is the microSD. It has a metal cover preventing access of the
soldering iron tip to the pins underneath. The way *I* handled it is to
carefully remove the metal cover by popping the two clips each side. Be very
careful if you follow in my footsteps as the card ejector mechanism is spring
loaded. You need to gently lift the metal cover and catch the spring and
associated tiny parts forming that mechanism without losing them and figuring
out how to place them back so the card ejector mechanism still operates. Once
the metal cover is lifted, the base can be soldered via its exposed 8 pins
then the ejector mechanism and spring replaced and the top metal cover pushed
back so the 4 side clips engage fully. Then the 4 pads of the metal cover can
be soldered to the board.

IC101 has what seems like a large via underneath it. This served as a thermal
pad since this IC is the LiPo charger. Place enough solder into the via to
completely fill it level with the board *after* soldering IC101. This solder
will flow through the via and connect with the thermal pad underneath IC101.
This design choice was done with manual soldering in mind and should be
replaced with the usual (no via) thermal pad for production in a reflow oven.

The battery connector SV102 is optional, if you prefer, you can permanently
solder a battery to the two large nearby vias labeled GND and +VBAT however
once you've done that the battery can't be easily detached any more.

For a low profile board use tantalum caps for C107 and C550 as per the BOM,
but taller and cheaper aluminium electrolytic caps can be used as well.

There are 4 components placed on the other side of the board not shown in
the attached files. The joystick is obvious by the footprint and can only be
fitted one way (4 pins on one side and 3 on the other).

The LCD connector is also obvious by the footprint and has no orientation so
it can be fitted either way and still work.

This leaves the two LEDs, cathodes (negative) towards the edge of the board,
yellow LED closest to the screw hole, then the red LED next to it.
