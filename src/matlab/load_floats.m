% LOAD_FLOATS Load values from a binary file of floats
%
% [values] = LOAD_FLOATS(filename) loads float values from the specified file.
%
function [values] = load_floats(filename)
    values = [];
    f = fopen(filename, 'rb');

    if (f == -1)
        error(strcat('Unable to open ', filename));
    end

    values = fread(f, inf, 'float');

    fclose(f);
