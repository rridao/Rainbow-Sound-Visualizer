//-----------------------------------------------------------------------------
// name: VisualSine.cpp
// desc: hello sine wave, real-time
//
// author: Ge Wang (ge@ccrma.stanford.edu)
//   date: fall 2014
//   uses: RtAudio by Gary Scavone
//-----------------------------------------------------------------------------
#include "RtAudio/RtAudio.h"
#include "chuck_fft.h"
#include "chuck.h"
#include <math.h>
#include <stdlib.h>
#include <iostream>
using namespace std;

#ifdef __MACOSX_CORE__
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#endif




//-----------------------------------------------------------------------------
// function prototypes
//-----------------------------------------------------------------------------
void initGfx();
void idleFunc();
void displayFunc();
void reshapeFunc( GLsizei width, GLsizei height );
void keyboardFunc( unsigned char, int, int );
void mouseFunc( int button, int state, int x, int y );
//void drawSun( SAMPLE * stereobuffer, int len, int channels);
void drawTime();
inline double map_log_spacing( double ratio, double power );
double compute_log_spacing( int fft_size, double factor );

// our datetype
#define SAMPLE float
// corresponding format for RtAudio
#define MY_FORMAT RTAUDIO_FLOAT32
// sample rate
#define MY_SRATE 44100
// number of channels
#define MY_CHANNELS_INPUT 1
#define MY_CHANNELS_OUTPUT 2
// number of buffers
#define SND_BUFFER_SIZE 1024
#define SND_FFT_SIZE (SND_BUFFER_SIZE * 2)
// for convenience
#define MY_PIE 3.14159265358979

// width and height
long g_width = 1024;
long g_height = 720;
// global buffer
SAMPLE * g_buffer = NULL;
long g_bufferSize;

// Global Variables //
bool g_draw_dB = false;
//bool g_drawSun = true;
ChucK * the_chuck = NULL;
//GLfloat g_sunScale = 0.45f;
GLint g_delay = SND_BUFFER_SIZE/2;

// Global Audio Buffer (From sound peek) //
SAMPLE g_fft_buffer[SND_FFT_SIZE];
SAMPLE g_audio_buffer[SND_BUFFER_SIZE]; // latest mono buffer (possibly preview)
SAMPLE g_stereo_buffer[SND_BUFFER_SIZE*2]; // current stereo buffer (now playing)
SAMPLE g_back_buffer[SND_BUFFER_SIZE]; // for lissajous
SAMPLE g_cur_buffer[SND_BUFFER_SIZE];  // current mono buffer (now playing), for lissajous
//GLfloat g_window[SND_BUFFER_SIZE]; // DFT transform window
GLfloat g_log_positions[SND_FFT_SIZE/2]; // precompute positions for log spacing
//GLint g_buffer_size = SND_BUFFER_SIZE; //MIGHT NOT NEED
GLint g_fft_size = SND_FFT_SIZE;

// for log scaling
GLdouble g_log_space = 0;
GLdouble g_log_factor = 1;

// the index associated with the waterfall
GLint g_wf = 0;

GLint g_cloudCount = 0;
GLfloat g_cloudRand = 0.0;

// for waterfall
struct Pt2D { float x; float y; };
Pt2D ** g_spectrums = NULL;
GLuint g_depth = 1;
// array of booleans for waterfall
GLboolean * g_draw = NULL;

SAMPLE ** g_waveforms = NULL;
GLfloat g_wf_delay_ratio = 1.0f / 3.0f;
GLuint g_wf_delay = 0; //because there is no file yet (GLuint)(g_depth * g_wf_delay_ratio + .5f);

