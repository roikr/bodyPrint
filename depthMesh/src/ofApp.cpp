#include "ofApp.h"
#include "Shaders.h"

#define STRINGIFY(A) #A
#define MAX_POSITION 1000.0



//--------------------------------------------------------------
void ofApp::setup(){
    ofDisableArbTex();
//    ofDisableSetupScreen();
    
    
    gui.setup("panel");
    gui.add(fps.set("fps",""));
    gui.add(pointSize.set("pointSize", 3, 1, 10));
    gui.add(gridScale.set("gridScale", -2.0, -5.0, 5.0)); // ( -1 for linux64)
    gui.add(gridOffset.set("gridOffset", -1.0, -1.0, 1.0)); // ( -1 for linux64)
    gui.add(minEdge0.set("minEdge0", 0.0, 0.0, 1.0));
    gui.add(maxEdge0.set("maxEdge0", 1.0, 0.0, 1.0));
    gui.add(depthScale.set("depthScale", -5, -10.0, 0.0)); // 10^-5
    gui.add(minEdge1.set("minEdge1", 0.0, 0.0, 1.0));
    gui.add(maxEdge1.set("maxEdge1", 1.0, 0.0, 1.0));
//    gui.add(nearClipping.set("nearClipping", 0, 0, MAX_POSITION));
//    gui.add(farClipping.set("farClipping", MAX_POSITION, 0, MAX_POSITION));
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
    
    /*
    int iy=220;
    int ix=300;
    short unsigned int depth = cam.getDepth()[iy*columns+ix];
    if (depth ) {
        ofVec3f m(ix,iy,depth);
        ofVec3f p(cam.getWorldCoordinateAt(ix,iy , depth));
        cout << m << "\t" << p*scale << endl;
    }
    */
    
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
//    ofSetupScreenPerspective(1024,768,60,nearClipping,farClipping);
//    ofSetupScreenOrtho(1024,768,-nearClipping,-farClipping);
    ofBackground(0);
    ofSetColor(255);
    
    
    
    if (bUseShader) {
        cloudShader.begin();
        cloudShader.setUniform1f("minEdge", minEdge1);
        cloudShader.setUniform1f("maxEdge",maxEdge1);
        cloudShader.setUniform1f("scale",pow(10,depthScale));

    } else {
        depthTex.draw(0, 0);
    }
    
    float scale = pow(10, gridScale);
    float offset = pow(10, gridOffset);

    ofPushMatrix();

    
    ofRotateX(sceneRotation->x);
	ofRotateY(sceneRotation->y);
	ofRotateZ(sceneRotation->z);
	ofTranslate(ofVec3f(position->x, position->y, position->z));
	ofRotateX(cameraRotation->x);
	ofRotateY(cameraRotation->y);
	ofRotateZ(cameraRotation->z);
    ofScale(scale,scale,scale*offset);
    glPointSize(pointSize);
    ofEnableDepthTest();

    mesh.drawVertices();
    ofDisableDepthTest();
    
    
//    ofMatrix4x4 mat = ofGetCurrentMatrix(OF_MATRIX_MODELVIEW) * ofGetCurrentMatrix(OF_MATRIX_PROJECTION);
//    cout << mat.getTranslation() << endl;
    ofPopMatrix();
    
     /*
    int columns = 640;
    int iy=220;
    int ix=300;
    short unsigned int depth = cam.getDepth()[iy*columns+ix];
    if (depth ) {
        ofVec3f m(ix,iy,depth);
        ofVec3f p(cam.getWorldCoordinateAt(ix,iy , depth));
//        cout << m << "\t" << p << endl;
        cout << mat.postMult(p) << endl;
    }
    */
    
    
    if (bUseShader) {
        cloudShader.end();
    }
    
//    ofSetupScreen();
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
