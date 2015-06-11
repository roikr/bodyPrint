#include "ofApp.h"
#include "Shaders.h"

#define STRINGIFY(A) #A




enum {
    STATE_CAMERA,
    STATE_POINT_PICKING,
    STATE_SCREEN
};

//--------------------------------------------------------------
void ofApp::setup(){
    
    ofDisableArbTex();
    
    
    gui.setup("panel");
    gui.add(fps.set("fps",""));
    gui.add(pointSize.set("pointSize", 1, 1, 10));
    gui.add(minEdge0.set("minEdge0", 0.0, 0.0, 1.0));
    gui.add(maxEdge0.set("maxEdge0", 1.0, 0.0, 1.0));
    gui.add(position.set("position",-0.01));
    gui.add(depthScale.set("depthScale", 0, -5.0, 5.0)); // 10^-5
    gui.add(minEdge1.set("minEdge1", 0.0, 0.0, 1.0));
    gui.add(maxEdge1.set("maxEdge1", 1.0, 0.0, 1.0));
    
 
    gui.loadFromFile("settings.xml");
    
    
    ofFile fileRead("screen.txt");
    fileRead >> screen;
    
    
    ofxOpenNI2::init();
    depthCam.setup();
    depthCam.setDepthMode(5);
   
    depthTex.allocate(depthCam.depthMode.width, depthCam.depthMode.height, GL_R16 );
    createCloudShader(cloudShader);
    
    bUseShader = false;
    
    state = STATE_CAMERA;
    
    

    cam.lookAt(ofVec3f(0,0,1),ofVec3f(0,1,0));
    cam.setPosition(0,0,-0.01);
    cam.setNearClip(0.001);
    
}

void ofApp::updateMesh() {
    
    mesh.clear();
    mesh.setMode(OF_PRIMITIVE_POINTS);
    
    int rows=depthCam.depthMode.height;
    int columns=depthCam.depthMode.width;
    
    int minE = minEdge0*USHRT_MAX;
    int maxE = maxEdge0*USHRT_MAX;
  
    for(int iy = 0; iy < rows; iy++) {
        for(int ix = 0; ix < columns; ix++) {
            short unsigned int depth = depthCam.getDepth()[iy*columns+ix];
            ofVec3f pos = depthCam.getWorldCoordinateAlt(ix, iy, depth);
            if (depth && depth> minE && depth<maxE) {
                mesh.addVertex(pos);
            }
        }
    }
}

//--------------------------------------------------------------
void ofApp::update(){

    fps=ofToString(ofGetFrameRate());
    depthCam.update();
    if (depthCam.bNewDepth) {
        depthTex.loadData(depthCam.getDepth(), depthCam.depthMode.width, depthCam.depthMode.height,GL_RED);
        updateMesh();
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofBackground(0);
    ofSetColor(255);
    
    depthTex.draw(0, 0);
    
    if (bUseShader) {
        cloudShader.begin();
        cloudShader.setUniform1f("minEdge", minEdge1);
        cloudShader.setUniform1f("maxEdge",maxEdge1);
        cloudShader.setUniform1f("scale",pow(10,depthScale));
        
    }
    
    cam.begin();
    if (state==STATE_SCREEN) {
        ofMultMatrix(screen);
    }
    
   
    glPointSize(pointSize);
    ofEnableDepthTest();
    
    mesh.drawVertices();
    ofDisableDepthTest();
    cam.end();
    
    if (bUseShader) {
        cloudShader.end();
        
    }
    
     gui.draw();
    
    
    if (state==STATE_POINT_PICKING && ofGetMousePressed()) {
    
        int n = mesh.getNumVertices();
        float nearestDistance = 0;
        ofVec2f nearestVertex;
        int nearestIndex = 0;
        ofVec2f mouse(mouseX, mouseY);
        for(int i = 0; i < n; i++) {
            ofVec3f ver = mesh.getVertex(i);
            ofVec3f cur = cam.worldToScreen(ver);
            float distance = cur.distance(mouse);
            if(i == 0 || distance < nearestDistance) {
                nearestDistance = distance;
                nearestVertex = cur;
                tempMarker = ver;
                nearestIndex = i;
            }
        }
        
        ofSetColor(ofColor::gray);
        ofLine(nearestVertex, mouse);
        
        ofNoFill();
        ofSetColor(ofColor::yellow);
        ofSetLineWidth(2);
        ofCircle(nearestVertex, 4);
        ofSetLineWidth(1);
        
        ofVec2f offset(10, -10);
        ofDrawBitmapStringHighlight(ofToString(nearestIndex), mouse + offset);
        
    }
    
    if (state==STATE_CAMERA || state==STATE_POINT_PICKING) {
        ofNoFill();
        ofSetColor(ofColor::red);
        ofSetLineWidth(2);
        for (vector<ofVec3f>::iterator iter=markers.begin();iter!=markers.end();iter++) {
            ofCircle(cam.worldToScreen(*iter), 4);
        }
         ofSetLineWidth(1);
    }
}

void ofApp::exit() {
    depthCam.exit();
}

void ofApp::saveScreenMatrix() {
    
    
    ofVec3f origin = markers[0];
    ofVec3f vecX = 2*(origin-markers[1]);
    ofVec3f vecY = origin-markers[2];
    
    ofVec3f lookAtDir = -(vecX.cross(vecY)).normalize();
    
    cout << lookAtDir << endl;
    ofVec3f center = markers[1]-0.5*vecY;
    screen.makeLookAtViewMatrix(center, center+lookAtDir, vecY);
    
    ofFile fileWrite("screen.txt",ofFile::WriteOnly);
    fileWrite << screen;
    
    
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    
    switch (key) {
        case '1':
        case '2':
        case '3':
            if (markers.empty()) {
                markers.assign(3, ofVec3f(0));
            }
            markers[key-'1'] = tempMarker;
            break;
        case 'c':
            state = STATE_CAMERA;
            
            break;
        case 'p':
            state = STATE_POINT_PICKING;
            break;
        case 's':
            state = STATE_SCREEN;
            if (!markers.empty()) {
                saveScreenMatrix();
            }
            cam.setPosition(0,0,position);
           
            break;
        
        case 'd':
            bUseShader =!bUseShader;
            break;
        
        case 'm':
            markers.clear();
            break;
            
        case 't':
            ofToggleFullscreen();
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
    

    if (state==STATE_SCREEN || state==STATE_CAMERA) {

        cam.dolly((lastPos.y-y)/ofGetHeight());
        lastPos=ofVec2f(x,y);
        
        if (state==STATE_SCREEN) {
            position = cam.getPosition().z;
        }
    }
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
    lastPos = downPos = ofVec2f(x,y);
    
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
