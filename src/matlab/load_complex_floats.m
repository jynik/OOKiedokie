% LOAD_COMPLEX_FLOATS Load complex floats from a binary file
%
% [values] = LOAD_FLOATS(filename) loads float values from the specified file.
%
function [values] = load_complex_floats(filename)
    floats = load_floats(filename);

    len = length(floats);

    if mod(len, 2) ~= 0
        error(strcat(filename, ' contains incomplete samples.'));
    end

    values_i = floats(1:2:len-1);
    values_q = floats(2:2:len);

    values = values_i + 1j * values_q;