//-----------------------------------------------------------------------------
// name: callme()
// desc: audio callback
//-----------------------------------------------------------------------------
int callme (void * outputBuffer, void * inputBuffer, unsigned int numFrames,
           double streamTime, RtAudioStreamStatus status, void * data) {
    // cast!
    SAMPLE * input  = (SAMPLE *)inputBuffer;
    SAMPLE * output = (SAMPLE *)outputBuffer;
    
    // compute chuck!
    // (TODO: to fill in)
    
    // fill
    for (int i = 0; i < numFrames; i++) {
        // copy the input to visualize only the left-most channel
        g_buffer[i] = input[i*MY_CHANNELS_INPUT];
        
        // also copy in the output from chuck to our visualizer
        // (TODO: to fill in)
        
        // mute output -- TODO will need to disable this once ChucK produces output, in order for you to hear it!
        for (int j = 0; j < MY_CHANNELS_OUTPUT; j++) {
            output[i*MY_CHANNELS_OUTPUT + j] = 0;
        }
    }
    
    // copy
    memcpy( g_stereo_buffer, input, numFrames * 2 * sizeof(SAMPLE) );

    // convert stereo to mono
    for (int i = 0; i < numFrames; i++) {
//        fprintf(stderr, "Stereo[%d]: %f ", i, g_stereo_buffer[i]);
//        fprintf(stderr, "Calc[%d]: %f ", i, g_stereo_buffer[i*2] + g_stereo_buffer[i*2+1]);
        g_audio_buffer[i] = g_stereo_buffer[i*2] + g_stereo_buffer[i*2+1];
        g_audio_buffer[i] /= 2.0f;
//        fprintf(stderr, "Audio[%d]: %f\n", i, g_audio_buffer[i]);
//        fprintf(stderr, "Stereo buffer[%d]: %f", i, g_stereo_buffer[i]);
    }
    
    
    
    return 0;
}

//-----------------------------------------------------------------------------
// name: main()
// desc: entry point
//-----------------------------------------------------------------------------
int main (int argc, char ** argv) {
    // instantiate RtAudio object
    RtAudio audio;
    // variables
    unsigned int bufferBytes = 0;
    // frame size
    unsigned int bufferFrames = 1024;
    
    // check for audio devices
    if (audio.getDeviceCount() < 1) {
        // nopes
        cout << "no audio devices found!" << endl;
        exit( 1 );
    }
    
    // initialize GLUT
    glutInit( &argc, argv );
    // init gfx
    initGfx();
    
    // let RtAudio print messages to stderr.
    audio.showWarnings( true );
    
    // set input and output parameters
    RtAudio::StreamParameters iParams, oParams;
    iParams.deviceId = audio.getDefaultInputDevice();
    iParams.nChannels = 1;
    iParams.firstChannel = 0;
    oParams.deviceId = audio.getDefaultOutputDevice();
    oParams.nChannels = 2;
    oParams.firstChannel = 0;
    
    // create stream options
    RtAudio::StreamOptions options;
    
    // go for it
    try {
        // open a stream
        audio.openStream( &oParams, &iParams, MY_FORMAT, MY_SRATE, &bufferFrames, &callme, (void *)&bufferBytes, &options );
    } catch (RtError& e) {
        // error!
        cout << e.getMessage() << endl;
        exit( 1 );
    }
    
    // compute
    bufferBytes = bufferFrames * MY_CHANNELS_OUTPUT * sizeof(SAMPLE);
    // allocate global buffer
    g_bufferSize = bufferFrames;
    g_buffer = new SAMPLE[g_bufferSize];
    memset( g_buffer, 0, sizeof(SAMPLE)*g_bufferSize );
    
    // set up chuck
    the_chuck = new ChucK();
    
    // TODO: set sample rate and number of in/out channels on our chuck
    the_chuck -> setParam("SAMPLE_RATE", MY_SRATE);
    the_chuck -> setParam("INPUT_CHANNELS", MY_CHANNELS_INPUT);
    the_chuck -> setParam("OUTPUT_CHANNELS", MY_CHANNELS_OUTPUT);
    
    // TODO: initialize our chuck
    the_chuck -> init();
    
    // TODO: run a chuck program
    the_chuck -> compileFile("kermit.ck", "", 1);//, const std::string & argsTogether, int count = 1 );
    
    // initialize audio
    if( g_wf_delay )
    {
        fprintf(stderr, "g_wf_delay lol");
        g_waveforms = new SAMPLE *[g_wf_delay];
        for( int i = 0; i < g_wf_delay; i++ )
        {
            // allocate memory (stereo)
            g_waveforms[i] = new SAMPLE[g_bufferSize * 2];
            // zero it
            memset( g_waveforms[i], 0, g_bufferSize * 2 * sizeof(SAMPLE) );
        }
    }
    
    // go for it
    try {
        // start stream
        audio.startStream();
        
        // let GLUT handle the current thread from here
        glutMainLoop();
        
        // stop the stream.
        audio.stopStream();
    } catch (RtError& e) {
        // print error message
        cout << e.getMessage() << endl;
        goto cleanup;
    }
    
cleanup:
    // close if open
    if (audio.isStreamOpen()) audio.closeStream();
    
    // done
    return 0;
}

