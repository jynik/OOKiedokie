# Device Specification Files #

The device specification files found in this directory describe
how data transmitted/received by a device is encoded. These files
include:

* A device name and brief description
* The number of bits in messages transmitted/received by the device
* A state machine that is used to encode/decode raw data
* A description of how the data is divided up into fields and how these fields should be displayed/parsed

This information is used to both decode and display received data,
as well as generate IQ samples to transmit messages to the target device.

These files are written in JSON, with the intent of being easy to read, write,
and parse.

# Supported Devices #

Below is a list of currently supported devices.

| Device File                         | Description                                                    |
| ----------------------------------- | -------------------------------------------------------------- |
| [p3l-nexa2012](p3l-nexa2012.md)     | Radio Shack-branded 433.92 MHz wireless temperature sensor     |

# Format and Structure #

Device specifications must begin with a top-level `device` object:

```
{ "device": {
    ...
}}
```

This provides a simple means for OOKiedokie to error out in an incorrect JSON
file is provided.

Next, the device specification must contain `name` and `description` fields:

```
    "name":         "example-device",
    "description":  "An example device that operates at 915.325 Mhz",
```

It is preferred that the `name` field matches the filename. The `description` field should contain
a brief description of the device and its purpose, if known.

A `num_bits` field must specify the size of messages used by this device, in bits.

```
    "num_bits": 32,
```

Internally, OOKiedokie allocates a data buffer of this size. When receiving, data bits
are aggregated into this data buffer as the state machine is traversed. When
transmitting, the desired message is pre-filled in this data buffer and the state
machine is traversed such that these data-bits are assumed, and the output samples are that
generated based upon those assumptions.

A `states` array must be provided, which defines the state machine used to both encode and decode
data for the associated device type:

```
    "states": [
        ...
    ]
```

Each entry in the `states` array defines conditions under which to transition
to the next state, and any actions to perform during the transition. This is
detailed in the following section.

Lastly, a `fields` array defines how the `num_bits` bits in device messages
are organized into fields. Each field entry describes the position of the
field bits in the message, and how the data is formatted in that field. The
`fields` array is described more in a later section.

## State Machine Definition ##

The `states` array defines the state machine that OOKiedokie shall use to
decode received messages, or encode data for transmission. It consists of an
array of objects with the following properties:

```
    {
        "name":         <String value. Required>,
        "duration_us":  <Integer value >= 0. Optional if triggers have durations>
        "timeout_us":   <Integer value >= 0. Optional>,
        "triggers":     <Array of triggers objects. Required>
    }
```

The state's `name` field should be a string that uniquely identifies the state.

The `duration_us` field describes the expected duration of a state, in
microseconds. When receiving input, the expected duration is used by OOKiedokie
to test if the input is meeting the specification outlined defined by the
state machine. OOKiedokie will reset the state machine if not. Note that a 15%
tolerance is applied to this value to account for some expected variation. When
used to craft samples to transmit, this value defines the exact length of this
state. This field may be set to 0 if is not applicable. (This is the default if
it is not specified.) However, if it is set to 0, it is expected that one of the
entries in the state's `triggers` array will define a duration. (This is
explained more below.)

The `timeout_us` field describes a timeout for the state, in microseconds. A
value of 0 implies, "no timeout." This field is optional and will default to 0
if not specified. The effect of a timeout will become clear shortly.

The `triggers` array defines the conditions under which a state is left,
which state is transitioned to next, and any actions that that should be
performed as the result of this transition. The entries in the `triggers`
array are evaluated in the order listed, allowing one to specify the
priority of the conditions that are checked. In general, one will want to
place a trigger with a "timeout" condition last in the list to that it is
activated only if no other triggers are satisfied during that sample-time.
(The state machine processes one sample at a time. We use "sample-time" to
refer to the context in which a particular sample is being processed.)


The first state in any OOKiedokie state machine **must** be a state named
"reset." When this state executes, it resets the aggregated data bits to zero
and resets various internal data structures. It is **highly** recommended that
this be configured to immediately transition to an "idle" state so that these
reset operations are only performed a single time.

```
        {
            "name":         "reset",

            "triggers": [
                {
                    "condition": "always",
                    "state":     "idle"
                }
            ]
        },

        {
            "name":         "idle",

            "triggers": [
                ...
            ]
        },
```

### Triggers ###

The `triggers` array should consist of one or more objects containing the
following fields:

```
        {
            "condition":    <Condition under which this trigger is activated.>
            "duration_us":  <Duration. Required if state did not have a duration.>
            "state":        <Name of next state>
            "action":       <Action to perform during state transition.>
        },
```

#### Condition ####

The `condition` field may be one of the following strings. If this condition
is satisfied, then trigger is "activated" and the state machine will progress
to the specified state.

