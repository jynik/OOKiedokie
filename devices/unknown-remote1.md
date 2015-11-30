# unknown-remote1 #

This device is an unknown remote control that operates at 433.92 MHz. Its
utility is unknown, as samples were taken from a previously broken devices
rescued out of a junk pile. No FCC ID is visible on the circuit boards or
casing pieces. Based upon the available buttons and their arrangements, it is
likely a universal remote for an A/V system or home entertainment center.

## Encoding and Format ##

This device utilizes [Pulse-Position Modulation](https://en.wikipedia.org/wiki/Pulse-position_modulation)
to encode data. A message begins with an initial long pulse followed by an
initial long off-time. A short or long delay is used between pulses to
encode 0 and 1 bits, respectively.

Each message contains three fields:
  * An 8-bit preamble. This value is constant across devices.
  * An 8-bit address or ID. This differs across devices, but appears to be configurable. This might be used to address different units within the associated system.
  * A 16-bit key code. This is separated into two bytes, `B1` and `B2` such that `B2 = ~B1`.

While a button is held in the pressed state, its associated message continually transmitted with approximately 42.5 milliseconds between each repetition.
