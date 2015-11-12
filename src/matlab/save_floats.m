% SAVE_FLOATS  Save vector of floats to the specified file
% 
% save_floats(filename, values)
%
function save_floats(filename, values)
    f = fopen(filename, 'wb');

    if (f == -1)
        error(strcat('Unable to open ', filename))
    end

    values = fwrite(f, values, 'float');

    fclose(f);
