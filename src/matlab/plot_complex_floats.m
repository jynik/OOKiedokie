% PLOT_COMPLEX_FLOATS   Display time and frequency domain plots for the signal
%                       contained in the specified file
%
% plot_complex_floats(filename)
function plot_complex_floats(filename)
    sig = load_complex_floats(filename);
    plot_sig(sig);
end
