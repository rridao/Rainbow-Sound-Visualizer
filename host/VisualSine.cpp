//-----------------------------------------------------------------------------
// name: VisualSine.cpp
// desc: hello sine wave, real-time
//
// author: Ge Wang (ge@ccrma.stanford.edu)
//   date: fall 2014
//   uses: RtAudio by Gary Scavone
//-----------------------------------------------------------------------------
#include "RtAudio/RtAudio.h"
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
#define MY_SND_BUFFER_SIZE 1024
// for convenience
#define MY_PIE 3.14159265358979

// width and height
long g_width = 1024;
long g_height = 720;
// global buffer
SAMPLE * g_buffer = NULL;
long g_bufferSize;

// global variables
bool g_draw_dB = false;
bool g_drawSun = true;
ChucK * the_chuck = NULL;
GLfloat g_sun_scale = 0.45f;
GLint g_delay = MY_SND_BUFFER_SIZE/2;

// global audio buffer
SAMPLE g_back_buffer[MY_SND_BUFFER_SIZE]; // for lissajous
SAMPLE g_cur_buffer[MY_SND_BUFFER_SIZE];  // current mono buffer (now playing), for lissajous
SAMPLE g_stereo_buffer[MY_SND_BUFFER_SIZE*2]; // current stereo buffer (now playing)
SAMPLE g_audio_buffer[MY_SND_BUFFER_SIZE]; // latest mono buffer (possibly preview)
GLint g_buffer_size = MY_SND_BUFFER_SIZE;



//-----------------------------------------------------------------------------
// name: callme()
// desc: audio callback
//-----------------------------------------------------------------------------
int callme( void * outputBuffer, void * inputBuffer, unsigned int numFrames,
            double streamTime, RtAudioStreamStatus status, void * data ) {
    // cast!
    SAMPLE * input = (SAMPLE *)inputBuffer;
    SAMPLE * output = (SAMPLE *)outputBuffer;
    
    // compute chuck!
    // (TODO: to fill in)
    
    //COPY FOR SUN?????
    memcpy( g_stereo_buffer, input, numFrames * 2 * sizeof(SAMPLE) );
    
    // fill
    for( int i = 0; i < numFrames; i++ ) {
        // copy the input to visualize only the left-most channel
        g_buffer[i] = input[i*MY_CHANNELS_INPUT];
        
        // also copy in the output from chuck to our visualizer
        // (TODO: to fill in)
        
        // mute output -- TODO will need to disable this once ChucK produces output, in order for you to hear it!
        for( int j = 0; j < MY_CHANNELS_OUTPUT; j++ ) {
            output[i*MY_CHANNELS_OUTPUT + j] = 0;
            
        }
        
        
        //HERE COMES THE SUN?
        g_audio_buffer[i] = g_stereo_buffer[i*2] + g_stereo_buffer[i*2+1];
        g_audio_buffer[i] /= 2.0f;
    }
    
    // For sun???
    // co
    
    
    
    return 0;
}




