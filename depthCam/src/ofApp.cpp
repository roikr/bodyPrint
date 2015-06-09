#include "ofApp.h"
//#include "Shaders.h"

#define STRINGIFY(A) #A
//--------------------------------------------------------------
void ofApp::setup(){
    ofDisableArbTex();
    
    
    ofxOpenNI2::init();
    
    vector<string> devices = ofxOpenNI2::listDevices();
    
    for (vector<string>::iterator iter=devices.begin(); iter!=devices.end(); iter++) {
        camera c;
        c.cam = new ofxOpenNI2;
        c.cam->setup(iter->c_str());
        c.cam->setDepthMode(5);
        c.rotation = 0;
        c.tex.allocate(c.cam->depthWidth, c.cam->depthHeight, GL_R16 );
        
        cameras.push_back(c);
    }
    

}

//--------------------------------------------------------------
void ofApp::update(){
    ofSetWindowTitle(ofToString(ofGetFrameRate()));
    
    
    for (vector<camera>::iterator iter=cameras.begin(); iter!=cameras.end(); iter++) {
        iter->cam->update();
        
        
        if(iter->cam->bNewDepth) {
            iter->tex.loadData(iter->cam->getDepth(), iter->cam->depthWidth, iter->cam->depthHeight,GL_RED);
            
            
            
        }
    }
    
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofSetColor(255);
    ofClear(0);
    
    for (vector<camera>::iterator iter=cameras.begin(); iter!=cameras.end(); iter++) {
        ofPushMatrix();
        
        ofTranslate(distance(cameras.begin(), iter)*cameras[0].cam->depthWidth+iter->cam->depthWidth/2, iter->cam->depthWidth/2);
        ofPushMatrix();
        ofRotate(iter->rotation*90);
        ofTranslate(-iter->cam->depthWidth/2, -iter->cam->depthHeight/2);
        iter->tex.draw(0, 0);
        ofPopMatrix();
        ofPopMatrix();
        
    
    }
    
    
}

void ofApp::exit() {
    for (vector<camera>::iterator iter=cameras.begin(); iter!=cameras.end(); iter++) {
        iter->cam->exit();
        delete iter->cam;
    }

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    switch (key) {
        case '1':
            cameras[0].rotation--;
            break;
        case '2':
            cameras[0].rotation++;
            break;
        case '3':
            cameras[1].rotation--;
            break;
        case '4':
            cameras[1].rotation++;
            break;
            
        default:
            break;
    }
    
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