| Condition String                    | Description                                              |
| ----------------------------------- | -------------------------------------------------------- |
| "always"                            | Causes transition to occur immediately.                  |
| "pulse\_start"                      | Signal has changed from "off" to "on."                   |
| "pulse\_end"                        | Signal has changed from "on" to "off."                   |
| "timeout"                           | The state's specified timeout has expired.               |
| "msg\_complete"                     | All bits (i.e., `num_bits` of data have been aggregated. |

The `always` condition is special, in the sense that it occurs immediately,
and does not consume a sample-time. This is useful for having the reset state
immediately transition to an "idle" state, and still allow that idle state
to "see" the current sample.

In most cases, the `msg_complete` condition should be given top priority by placing it first in
the `triggers` array, result in a transition to the `reset` state, and be
accompanied with the `output_data` action. OOKiedokie will error out if more
than `num_bits` are attempted to be aggregated.  This is shown below:

```
        {
            "condition":    "msg_complete",
            "state":        "reset",
            "action":       "output_data"
        },
```

#### Duration ####

If the state under which a trigger is placed has not defined a non-zero duration
(via its `duration_us` field), then the elapsed duration can be used as an additional
condition. The condition effectively becomes the logical AND of `condition`
and whether `duration_us` has elapsed in the current state.

For example, the following `bit_off_time` state is defined such that:

* The state has no `duration_us` field set. This implies that one or more trigger conditions will do so.
* The first trigger will be satisfied if:
 * The signal transitions from off to on
 * AND this occurs 2000 microseconds (+/- allowed tolerance) into this state.
* The second trigger will be satisfied if:
 * The signal transitions from off to on
 * AND this occurs 4000 microseconds (+/- allowed tolerance) into this state.
* The last trigger will occur if 6000 microseconds elapse and none of the previous triggers are activated.


```
        {
            "name":         "bit_off_time",
            "timeout_us":   6000,

            "triggers": [
                {
                    "condition":    "pulse_start",
                    "duration_us":  2000,
                    "state":        "bit_pulse",
                    "action":       "append_0"
                },

                {
                    "condition":    "pulse_start",
                    "duration_us":  4000,
                    "state":        "bit_pulse",
                    "action":       "append_1"
                },

                {
                    "condition":    "timeout",
                    "state":        "reset"
                }
            ]
        }
```

#### State ####

The trigger's `state` field defines the next state to transition to as a result
of the trigger being activated.

#### Action: RX ####

When receiving, a trigger's `action` field defines the action to perform as the result of its
activation. The available actions are listed below.

| Action String                       | Description                                     |
| ----------------------------------- | ----------------------------------------------- |
| "none"                              | Perform no action.                              |
| "append\_0"                         | Append a 0 to the buffer of aggregated bits.    |
| "append\_1"                         | Append a 1 to the buffer of aggregated bits.    |
| "output\_data"                      | Format and print aggregated data.               |

If no `action` field is specified, then "none" is assumed.

Consider the trigger in the previous example:


#### Action: TX ####

When transmitting, the `action` field is first treated as the condition that
activates the trigger. The `condition` and `duration_us` fields are then used to
define how the output samples are crafted. The `append_0` and `append_1` actions are used
to select a trigger, depending on the current data bit being evaluated. When
the `output_data` action is encountered, the generation of samples is
completed, and OOKiedokie will transmit the computed samples.

If no triggers contain `append_0`, `append_1`, or `output_data` actions, the
`condition` field is used to determine the next state to transition to while generating
samples.

Consider this trigger from the previous example:

```
                {
                    "condition":    "pulse_start",
                    "duration_us":  4000,
                    "state":        "bit_pulse",
                    "action":       "append_1"
                },
```

When traversing through the state machine, this trigger will be activated if the
current data bit that state machine is generating samples for is a 1. It will generate
4000 microseconds worth of the current output (presumably "off") and then change
its output state to "on" due to the `pulse_start` condition.

## Fields ##

The `fields` array is used to break the data buffer into individual fields and describe
the conversion between data bits and the field representation.

Each object in the `fields` array has the following form:

```
        {
            "name":         <Name string. Required>,
            "default":      <Default value, as a string. Required>,
            "start_bit":    <Start bit position value. Required.>,
            "end_bit":      <End bit position value. Required.>,
            "endianness":   <Bit endianness string. Required.>,
            "format":       <Field format string.>,
            "scaling":      <Value scaling factor. Optional.>,
            "offset":       <Value offset. Optional.>,
        },
```

The `name` field should be a unique and user-friendly string used to identify
the field. This is the name that will be printed when OOKiedokie prints the
contents of received fields. This is also the name used to provide field values
via OOKiedokie's `--tx-param` option.

The `default` field must contain the default value of the field, as a string.

The `start bit` and `end bit` fields define the position of the field's data
within the data buffer. The data buffer is laid out left to right, with
position 0 being left-most. When receiving, position 0 is where the first
received bit is placed. These should be defined as positive integer values such
that `start bit` is less than or equal to `end bit`. A single-bit field will have
both of these set to the same value.

The `endianness` field defines the bit-endianess of the data with the specified field.
This should be set to "big" or "little."

The `scaling` and `offset` fields are floating point values that may be used to
apply a scaling factor and offset to field values. When receiving, these are
applied as follows:


```
    field_value = data[start_bit:end_bit] * scaling + offset
```

When transmitting, these are applied as follows:

```
    data[start_bit:end_bit] = (field_value - offset) / scaling
```

If not specified, `scaling` defaults to 1.0 and `offset` defaults to 0.

The `format` field defines the data type and presentation of the field. Below
are the available options:

| Format                              | Description                                                      |
| ----------------------------------- | ---------------------------------------------------------------- |
| "hex"                               | Unsigned integer. Displayed as a hexadecimal value.              |
| "unsigned decimal"                  | Unsigned integer. Displaced as a decimal value.                  |
| "sign-magnitude"                    | Signed integer with MSB as sign-bit. Displayed as decimal value. |
| "two's complement"                  | Signed two's complement integer. Displayed as decimal value.     |
| "float"                             | Floating point value.                                            |

The "float" option is similar to the "two's complement" option, exception that its fractional portion
is retained when the scaling factor is applied. As such, the precision is a factor of the field width,
scaling, and offset.
