# remaplut
Remap bits in a lookup table.

Takes an input text file with a lookup table and remaps the bits in it, producing a new lookup table as output. Supports 8, 16, 32 and 64 bit numbers.

See the tests directory for examples. Basically lines starting with ## set the mapping, and lines starting with 0b are values. All other lines are ignored so you can insert comments etc. in your source file.

This was originally written to help make wiring up displays easier. Typically there is a character lookup table for the segments/pixels in the display. By remapping the bits each segment/pixel can be wired in the most convinient way, or as needs dictate with things like different voltage/current LEDs.
