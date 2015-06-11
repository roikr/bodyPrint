#include "ofApp.h"
#include "Shaders.h"

#define STRINGIFY(A) #A
#define POINT_CLOUD_SCALE 500.0


enum {
    STATE_CAMERA,
    STATE_CLOUD,
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
    gui.add(depthScale.set("depthScale", 0, -10.0, 0.0)); // 
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
}

void ofApp::updateMesh() {
    
    mesh.clear();
    mesh.setMode(OF_PRIMITIVE_POINTS);
    
    int rows=cam.depthMode.height;
    int columns=cam.depthMode.width;
    
    int minE = minEdge0*USHRT_MAX;
    int maxE = maxEdge0*USHRT_MAX;
    
//    int minD=USHRT_MAX;
//    ofVec3f minPos(0);
  
    for(int iy = 0; iy < rows; iy++) {
        for(int ix = 0; ix < columns; ix++) {
            short unsigned int depth = cam.getDepth()[iy*columns+ix];
            
            ofVec3f pos = cam.getWorldCoordinateAlt(ix, iy, depth);
            if (depth && depth> minE && depth<maxE) {
                
                mesh.addVertex(pos);
                
            }
            
//            if (depth && depth<minD) {
//                minD = depth;
//                minPos = pos;
//            }
            
        }
    }
    
//    cout << minD << '\t' << minPos << endl;
    
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
    if (bUseShader) {
        cloudShader.begin();
        cloudShader.setUniform1f("minEdge", minEdge1);
        cloudShader.setUniform1f("maxEdge",maxEdge1);
        cloudShader.setUniform1f("scale",pow(10,depthScale));
        depthTex.draw(0, 0);
        cloudShader.end();
    } else {
        depthTex.draw(0, 0);
    }
    
    easy.begin();
    
    ofMultMatrix(xform);
     ofScale(POINT_CLOUD_SCALE,POINT_CLOUD_SCALE,POINT_CLOUD_SCALE);
    glPointSize(pointSize);
    ofEnableDepthTest();
    
    mesh.drawVertices();
    ofDisableDepthTest();
    
//    ofMatrix4x4 mat = ofGetCurrentMatrix(OF_MATRIX_MODELVIEW) * ofGetCurrentMatrix(OF_MATRIX_PROJECTION);
//    cout << mat.getTranslation() << endl;
    easy.end();
    
   
    //    ofSetupScreen();
    gui.draw();
    
    if (state==STATE_CLOUD) {
    
        int n = mesh.getNumVertices();
        float nearestDistance = 0;
        ofVec2f nearestVertex;
        int nearestIndex = 0;
        ofVec2f mouse(mouseX, mouseY);
        for(int i = 0; i < n; i++) {
            ofVec3f ver = xform.preMult(mesh.getVertex(i)*POINT_CLOUD_SCALE);
            
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
        
        
        if (ofGetMousePressed()) {
            ofSetColor(ofColor::red);
            ofSetLineWidth(2);
            ofCircle(easy.worldToScreen(pivot), 4);
            ofSetLineWidth(1);
        }
    }
}

void ofApp::exit() {
    cam.exit();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    switch (key) {
        case ' ':
            state=(state+1)%2;
            switch (state) {
                case STATE_CAMERA:
                    easy.enableMouseInput();
                    break;
                case STATE_CLOUD:
                case STATE_SCREEN:
                    easy.disableMouseInput();
                    break;
                    
                default:
                    break;
            }
            break;
        case 's':
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
    
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
    
    switch (state) {
        case STATE_CLOUD:
            
            
            xform = ofMatrix4x4::newTranslationMatrix(-pivot);
            xform.rotate(360*(x-downPos.x)/ofGetWidth(), 0, 1, 0);
            xform.rotate(360*(downPos.y-y)/ofGetHeight(), 1, 0, 0);
            xform.translate(pivot);
            xform.preMult(cform);
            
            break;
    }
    
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
    downPos = ofVec2f(x,y);
    
    
    if (state==STATE_CLOUD) {
        cform = xform;
        pivot = tempMarker;
    }
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
