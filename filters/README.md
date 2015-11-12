# Filters

This directory contains filter designs.

Files ending in a `.fda` extension are the design files associated with the
MATLAB *Filter Design & Analysis Tool*.

The `.mat` files are the exported filter taps. These are intended to be
used in conjunction with the plot\_stages.m function to view the response
of repeated decimation stages created with these taps. 

The `.json` files are the resulting filter specifications used by OOKiedokie.

In general, these files are named based upon their pass-band, stop-band, and
decimation characteristics. Because different hardware will utilize different
sample rates, these are expressed in terms of the sample rate, `Fs`.

The preferred naming scheme is:
    
    <pass_band>_<stop_band>[_decimation].<extension>

For example, for a filter file with a passband at Fs/32 and a stop band at
Fs/8, with no decimation (decimation by1):

    Fs32_Fs8.json

And for the same filter with decimation by 2:

    Fs32_Fs8_dec2.json
