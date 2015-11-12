% PLOT_SIG  Display time and frequency domain plots for the provided complex signal.
%
% plot_sig(sig)
%
function plot_sig(sig)
    subplot(2, 2, 1);
    plot(real(sig));
    title('I Channel');
    xlabel('Sample #');
    ylabel('Value');
    
    subplot(2, 2, 3);
    plot(imag(sig));
    title('Q Channel');
    xlabel('Sample #');
    ylabel('Value');

    subplot(2, 2, 2);
    plot(imag(sig));
    title('Magnitude');
    xlabel('Sample #');
    ylabel('Value');

    subplot(2, 2, 4);
    pwelch(sig, 'power');
end 
