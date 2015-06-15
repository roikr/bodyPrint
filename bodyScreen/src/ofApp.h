#pragma once

#include "ofxOpenNI2.h"
#include "ofMain.h"
#include "ofxGui.h"
#include "ofxVideoRecorder.h"
#include "ofxOpenCv.h"
#include "ofxQuadWarp.h"

struct layer {
    ofFbo fbo;
    float hueOffset;
};

struct camera {
    ofxOpenNI2 sensor;
    ofParameter<float>minEdge0,maxEdge0,minEdge1,maxEdge1;
    ofMatrix4x4 mat;
    ofVboMesh mesh;
    ofParameterGroup params;
    ofShortPixels background;
    
    ofVec3f tempMarker;
    vector<ofVec3f> markers;
    ofParameter<ofVec3f> offset;
    ofVec3f downOffset;
    
    ofColor color;
    
    bool bFlipX;
    bool bFlipY;
    bool bFlipZ;
};

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();
        void exit();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
    
        void updateMesh(camera &cam);
        void renderCam(camera &cam,int pointSize);
        void updateLayer(layer &l,ofFbo &depth,float decay);
        void captureBackground();
    
        void saveScreenMatrix(camera &cam);
    
    
    camera cam[2];
    
    ofParameter<int> pointSize;
    //ofParameter<float>depthScale;
    ofShader cloudShader;
    ofFbo depthFbo;
    
    ofParameter<float> camZPos;
    ofCamera virtualCam;
    ofVec2f downPos;
    float downCamZPos;
    
    
    ofParameter<float> tolerance; // for background subtraction
    
    ofFbo camFbo; // duplicate depthFbo for freeze and fade
    
    
    ofParameterGroup visualParams;
    ofShader strobeShader;
    ofFbo ping,pong;
    ofParameter<float> decay0,decay1,sat,hueRate,offset;
    ofParameter<int> strobeRate;
    int frameNum;
    ofShader blurShader;
    ofParameter<float> variance;
    ofParameter<int> radius;
    
    ofParameterGroup levels;
    ofParameter<float> inputBlack,inputWhite,gamma,outputBlack,outputWhite;
    
    layer camLayer;
    vector<layer> layers;
    ofFbo compFbo;
    ofShader compShader;
    
    ofShader screenShader;
    
    ofxCvGrayscaleImage grayImg;
    ofxCvContourFinder contour;
    
    ofParameterGroup detectionParams;
    ofParameter<int> blobPointSize;
    ofParameter<float> minArea,maxArea;
    ofParameter<bool>blobDetection;
    
    ofFbo blobFbo;
    
    ofxVideoRecorder recorder;
    
    ofParameterGroup timerParams;
    float startTime;
    ofParameter<string> videoQueue;
    ofParameter<float> recordDuration,waitDuration,idleInterval,minimumDuration;
    float recordTimer,waitTimer,idleTimer;
    ofParameter<string> recordTime,waitTime;
    string filename;
    
    vector<string> memories;
    vector<ofVideoPlayer> players;
    
    
    ofParameter<string> fps;
    ofxPanel gui;
    
    ofMatrix4x4 mat;
    
    int state;
    
    
    bool bFirstIdle;
    
    ofSoundPlayer recSound,ambientSound;
    ofParameter<float>recLevel,ambLevel;
    
    bool bShowGui;
    bool bCaptureBg;
    bool bUseBg;
    
    ofxQuadWarp warper;
    bool bEnableWarper;
    bool bShowMouse;
};
