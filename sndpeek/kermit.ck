SinOsc kermit => Gain gK => dac;
SinOsc banjo1 => Gain gB => dac;
SinOsc banjo2 => gB => dac;

5.0 => gK.gain;
0.3 => gB.gain;

130::ms => dur tempo;
 
// Initialize pitch //
0 => banjo1.freq;
0 => banjo2.freq;
0 => kermit.freq;

60 => int bLength; //Length of banjo start

spork ~ banjo(banjo1, banjo2, tempo);
spork ~ singer(kermit, tempo, bLength);
bLength*tempo => now;
0.2 => gB.gain;


// Banjos //

fun void banjo(Osc @ b1, Osc @ b2, dur t) {
    9 => int numIntro; // length of song
    int intro[3][numIntro]; 
    // intro[0]: MIDI notes for banjo 1
    // intro[1]: MIDI notes for banjo 2
    // intro[2]: Num of 16th notes
    
    [ 69, 76, 81, 76, 81, 76, 69, 78, 81 ] @=> intro[0];
    [  0,  0, 85,  0, 85,  0,  0,  0, 86 ] @=> intro[1];
    [  3,  1,  3,  1,  3,  1,  3,  1,  8 ] @=> intro[2];
    
    
    for(0 => int i; i < numIntro; i++) {
        Std.mtof(intro[0][i]-12) => b1.freq; // Assign freq
        Std.mtof(intro[1][i]-12) => b2.freq; // Assign freq
        t*intro[2][i] => now;             // Assign time
    }
    //4*t => now;
    0 => b1.freq; 0 => b2.freq;
    12*t => now;
    for(0 => int i; i < numIntro; i++) {
        Std.mtof(intro[0][i]-12) => b1.freq; // Assign freq
        Std.mtof(intro[1][i]-12) => b2.freq; // Assign freq
        t*intro[2][i] => now;             // Assign time
    }
    
    14 => int numSong;
    int song[3][numSong];
    
    for(0 => int i; i < numSong; i++) {
        Std.mtof(song[0][i]-12) => b1.freq; // Assign freq
        Std.mtof(song[1][i]-12) => b2.freq; // Assign freq
        t*song[2][i] => now;             // Assign time
    }
    0 => b1.freq;
    0 => b2.freq;
    
}


// Melody (Kermit the Frog here) //
fun void singer(Osc @ k, dur t, int bLength) {
    // Pause for banjos
    bLength*t => now;
    
    11 => int numNotes; // length of song
    int song[2][numNotes]; // song[0]: MIDI notes
                           // song[1]: Num of 16th notes
    [ 61, 64, 69, 71, 73, 73, 62, 64, 69, 69, 71 ] @=> song[0];
    [  4,  6,  2,  2,  2,  8,  4,  4,  4,  4,  6 ] @=> song[1];
    
    for(0 => int i; i < numNotes; i++) {
        Std.mtof(song[0][i]) => k.freq; // Assign freq
        t*song[1][i] => now;               // Assign time
    }
    0 => k.freq;
}


while(true) {
    200::ms => now;
}