//-----------------------------------------------------------------------------
// Name: initGfx()
// Desc: initialize graphics
//-----------------------------------------------------------------------------
void initGfx() {
    // double buffer, use rgb color, enable depth buffer
    glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH );
    // initialize the window size
    glutInitWindowSize( g_width, g_height );
    // set the window postion
    glutInitWindowPosition( 100, 100 );
    // create the window
    glutCreateWindow( "VisualSine" );
    
    // set the idle function - called when idle
    glutIdleFunc( idleFunc );
    // set the display function - called when redrawing
    glutDisplayFunc( displayFunc );
    // set the reshape function - called when client area changes
    glutReshapeFunc( reshapeFunc );
    // set the keyboard function - called on keyboard events
    glutKeyboardFunc( keyboardFunc );
    // set the mouse function - called on mouse stuff
    glutMouseFunc( mouseFunc );
    
    // set clear color
    glClearColor( 0, 0, 0, 1 );
    // enable color material
    glEnable( GL_COLOR_MATERIAL );
    // enable depth test
    glEnable( GL_DEPTH_TEST );
    
    // initialize for waterfall
    g_spectrums = new Pt2D *[g_depth];
    for( int i = 0; i < g_depth; i++ )
    {
        g_spectrums[i] = new Pt2D[SND_FFT_SIZE];
        memset( g_spectrums[i], 0, sizeof(Pt2D)*SND_FFT_SIZE );
    }
    g_draw = new GLboolean[g_depth];
    memset( g_draw, 0, sizeof(GLboolean)*g_depth );
    // compute log spacing
    g_log_space = compute_log_spacing( g_fft_size / 2, g_log_factor );
}

//-----------------------------------------------------------------------------
// Name: map_log_spacing( )
// Desc: ...
//-----------------------------------------------------------------------------
inline double map_log_spacing( double ratio, double power )
{
    // compute location
    return ::pow(ratio, power) * g_fft_size;
}

//-----------------------------------------------------------------------------
// Name: compute_log_spacing( )
// Desc: ...
//-----------------------------------------------------------------------------
double compute_log_spacing( int fft_size, double power )
{
    int maxbin = fft_size; // for future in case we want to draw smaller range
    int minbin = 0; // what about adding this one?
    
    for(int i = 0; i < fft_size; i++)
    {
        // compute location
        g_log_positions[i] = map_log_spacing( (double)i/fft_size, power );
        // normalize, 1 if maxbin == fft_size
        g_log_positions[i] /= pow((double)maxbin/fft_size, power);
    }
    
    return 1/::log(fft_size);
}

//-----------------------------------------------------------------------------
// Name: reshapeFunc()
// Desc: called when window size changes
//-----------------------------------------------------------------------------
void reshapeFunc( GLsizei w, GLsizei h ) {
    // save the new window size
    g_width = w; g_height = h;
    // map the view port to the client area
    glViewport( 0, 0, w, h );
    // set the matrix mode to project
    glMatrixMode( GL_PROJECTION );
    // load the identity matrix
    glLoadIdentity( );
    // create the viewing frustum
    gluPerspective( 45.0, (GLfloat) w / (GLfloat) h, 1.0, 300.0 );
    // set the matrix mode to modelview
    glMatrixMode( GL_MODELVIEW );
    // load the identity matrix
    glLoadIdentity( );
    // position the view point
    gluLookAt( 0.0f, 0.0f, 10.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f );
}

