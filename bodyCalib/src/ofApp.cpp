#include "ofApp.h"
#include "Shaders.h"

#define STRINGIFY(A) #A
#define POINT_CLOUD_SCALE 500.0
#define SHADER_DEPTH_SCALE 0.5/POINT_CLOUD_SCALE


enum {
    STATE_CAMERA,
    STATE_POINT_PICKING,
    STATE_SCREEN
};

//--------------------------------------------------------------
void ofApp::setup(){
    
    ofDisableArbTex();
//    ofDisableSetupScreen();
    
    
    gui.setup("panel");
    gui.add(fps.set("fps",""));
    gui.add(pointSize.set("pointSize", 1, 1, 10));
    gui.add(minEdge0.set("minEdge0", 0.0, 0.0, 1.0));
    gui.add(maxEdge0.set("maxEdge0", 1.0, 0.0, 1.0));
    gui.add(minEdge1.set("minEdge1", 0.0, 0.0, 1.0));
    gui.add(maxEdge1.set("maxEdge1", 1.0, 0.0, 1.0));
//    gui.add(nearClipping.set("nearClipping", 0, 0, MAX_POSITION));
//    gui.add(farClipping.set("farClipping", MAX_POSITION, 0, MAX_POSITION));
    //gui.add(position.set("position", ofVec3f(0), ofVec3f(-MAX_POSITION), ofVec3f(MAX_POSITION)));
    //gui.add(cameraRotation.set("cameraRotation", ofVec3f(0), ofVec3f(-180), ofVec3f(180)));
    //gui.add(sceneRotation.set("sceneRotation", ofVec3f(0), ofVec3f(-180), ofVec3f(180)));
    
    gui.loadFromFile("settings.xml");
    
    
    ofxOpenNI2::init();
    cam.setup();
    cam.setDepthMode(5);
   
    depthTex.allocate(cam.depthMode.width, cam.depthMode.height, GL_R16 );
    createCloudShader(cloudShader);
    
    bUseShader = false;
    
    state = STATE_CAMERA;
    markers.assign(3, ofVec3f(0));
    
}

void ofApp::updateMesh() {
    
    mesh.clear();
    mesh.setMode(OF_PRIMITIVE_POINTS);
    
    int rows=cam.depthMode.height;
    int columns=cam.depthMode.width;
    
    int minE = minEdge0*USHRT_MAX;
    int maxE = maxEdge0*USHRT_MAX;
  
    for(int iy = 0; iy < rows; iy++) {
        for(int ix = 0; ix < columns; ix++) {
            short unsigned int depth = cam.getDepth()[iy*columns+ix];
            ofVec3f pos = cam.getWorldCoordinateAlt(ix, iy, depth);
            if (depth && depth> minE && depth<maxE) {
                mesh.addVertex(pos);
            }
        }
    }
}

//--------------------------------------------------------------
void ofApp::update(){

    fps=ofToString(ofGetFrameRate());
    cam.update();
    if (cam.bNewDepth) {
        depthTex.loadData(cam.getDepth(), cam.depthMode.width, cam.depthMode.height,GL_RED);
        updateMesh();
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
//    ofSetupScreenPerspective(1024,768,60,nearClipping,farClipping);
//    ofSetupScreenOrtho(1024,768,-nearClipping,-farClipping);
    ofBackground(0);
    ofSetColor(255);
    
    depthTex.draw(0, 0);
    
    if (bUseShader) {
        cloudShader.begin();
        cloudShader.setUniform1f("minEdge", minEdge1);
        cloudShader.setUniform1f("maxEdge",maxEdge1);
        cloudShader.setUniform1f("scale",1.0/POINT_CLOUD_SCALE);
        
    }
    
    switch (state) {
        case STATE_CAMERA:
        case STATE_POINT_PICKING:
            easy.begin();
            break;
        case STATE_SCREEN:
            screen.begin();
            break;
        default:
            break;
    }
    
    
    ofMultMatrix(xform);
     ofScale(POINT_CLOUD_SCALE,POINT_CLOUD_SCALE,POINT_CLOUD_SCALE);
    glPointSize(pointSize);
    ofEnableDepthTest();
    
    mesh.drawVertices();
    ofDisableDepthTest();
    
//    ofMatrix4x4 mat = ofGetCurrentMatrix(OF_MATRIX_MODELVIEW) * ofGetCurrentMatrix(OF_MATRIX_PROJECTION);
//    cout << mat.getTranslation() << endl;
    switch (state) {
        case STATE_CAMERA:
        case STATE_POINT_PICKING:
            easy.end();
            break;
        case STATE_SCREEN:
            screen.end();
            break;
            
        default:
            break;
    }
    
    if (bUseShader) {
        cloudShader.end();
        
    }
    
   
    //    ofSetupScreen();
    gui.draw();
    
    
    if (state==STATE_POINT_PICKING && ofGetMousePressed()) {
    
        int n = mesh.getNumVertices();
        float nearestDistance = 0;
        ofVec2f nearestVertex;
        int nearestIndex = 0;
        ofVec2f mouse(mouseX, mouseY);
        for(int i = 0; i < n; i++) {
            //ofVec3f ver = xform.preMult(mesh.getVertex(i)*POINT_CLOUD_SCALE);
            ofVec3f ver = mesh.getVertex(i)*POINT_CLOUD_SCALE;
            ofVec3f cur = easy.worldToScreen(ver);
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
            ofCircle(easy.worldToScreen(*iter), 4);
        }
         ofSetLineWidth(1);
    }
}

void ofApp::exit() {
    cam.exit();
}

void ofApp::setScreenState() {
    state = STATE_SCREEN;
    
    ofVec3f origin = markers[0];
    ofVec3f vecX = 2*(origin-markers[1]);
    ofVec3f vecY = origin-markers[2];
    
    ofVec3f lookAtDir = (vecX.cross(vecY)).normalize();
    
    cout << lookAtDir << endl;
    ofVec3f center = markers[1]-0.5*vecY;
    
    screen.setPosition(center-0.5*lookAtDir*POINT_CLOUD_SCALE);
    screen.lookAt(center,vecY);
    screen.enableOrtho();
    
    
    
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    
    switch (key) {
        case '1':
        case '2':
        case '3':
            markers[key-'1'] = tempMarker;
            break;
        case 'c':
            state = STATE_CAMERA;
            break;
        case 'p':
            state = STATE_POINT_PICKING;
            break;
        case 's':
            setScreenState();
            break;
        
        case 'd':
            bUseShader =!bUseShader;
            break;
        
        case 'm':
            markers.push_back(tempMarker);
            break;
            
        case 't':
            ofToggleFullscreen();
            break;
        default:
            break;
    }
    
    if (state==STATE_CAMERA) {
        easy.enableMouseInput();
    } else {
        easy.disableMouseInput();
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
    
//    switch (state) {
//        case STATE_CLOUD_ROTATION:
//            
//            
//            xform = ofMatrix4x4::newTranslationMatrix(-pivot);
//            xform.rotate(360*(x-downPos.x)/ofGetWidth(), 0, 1, 0);
//            xform.rotate(360*(downPos.y-y)/ofGetHeight(), 1, 0, 0);
//            xform.translate(pivot);
//            xform.preMult(cform);
//            
//            break;
//    }
    if (state==STATE_SCREEN) {
//        screen.truck(x-downPos.x);
//        screen.boom(y-downPos.y);
        screen.dolly(downPos.y-y);
        
    }
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
    downPos = ofVec2f(x,y);
    
    
//    if (state==STATE_CLOUD_ROTATION) {
//        cform = xform;
//        pivot = tempMarker;
//    }
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
