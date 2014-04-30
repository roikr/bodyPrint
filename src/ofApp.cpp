#include "ofApp.h"
#include "Shaders.h"

#define STRINGIFY(A) #A
#define MAX_POSITION 100000.0



//--------------------------------------------------------------
void ofApp::setup(){
    ofDisableArbTex();
//    ofDisableSetupScreen();
    
    
    gui.setup("panel");
    gui.add(fps.set("fps",""));
    gui.add(pointSize.set("pointSize", 3, 1, 10));
    gui.add(minEdge0.set("minEdge0", 0.0, 0.0, 1.0));
    gui.add(maxEdge0.set("maxEdge0", 1.0, 0.0, 1.0));
    gui.add(minEdge1.set("minEdge1", 0.0, 0.0, 1.0));
    gui.add(maxEdge1.set("maxEdge1", 1.0, 0.0, 1.0));
    gui.add(nearClipping.set("nearClipping", 0, 0, MAX_POSITION));
    gui.add(farClipping.set("farClipping", MAX_POSITION, 0, MAX_POSITION));
    gui.add(position.set("position", ofVec3f(0), ofVec3f(-MAX_POSITION), ofVec3f(MAX_POSITION)));
    gui.add(cameraRotation.set("cameraRotation", ofVec3f(0), ofVec3f(-180), ofVec3f(180)));
    gui.add(sceneRotation.set("sceneRotation", ofVec3f(0), ofVec3f(-180), ofVec3f(180)));
    
    gui.loadFromFile("settings.xml");
    
    
    ofxOpenNI2::init();
    cam.setup();
    cam.setDepthMode(5);
   
    depthTex.allocate(cam.depthWidth, cam.depthHeight, GL_R16 );
    createCloudShader(cloudShader);
    
    bUseShader = false;
    

}

void ofApp::updateMesh() {
    
    mesh.clear();
    mesh.setMode(OF_PRIMITIVE_POINTS);
    
    int rows=cam.depthHeight;
    int columns=cam.depthWidth;
    
    int minE = minEdge0*USHRT_MAX;
    int maxE = maxEdge0*USHRT_MAX;
    
    for(int iy = 0; iy < rows; iy++) {
        for(int ix = 0; ix < columns; ix++) {
            short unsigned int depth = cam.getDepth()[iy*columns+ix];
            if (depth && depth> minE && depth<maxE) {
                mesh.addVertex(cam.getWorldCoordinateAt(ix, iy, depth));
                
            }
            
            
        }
    }

    
}

//--------------------------------------------------------------
void ofApp::update(){

    fps=ofToString(ofGetFrameRate());
    cam.update();
    if (cam.bNewDepth) {
        depthTex.loadData(cam.getDepth(), cam.depthWidth, cam.depthHeight,GL_RED);
        updateMesh();
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofSetupScreenPerspective(1024,768,60,nearClipping,farClipping);
//    ofSetupScreenOrtho(1024,768,-nearClipping,-farClipping);
    ofBackground(0);
    ofSetColor(255);
    
   
    
    if (bUseShader) {
        cloudShader.begin();
        cloudShader.setUniform1f("minEdge", minEdge1);
        cloudShader.setUniform1f("maxEdge",maxEdge1);

    } else {
        depthTex.draw(0, 0);
    }
    
    
    ofPushMatrix();

    
    ofRotateX(sceneRotation->x);
	ofRotateY(sceneRotation->y);
	ofRotateZ(sceneRotation->z);
	ofTranslate(position->x, position->y, position->z);
	ofRotateX(cameraRotation->x);
	ofRotateY(cameraRotation->y);
	ofRotateZ(cameraRotation->z);
    glPointSize(pointSize);
    ofEnableDepthTest();
//    ofScale(0.3,0.3, 0.3); // make y point down
    mesh.drawVertices();
    ofDisableDepthTest();
    
    ofPopMatrix();
    
    if (bUseShader) {
        cloudShader.end();
    }
    
    ofSetupScreen();
    gui.draw();
}

void ofApp::exit() {
    cam.exit();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    bUseShader =!bUseShader;
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