//-----------------------------------------------------------------------------
// name: main()
// desc: entry point
//-----------------------------------------------------------------------------
int main( int argc, char ** argv ) {
    // instantiate RtAudio object
    RtAudio audio;
    // variables
    unsigned int bufferBytes = 0;
    // frame size
    unsigned int bufferFrames = 1024;
    
    // check for audio devices
    if( audio.getDeviceCount() < 1 ) {
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
    } catch( RtError& e ) {
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
    
    
    // go for it
    try {
        // start stream
        audio.startStream();
        
        // let GLUT handle the current thread from here
        glutMainLoop();
        
        // stop the stream.
        audio.stopStream();
    } catch( RtError& e ) {
        // print error message
        cout << e.getMessage() << endl;
        goto cleanup;
    }
    
    cleanup:
    // close if open
    if( audio.isStreamOpen() ) {
        audio.closeStream();
    }
    
    // done
    return 0;
}




//-----------------------------------------------------------------------------
// Name: reshapeFunc()
// Desc: called when window size changes
//-----------------------------------------------------------------------------
void initGfx() {
    // double buffer, use rgb color, enable depth buffer
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    // initialize the window size
    glutInitWindowSize(g_width, g_height);
    // set the window postion
    glutInitWindowPosition(100, 100);
    // create the window
    glutCreateWindow("VisualSine");
    
    // set the idle function - called when idle
    glutIdleFunc(idleFunc);
    // set the display function - called when redrawing
    glutDisplayFunc(displayFunc);
    // set the reshape function - called when client area changes
    glutReshapeFunc(reshapeFunc);
    // set the keyboard function - called on keyboard events
    glutKeyboardFunc(keyboardFunc);
    // set the mouse function - called on mouse stuff
    glutMouseFunc(mouseFunc);
    
    // set clear color
    glClearColor(0, 0, 0, 1);
    // enable color material
    glEnable(GL_COLOR_MATERIAL);
    // enable depth test
    glEnable(GL_DEPTH_TEST);
}




//-----------------------------------------------------------------------------
// Name: reshapeFunc()
// Desc: called when window size changes
//-----------------------------------------------------------------------------
void reshapeFunc(GLsizei w, GLsizei h) {
    // save the new window size
    g_width = w; g_height = h;
    // map the view port to the client area
    glViewport(0, 0, w, h);
    // set the matrix mode to project
    glMatrixMode(GL_PROJECTION);
    // load the identity matrix
    glLoadIdentity();
    // create the viewing frustum
    gluPerspective(45.0, (GLfloat) w / (GLfloat) h, 1.0, 300.0);
    // set the matrix mode to modelview
    glMatrixMode(GL_MODELVIEW);
    // load the identity matrix
    glLoadIdentity();
    // position the view point
    gluLookAt(0.0f, 0.0f, 10.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
}




//-----------------------------------------------------------------------------
// Name: keyboardFunc()
// Desc: key event
//-----------------------------------------------------------------------------
void keyboardFunc(unsigned char key, int x, int y) {
    switch (key) {
        case 'l':
            g_drawSun = !g_drawSun;
        case 'd':
            g_draw_dB = !g_draw_dB;
            break;
        case 'Q':
        case 'q':
            exit(1);
            break;
    }
    
    glutPostRedisplay();
}




//-----------------------------------------------------------------------------
// Name: mouseFunc()
// Desc: handles mouse stuff
//-----------------------------------------------------------------------------
void mouseFunc(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        // when left mouse button is down
        if (state == GLUT_DOWN) {
        } else {
        }
    } else if (button == GLUT_RIGHT_BUTTON) {
        // when right mouse button down
        if (state == GLUT_DOWN) {
        } else {
        }
    } else {
    }
    
    glutPostRedisplay();
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
void drawSun(SAMPLE * stereobuffer, int len, int channels) {
    float x, y;
    SAMPLE * buffer;
    
    // 1 or 2 channels only for now
    assert(channels >= 1 && channels <= 2);
    
    // mono
    if (channels == 1) {
        buffer = g_cur_buffer;
        // convert to mono
        for (int m = 0; m < len; m++) {
            buffer[m] = stereobuffer[m*2] + stereobuffer[m*2+1];
            buffer[m] /= 2.0f;
        }
    } else {
        buffer = stereobuffer;
    }
    
    // color
    glColor3f(1.0f, 1.0f, .5f);
    // save current matrix state
    glPushMatrix();
    // translate
    glTranslatef(1.2f, 0.0f, 0.0f);
    // draw it
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i < len * channels; i += channels) {
        
        x = buffer[i] * g_sun_scale;
        if (channels == 1) {
            // delay
            y = (i - g_delay >= 0) ? buffer[i-g_delay] : g_back_buffer[len + i-g_delay];
            y *= g_sun_scale;
        } else {
            y = buffer[i + channels-1] * g_sun_scale;
        }
        
        glVertex3f(x, y, 0.0f);
        // glVertex3f( x, y, sqrt( x*x + y*y ) * -g_sun_scale );
    }
    glEnd();
    // restore matrix state
    glPopMatrix();
    
    // hmm...
    if (channels == 1) {
        memcpy(g_back_buffer, buffer, len * sizeof(SAMPLE));
    }
}

//-----------------------------------------------------------------------------
// Name: displayFunc()
// Desc: callback function invoked to draw the client area
//-----------------------------------------------------------------------------
void displayFunc() {
    // local state
    static GLfloat zrot = 0.0f, c = 0.0f;
    
    // clear the color and depth buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // line width
    glLineWidth(1.0);
    // define a starting point
    GLfloat x = -5;
    // increment
    GLfloat xinc = ::fabs(x*2 / g_bufferSize);
    
    // color
    glColor3f(.5, 1, .5);
    
    // start primitive
    glBegin(GL_LINE_STRIP);
 
    // loop over buffer
    for (int i = 0; i < g_bufferSize; i++) {
        // plot
        glVertex2f(x, 3*g_buffer[i]);
        // increment x
        x += xinc;
    }
    
    // end primitive
    glEnd();
    
    if(g_drawSun) {
        drawSun(g_stereo_buffer, g_buffer_size, 1);
    }

    // flush!
    glFlush();
    // swap the double buffer
    glutSwapBuffers();
}
