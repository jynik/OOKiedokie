# OOKiedokie #

OOKiedokie is a tool intended to help SDR users interface with miscellaneous 
wireless devices utilizing [On-Off Keying](https://en.wikipedia.org/wiki/On-off_keying),
a very simple form of [Amplitude Shift Keying](https://en.wikipedia.org/wiki/Amplitude-shift_keying)
modulation.

The objectives of this project are:

* Provide a means to receive and decode OOK transmissions
* Provide the ability to craft and transmit messages to target devices
* Decouple the tool from user-created device definitions and filter designs via loadable JSON files
* Utilize a simple SDR abstraction to allow additional hardware support to be easily integrated
* Remain lightweight with minimal dependencies
* Provide a simple command-line interface for use in scripts

## Supported Hardware and File Formats ##
 * [Nuand bladeRF](https://www.nuand.com)
 * bladeRF binary SC16Q11 format (used by bladeRF-cli)

## Dependencies ##

**Required:**

 * [Jansson](http://www.digip.org/jansson/) (for parsing JSON files)

**Optional:**

 * [libbladeRF](https://github.com/nuand/bladeRF) (for [bladeRF](https://www.nuand.com) support)
 
## Documentation ##

*TO DO*: Create and link to documents regarding usage, examples, and development
