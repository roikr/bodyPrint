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

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
    
    ofxOpenNI2 cam;
    ofTexture depthTex;
    ofVboMesh mesh;
    ofShader cloudShader;
    ofxPanel gui;
    
    ofParameter<int> pointSize;
    ofParameter<float> gridScale,depthScale;
    ofParameter<float>minEdge0,maxEdge0,minEdge1,maxEdge1;
    ofParameter<string> fps;
    ofParameter<ofVec3f> sceneRotation,position,cameraRotation;
    
    bool bUseShader;
   
		
};
