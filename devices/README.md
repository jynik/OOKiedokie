# Device Specification Files #

The device specification files found in this directory describe
how data transmitted/received by a device is encoded. These files
include:

* A user-friendly device name (`name`)
* The number of bits in messages transmitted/received by the device (`num_bits`)
* A state machine that is used to encode/decode raw data (`states`)
* A description of how the data is divide up into fields (`fields`)
* How each message field should be interpreted and displayed (`formatter`)

This information is used to both decode and display received data,
as well as generate IQ samples to transmit messages to the target device.

These files are written in JSON, with the intent of being easy to read, write,
and parse. 

## Supported Devices ##

Below is a list of currently supported devices.

| Device File                         | Description                                                    |
| ----------------------------------- | -------------------------------------------------------------- |
| [p3l-nexa2012](p3l-nexa2012.md)     | RadioShack-branded 433.92 MHz wireless temperature sensor      |

## Format and Structure ##

Device specifications must begin with a top-level `device` object:

```
{ "device": { 
    .. 
}}
```

This provides a simple means for OOKiedokie to error out in an incorrect JSON
file is provided.

Next, the device specific tat