//-----------------------------------------------------------------------------
// Name: keyboardFunc()
// Desc: key event
//-----------------------------------------------------------------------------
void keyboardFunc( unsigned char key, int x, int y ) {
    switch( key )
    {
//        case 'l':
//            g_drawSun = !g_drawSun;
//            break;
        case 'Q':
        case 'q':
            exit(1);
            break;
            
        case 'd':
            g_draw_dB = !g_draw_dB;
            break;
    }
    
    glutPostRedisplay( );
}

//-----------------------------------------------------------------------------
// Name: mouseFunc()
// Desc: handles mouse stuff
//-----------------------------------------------------------------------------
void mouseFunc( int button, int state, int x, int y ) {
    if( button == GLUT_LEFT_BUTTON )
    {
        // when left mouse button is down
        if( state == GLUT_DOWN )
        {
        }
        else
        {
        }
    }
    else if ( button == GLUT_RIGHT_BUTTON )
    {
        // when right mouse button down
        if( state == GLUT_DOWN )
        {
        }
        else
        {
        }
    }
    else
    {
    }
    
    glutPostRedisplay( );
}

//-----------------------------------------------------------------------------
// Name: idleFunc()
// Desc: callback from GLUT
//-----------------------------------------------------------------------------
void idleFunc() {
    // render the scene
    glutPostRedisplay();
}

//-----------------------------------------------------------------------------
// name: drawSun()
// desc: draws lissajous sun
//-----------------------------------------------------------------------------
/*void drawSun( SAMPLE * stereobuffer, int len, int channels) {
    float x, y;
    SAMPLE * buffer;
    
    // 1 or 2 channels only for now
    assert( channels >= 1 && channels <= 2 );
    
    // mono
    if( channels == 1 )
    {
        buffer = g_cur_buffer;
        // convert to mono
        for( int m = 0; m < len; m++)
        {
            buffer[m] = stereobuffer[m*2] + stereobuffer[m*2+1];
            buffer[m] /= 2.0f;
        }
    }
    else
    {
        buffer = stereobuffer;
    }
    
    // color
    glColor3f( 1.0f, 1.0f, .5f );
    // save current matrix state
    glPushMatrix();
    // translate
    glTranslatef( 1.2f, 0.0f, 0.0f );
    // draw it
    glBegin( GL_LINE_STRIP );
    for( int i = 0; i < len * channels; i += channels )
    {
        x = buffer[i] * g_sunScale;
        if( channels == 1 )
        {
            // delay
            y = (i - g_delay >= 0) ? buffer[i-g_delay] : g_back_buffer[len + i-g_delay];
            y *= g_sunScale;
        }
        else
        {
            y = buffer[i + channels-1] * g_sunScale;
        }
        
        glVertex3f( x, y, 0.0f );
        // glVertex3f( x, y, sqrt( x*x + y*y ) * -g_sunScale );
    }
    glEnd();
    // restore matrix state
//    glPopMatrix();
 
    // hmm...
    if( channels == 1 )
        memcpy( g_back_buffer, buffer, len * sizeof(SAMPLE) );
}*/

//-----------------------------------------------------------------------------
// Name: drawTime()
// Desc: draw time domain
//-----------------------------------------------------------------------------
void drawTime() {
    // save the current matrix state
    glPushMatrix();
    // line width
    glLineWidth(1.0f);
    // define a starting point
    GLfloat x = -1.5f;
    // increment
    GLfloat xinc = ::fabs(x*2 / g_bufferSize);
    
    // color
    glColor3ub(190, 190, 190);
    
    // start primitive
    glBegin(GL_LINE_STRIP);
    
    // loop over buffer
    for (int i = 0; i < g_bufferSize; i++) {
        // plot
        glVertex2f(x, 3*g_buffer[i]-2.0f);
        // increment x
        x += xinc;
    }
    
    // end primitive
    glEnd();
    // restore previous matrix state
    glPopMatrix();
}

