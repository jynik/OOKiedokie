% Generate sample files for testing the FIR filter implementation
function gen_samples()
    
    % Short file, impulse with value in I channel
    sig = zeros(1, 100);
    sig(50) = 1+0j;

    save_complex_floats('short_impulse_I.cfloat', sig);

    % Short file, impulse with value in Q channel
    sig(50) = 0+1j;
    save_complex_floats('short_impulse_Q.cfloat', sig);

    % Impulse with value in I channel
    sig = zeros(1, 1e6);
    sig(1000) = 1+0j;
    save_complex_floats('impulse.cfloat', sig);

    % Normalized time for 1 million samples
    t = [1:1e6];

    % Tone at Fs/4
    omega = 2 * pi / 4.0;
    sig1 = exp(1j * omega * t);
    save_complex_floats('tone_fs4.cfloat', sig1);

    figure;
    pwelch(sig1, 'power');
    title('Fs / 4');

    % Tone at Fs/32
    omega = 2 * pi / 32.0;
    sig2 = exp(1j * omega * t);
    save_complex_floats('tone_fs32.cfloat', sig2);

    figure;
    pwelch(sig2, 'power');
    title('Fs / 32');

    % Two tones: Fs/4 and Fs/32
    sig3 = (sig1 + sig2) / 2.0;
    save_complex_floats('twotone_fs4_fs32.cfloat', sig3);

    figure;
    pwelch(sig3, 'power');
    title('Fs/32 & Fs/4');
end
