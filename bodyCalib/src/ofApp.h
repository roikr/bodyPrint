#pragma once

#include "ofxOpenNI2.h"
#include "ofMain.h"
#include "ofxGui.h"

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();
        void exit();
    
        void updateMesh();
        void saveScreenMatrix();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
    
    ofxOpenNI2 depthCam;
    ofTexture depthTex;
    ofVboMesh mesh;
    ofShader cloudShader;
    ofxPanel gui;
    
    ofParameter<int> pointSize;
    ofParameter<float>minEdge0,maxEdge0,position,minEdge1,maxEdge1,depthScale;
    ofParameter<string> fps;

    
    
    
    bool bUseShader;
    
    int state;
    ofVec3f tempMarker;
    vector<ofVec3f> markers;
    
    ofVec2f lastPos;
    ofVec2f downPos;
    ofMatrix4x4 screen;
    ofCamera cam;
    
    
		
};