//-----------------------------------------------------------------------------
// Name: drawRainbow()
// Desc: draw time domain
//-----------------------------------------------------------------------------
void drawRainbowLol() {
    // save the current matrix state
    glPushMatrix();
    // line width
    glLineWidth(1.0f);
    
    // distance between stripes
    GLfloat width = 0.6f;
    // define starting point
    GLfloat xS = -1.5f, yS = -1.0f;
    // define ending point
    GLfloat xE = 1.5f, yE = 4.0f;
    
    // red
    glColor3ub(255, 0, 0);
    glBegin(GL_LINE_STRIP);
    glVertex2f(xS, yS);
    glVertex2f(xE, yE);
    glEnd();
    // orange
    glColor3ub(255,127, 0);
    glBegin(GL_LINE_STRIP);
    glVertex2f(xS+width, yS);
    glVertex2f(xE+width, yE);
    glEnd();
    // yellow
    glColor3ub(255, 255, 0);
    glBegin(GL_LINE_STRIP);
    glVertex2f(xS+width*2, yS);
    glVertex2f(xE+width*2, yE);
    glEnd();
    // green
    glColor3ub(0, 255, 0);
    glBegin(GL_LINE_STRIP);
    glVertex2f(xS+width*3, yS);
    glVertex2f(xE+width*3, yE);
    glEnd();
    // blue
    glColor3ub(0, 0, 255);
    glBegin(GL_LINE_STRIP);
    glVertex2f(xS+width*4, yS);
    glVertex2f(xE+width*4, yE);
    glEnd();
    // purple
    glColor3ub(148, 0, 211);
    glBegin(GL_LINE_STRIP);
    glVertex2f(xS+width*5, yS);
    glVertex2f(xE+width*5, yE);
    glEnd();
    
    // restore previous matrix state
    glPopMatrix();
}

//-----------------------------------------------------------------------------
// Name: drawCloud()
// Desc: draw time domain
//-----------------------------------------------------------------------------
void drawCloud() {
    // save the current matrix state
    glPushMatrix();
    // line width
    glLineWidth(2.0f);
    // color
    glColor3ub(190, 190, 190);
    
    // start primitive
    glBegin(GL_LINE_STRIP);
    
    // quarter cloud
    /*glVertex2f(0.0f, -1.0f);
    glVertex2f(-0.75f, -1.25f);
    glVertex2f(-1.5f, -1.0f);
    glVertex2f(-2.5f, -1.5f);
    glVertex2f(-2.2f, -2.0f);*/
    
    if (g_cloudCount == 0) {
        g_cloudRand = (rand() % 100 - 50)/500.0;
        g_cloudCount = 50;
    } else {
        g_cloudCount--;
    }

    glVertex2f(0.0f+g_cloudRand, -1.0f+g_cloudRand);
    glVertex2f(-0.75f+g_cloudRand, -1.25f+g_cloudRand);
    glVertex2f(-1.5f+g_cloudRand, -1.0f+g_cloudRand);
    glVertex2f(-2.5f+g_cloudRand, -1.5f+g_cloudRand);
    glVertex2f(-2.2f+g_cloudRand, -2.0f+g_cloudRand);

    // end primitive
    glEnd();
    // restore previous matrix state
    glPopMatrix();
}

