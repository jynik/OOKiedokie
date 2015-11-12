# p3l-nexa2012 #

This device is a *Radio Shack Indoor/Outdoor Wireless Temperature Sensor* that used
to be sold as catalog [#6300769](https://web.archive.org/web/20150107204137/http://www.radioshack.com/radioshack-indoor-outdoor-thermometer-with-digital-clock/6300769.html). 
Some of these are still floating around on Ebay.

The FCC ID of the transmitter is [P3L-NEXA2012](https://apps.fcc.gov/oetcf/eas/reports/ViewExhibitReport.cfm?mode=Exhibits&RequestTimeout=500&calledFromFrame=N&application_id=Tz%2FveDkjBQEXhzZelH9oMQ%3D%3D&fcc_id=P3L-NEXA2012).
It operates at 433 MHz, modulating data via [Pulse-Position Modulation](https://en.wikipedia.org/wiki/Pulse-position_modulation).

## Encoding and Format ##

There are three time-delays between pulses used by this device:

 * An extra long delay that's used as a "start of message" indicator
 * A long delay for a `1` bit
 * A short delay for a `0` bit

A message begins with a "start of message" condition, followed by a preamble.

Next, an unknown 8-bit field is transmitted. It changes when only the
temperature field changes, potentially indicating that it is a checksum field.
However, the base station appears to accept messages regardless of the value
of this field.

A 2-bit "channel" field follows. This is effectively an address field. However,
the manual and marketing materials describe these devices operating on 
"channels" 1 through 3, hence the perpetuated misnomer.

The temperature in degrees C is encoded next. This is scaled by a factor of 10
and is encoded as a two's complement value in 12 bits.

The final field is also currently unknown, but is suspected to be a sequence
number. Its initial value at power on does not appear to be deterministic. Its
value changes each time the temperature field changes. A few data sets showed
that this value was increasing monotonically with each change in temperature.
However, some other tests showed non-monotonic changes in this value. It is
currently unclear if pressing the *TX* button on the transmitter affects
the manner in which this field is updated. The base station shows whether
the temperature is increasing, decreasing, or remaining the same. Therefore,
an alternative hypothesis for this field is that it is the delta from the
previous sample.

## Operation ##

This temperature sensor appears to transmit its state once a minute. Each
transmission consists of the state message being repeated 7 times.

When the base station is powered on, it enters an active listening mode
in order to synchronize with the duty cycle of transmitters. This
is indicated by a little wireless beacon icon blinking. During this time,
the information from received messages is displayed immediately.

After some period of time, the base station leaves its active listening mode
and period "wakes" to listen for transmissions. It is believe that a wake-up
timer is set based upon when a channel's message was received during the initial
synchronization period, and is reset each time a message is successfully received.
This implies there are three separate wake-up timers with logically OR'd triggers,
given that the base station supports up to three transmitters.
