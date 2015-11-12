% SAVE_COMPLEX_FLOATS  Save vector of complex values (floats) to the specified file
% 
% save_floats(filename, values)
%
function save_complex_floats(filename, values)
    values_i = real(values);
    values_q = imag(values);

    len = 2 * length(values_i);

    values_out(1:2:len-1) = values_i;
    values_out(2:2:len)   = values_q;

    save_floats(filename, values_out);
