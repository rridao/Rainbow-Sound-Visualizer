SinOsc k => dac;

11 => int num_notes;
100.0 => float tempo; 

int song[2][num_notes];

[ 26, 29, 34, 36, 38, 38, 27, 29, 34, 34, 36 ] @=> song[0];
[  4,  4,  4,  2,  2,  8,  4,  4,  4,  4,  6 ] @=> song[1];

for(0 => int i; i < num_notes; i++) {
    Std.mtof(song[0][i]+35) => k.freq;
    song[1][i]*15/tempo => float len;
    len::second => now;
}