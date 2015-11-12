% PLOT_FILTER_STAGES Plot the response of repeated decimating filter stages
%
% [Fs_out, stages] = plot_stages(taps, Fs, decimation, num_stages)
%
% Arguments:
%   taps            Filter taps
%   decimation      Decimation for each stage. (Optional. Default is 1.)
%   num_stages      Number of repeated filter stages. (Optional. Default is 1.)
%   Fs              Sample rate (Optional. Default is 3e6.)
%
% Return value
%   Fs_out          Output sample rate

function [Fs_out] = plot_filter_stages(taps, decimation, num_stages, Fs)


if nargin < 2
    decimation = 1;
end

if nargin < 3
    num_stages = 1;
end

if nargin < 4
    Fs = 3e6; % Default rate used for bladeRF.
end

legend_labels = cell(num_stages + 1, 1);
stages = cell(num_stages, 1);
total_decimation = 1;
total_response = 1;

for n = [1:num_stages]
    legend_labels{n} = strcat('Stage #', int2str(n));
    stages{n} = upsample(taps, total_decimation);
    total_decimation = total_decimation * 2;
    total_response = conv(total_response, stages{n});
end

h = fvtool(stages{1}, 1, 'Fs', Fs);

for n = [2:num_stages]
    addfilter(h, stages{n}, 1);
end

legend_labels{length(legend_labels)} = 'Total response';
stages{length(stages)} = total_response;
addfilter(h, stages{length(stages)}, 1);

legend(legend_labels);
Fs_out = total_decimation;
