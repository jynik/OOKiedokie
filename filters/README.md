# Filters

This directory contains FIR filter designs.

Files ending in a `.fda` extension are the design files associated with the
MATLAB *Filter Design & Analysis Tool*.

The `.mat` files are the exported filter taps. These are intended to be
loaded in MATLAB or Octave to view and analyze filter responses. 

The `.json` files are the resulting filter specifications used by OOKiedokie.

In general, these files are named based upon their pass-band, stop-band, and
decimation characteristics. Because different hardware will utilize different
sample rates, these are expressed in terms of the sample rate, `Fs`.

The preferred naming scheme is:

    <pass_band>_<stop_band>[_decimation].json

For example, for a filter file with a passband at Fs/32 and a stop band at
Fs/4, with no decimation (i.e., decimation by 1):

    Fs32_Fs4.json

And for the same filter with decimation by 2:

    Fs32_Fs4_dec2.json


## Filter file format ##

The FIR filters designed used by OOKiedokie are specified via JSON files.
Below is a filter design specification consisting of a single unity stage:

```
{ "filter": {
    "comment": "This filter is an example.",

    "stages": [
        {
            "decimation": 1
            "taps": [ 1.0 ]
        }
    ]
}}
```

The filter must begin with a top-level object named `filter`. This is enforced
by OOKiedokie and is used as a simple means to provide feedback if the wrong
.json file is provided.

The `comment` string is optional, but recommended. Use this to briefly describe
the design and its association with any design files (e.g., MATLAB fdatool files).

Next, the filter specification contains a `stages` array. This is required,
even if the filter only contains a single stage.

Each item in the `stages` array describes a filter stage object consisting of
a `decimation` factor and feed-forward `taps` array. The `decimation` entry is
optional, and defaults to 1 if not specified. This value must be an integer
greater than or equal to 1. The `taps` array is required, and should included
FIR filter taps specified as floating point values.

For a slightly larger example, see [fs128\_fs16\_dec4.json](fs128_fs16_dec4.json).