//-----------------------------------------------------------------------------
// Name: displayFunc()
// Desc: callback function invoked to draw the client area
//-----------------------------------------------------------------------------
void displayFunc( )
{
    // local state
    static GLfloat zrot = 0.0f, c = 0.0f;
    
    // local variables
    SAMPLE * buffer = g_fft_buffer;
    
    // clear the color and depth buffers
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    
    // get the latest (possibly preview) window
    memset( buffer, 0, SND_FFT_SIZE * sizeof(SAMPLE) );
    
    // copy currently playing audio into buffer
    memcpy( buffer, g_audio_buffer, g_bufferSize * sizeof(SAMPLE) );

//    // copy currently playing audio into buffer
//    drawSun( g_stereo_buffer, g_buffer_size, 1 );
    
    // draw time domain
    drawTime();
    // draw cloud
    drawCloud();
    
//    drawRainbowLol();
    
    // take forward FFT; result in buffer as FFT_SIZE/2 complex values
    rfft( (float *)buffer, g_fft_size/2, FFT_FORWARD );
    // cast to complex
    complex * cbuf = (complex *)buffer;
    
    // reset drawing offsets
    GLfloat x = -1.8f, y = -1.0f, inc = 3.6f;
    
    // color the spectrum
    glColor3f( 0.4f, 1.0f, 0.4f );
    // set vertex normals
    glNormal3f( 0.0f, 1.0f, 0.0f );
    
//    // copy current magnitude spectrum into waterfall memory
//    for(int i = 0; i < g_fft_size/2; i++ )
//    {
//        // copy x cgoordinate
//        g_spectrums[g_wf][i].x = x;
//        // copy y, could have diff scaling, see sndpeek
//        g_spectrums[g_wf][i].y = 1.8f * ::pow( 25 * cmp_abs( cbuf[i] ), .5 ) + y;
//        // increment x
//        x += inc * 2.0f;
//    }
    
    // draw the right things
    g_draw[g_wf] = true; //Might not need this line
    //Might mneed stuff for g_starting//
    g_draw[(g_wf+g_wf_delay)%g_depth] = true;
    
    // reset drawing variables
    x = -1.8f;
    inc = 3.6f / g_fft_size;
    
    // save current matrix state
    glPushMatrix();
    // translate in world coordinate
//    glTranslatef( x, 0.0, 0.0 );
    // scale it
//    glScalef( inc, 1.0 , -0.12f);
    
    // --- 1 waterfall --- //
    
    // get the magnitude spectrum of layer
    Pt2D * pt = g_spectrums[(g_wf)%g_depth];
    // present
    glLineWidth(2.0f);
    //define start
    x = -1.0f;
    GLfloat y2 = -0.75f;
    GLfloat xWidth = 0.5f, yWidth = 0.5f, length = 1.5f;
    GLfloat temp = pt->y;
    
    while (y2 < 4.0f) {
        // get the magnitude spectrum of layer
        pt = g_spectrums[(g_wf)%g_depth];
    
        glColor3ub(255,0,0);
        // render the red spectrum layer
        glBegin( GL_LINE_STRIP );
        for (GLint j = 0; j < g_fft_size/6; j++, pt++) {
            // draw the vertex
            glVertex2f( g_log_positions[j]/1000+x, temp+y2);
        }
        glEnd();
        
        glColor3ub(255,127,0);
        // render the red spectrum layer
        glBegin( GL_LINE_STRIP );
        for (GLint j = g_fft_size/6; j < g_fft_size/3; j++, pt++) {
            // draw the vertex
            glVertex2f( g_log_positions[j]/1000+x, temp+y2);
        }
        glEnd();
        
        glColor3ub(255,255,0);
        // render the red spectrum layer
        glBegin( GL_LINE_STRIP );
        for (GLint j = g_fft_size/3; j < g_fft_size/2; j++, pt++) {
            // draw the vertex
            glVertex2f( g_log_positions[j]/1000+x, temp+y2);
        }
        glEnd();
        
//        glColor3ub(0,255,0);
//        // render the red spectrum layer
//        glBegin( GL_LINE_STRIP );
//        for (GLint j = g_fft_size/2; j < g_fft_size/1.5f; j++, pt++) {
//            // draw the vertex
//            glVertex2f( g_log_positions[j]/1000+x, temp+y2);
//        }
//        glEnd();
//
//        glColor3ub(0,0,255);
//        // render the red spectrum layer
//        glBegin( GL_LINE_STRIP );
//        for (GLint j = g_fft_size/1.5f; j < g_fft_size/0.83f; j++, pt++) {
//            // draw the vertex
//            glVertex2f( g_log_positions[j]/1000+x, temp+y2);
//        }
//        glEnd();
//
//        glColor3ub(148,0,211);
//        // render the red spectrum layer
//        glBegin( GL_LINE_STRIP );
//        for (GLint j = g_fft_size/0.83f; j < g_fft_size; j++, pt++) {
//            // draw the vertex
//            glVertex2f( g_log_positions[j]/1000+x, temp+y2);
//        }
//        glEnd();
        
        x += xWidth;
        y2 += yWidth;

    }
    
    // restore matrix state
    glPopMatrix();
    
    
    
    
    // flush!
    glFlush( );
    // swap the double buffer
    glutSwapBuffers( );
}
