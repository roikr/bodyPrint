#include "ofApp.h"
#include "Shaders.h"

#define STAGE_WIDTH 1024
#define STAGE_HEIGHT 768

#define LAYERS_NUMBER 3
#define CAMERAS_NUMBER 1

#define STRINGIFY(A) #A

enum {
    STATE_PICKING,
    STATE_CALIBRATION,
    STATE_BACKGROUND,
    STATE_BLOB,
    STATE_IDLE,
    STATE_RECORD,
    
};


//--------------------------------------------------------------
void ofApp::setup(){
    

    ofDirectory dir;
    dir.allowExt("mov");
    dir.listDir(ofToDataPath("."));
    for (int i=0;i<dir.numFiles();i++) {
        memories.push_back(dir.getName(i));        
    }
    
    recSound.loadSound("camera memory sound 10 sec 48.wav");
    ambientSound.loadSound("ambi2.wav");
    ambientSound.setLoop(true);
    ambientSound.play();
    
    
    //ofSetWindowShape(STAGE_WIDTH, STAGE_HEIGHT);
    
    ofDisableArbTex();
    

    
    for (int i=0;i<CAMERAS_NUMBER;i++) {
        cam[i].params.setName("cam"+ofToString(i));
        cam[i].params.add(cam[i].minEdge0.set("minEdge0", 0.0, 0.0, 1.0));
        cam[i].params.add(cam[i].maxEdge0.set("maxEdge0", 1.0, 0.0, 1.0));
        cam[i].params.add(cam[i].minEdge1.set("minEdge1", 0.0, 0.0, 2.0));
        cam[i].params.add(cam[i].maxEdge1.set("maxEdge1", 2.0, 0.0, 2.0));
    }
    
    ofFile fileRead("cam0.txt");
    fileRead >> cam[0].mat;
    
    gui.setup("panel");
    gui.add(fps.set("fps",""));
    gui.add(ambLevel.set("ambLevel", 0.5, 0.0, 1.0));
    gui.add(recLevel.set("recLevel", 0.5, 0.0, 1.0));
    gui.add(pointSize.set("pointSize",3,1,10));
    gui.add(position.set("position",-0.01,-10,10));
    gui.add(depthScale.set("depthScale", 0, -5.0, 5.0)); // 10^-5

    for (int i=0;i<CAMERAS_NUMBER;i++) {
        gui.add(cam[i].params);
    }
    
        gui.add(tolerance.set("tolerance", 0.1, 0.0, 1.0));
    gui.add(decay0.set("decay0", 0.9, 0.9, 1.0));
    gui.add(decay1.set("decay1", 0.9, 0.9, 1.0));
    gui.add(strobeRate.set("strobeRate", 15, 1, 30));
    gui.add(variance.set("variance", .143,0,10));
    gui.add(radius.set("radius", 7,0,20)); // fps drop above 14
    gui.add(hueRate.set("hueRate", 0.0,0.0,1.0));
    gui.add(sat.set("sat", 0.0,0.0,1.0));
    gui.add(offset.set("offset", 0.0,-0.5,0.5));
    
    gui.add(minArea.set("minArea",0.05,0,0.1));
    gui.add(maxArea.set("maxArea", 0.5, 0, 1));
    gui.add(blobDetection.set("blobDetection",false));
    gui.add(recordTime.set("recordTime",""));
    gui.add(waitTime.set("waitTime",""));
    gui.add(recordDuration.set("recordDuration",10,10,30));
    gui.add(minimumDuration.set("minimumDuration",3,0,5));
//    gui.add(freezeDuration.set("freezeDuration",3,0,5));
    gui.add(waitDuration.set("waitDuration",2,0,10));
    gui.add(idleInterval.set("idleInterval",5,2,10));
    
    gui.add(videoQueue.set("videoQueue",""));
    gui.loadFromFile("settings.xml");
    
    virtualCam.lookAt(ofVec3f(0,0,1),ofVec3f(0,1,0));
    virtualCam.setPosition(0,0,position);
    //virtualCam.setPosition(0,0,-0.01);
    virtualCam.setNearClip(0.001);
    
    recorder.setPixelFormat("gray");
    
#ifdef TARGET_OSX
    
    
    //    recorder.setFfmpegLocation("~/ffmpeg");
    recorder.setVideoCodec("mpeg4");
    recorder.setVideoBitrate("800k");
    
    
   
#else
    
    // recorder.setFfmpegLocation(ofFilePath::getAbsolutePath("avconv"));
    recorder.setFfmpegLocation("avconv");
    
#endif
    ofxOpenNI2::init();
    vector<string> devices = ofxOpenNI2::listDevices();
    
//    if (devices.size()<2) {
//        std:exit();
//    }
    
    for (int i=0;i<CAMERAS_NUMBER;i++) {
        cam[i].sensor.setup(devices[i]);
        cam[i].sensor.setDepthMode(5);
        cam[i].background.allocate(cam[i].sensor.depthMode.width, cam[i].sensor.depthMode.height, 1);
    }
    
    
    createCloudShader(cloudShader);
    
    ofFbo::Settings s;
    s.width = STAGE_WIDTH;
    s.height = STAGE_HEIGHT;
    s.internalformat = GL_R16; // GL_R8 is not used in ofGetImageTypeFromGLType()
    
    depthFbo.allocate(s);
    camFbo.allocate(s);
    camFbo.begin();
    ofClear(0);
    camFbo.end();
    

//    createDepthBackgroundSubtractionShader(subtractShader);
//    subtractShader.begin();
//    subtractShader.setUniformTexture("bgTex", backgroundFbo.getTextureReference(), 1);
//    subtractShader.end();
    
    
    string fragment = STRINGIFY(
                                \n#version 150\n
                                
                                uniform sampler2D tex0;
                                uniform sampler2D memTex;
                                
                                
                                uniform int frameNum;
                                uniform int strobeRate;
                                uniform float decay;
                                uniform float sat;
                                uniform float hue;
                                uniform float offset;
                                
                                in vec2 texCoordVarying;
                                out vec4 fragColor;
                              
                                void main(void) {
                                    float c = texture(tex0,texCoordVarying).r;
                                    vec3 mem = texture(memTex,texCoordVarying).rgb;
                                    
                                    bool f = (frameNum % strobeRate) == 0;
                                    
                                    
                                    float lgt = c+offset;
//                                    float hue = float(frameNum/10 % 256)/255.0;
                                    float x= (1-abs(2*lgt-1))*sat;
                                    
                                    float h = fract(hue);
                                    vec3 y;
                                    y.r = abs(h * 6 - 3) - 1;
                                    y.g = 2 - abs(h * 6 - 2);
                                    y.b = 2 - abs(h * 6 - 4);
                                    
                                    vec3 col = (clamp(y,0,1)-0.5)*x+lgt;
                                    
                                    fragColor = vec4(mix(mem*decay,col,vec3(f && c>0)),1.0);
                                }
                                
                                );
    
    strobeShader.setupShaderFromSource(GL_VERTEX_SHADER, getSimpleVertex());
    strobeShader.setupShaderFromSource(GL_FRAGMENT_SHADER, fragment);
    strobeShader.bindDefaults();
    strobeShader.linkProgram();
    
    ping.allocate(depthFbo.getWidth(),depthFbo.getHeight());
    pong.allocate(depthFbo.getWidth(),depthFbo.getHeight());
    
    for (int i=0;i<LAYERS_NUMBER;i++) {
        layer l;
        l.fbo.allocate(depthFbo.getWidth(),depthFbo.getHeight());
        l.fbo.begin();
        ofClear(0);
        l.fbo.end();
        layers.push_back(l);
        
    }
    
    camLayer.fbo.allocate(depthFbo.getWidth(),depthFbo.getHeight());
    camLayer.fbo.begin();
    ofClear(0);
    camLayer.fbo.end();
    camLayer.hueOffset=0;
    
    compFbo.allocate(depthFbo.getWidth(),depthFbo.getHeight());
    compFbo.begin();
    ofClear(0);
    compFbo.end();
    
    
    createBlurShader(blurShader, radius, variance);
    
    grayImg.allocate(depthFbo.getWidth(), depthFbo.getHeight());
    
    ofVideoPlayer p;
    players.assign(LAYERS_NUMBER,p);
    
    string compFragment = STRINGIFY(
                                \n#version 150\n
                                uniform sampler2D tex0;
                                uniform sampler2D tex1;
                                uniform sampler2D tex2;
                                uniform sampler2D tex3;
                                uniform sampler2D tex4;
                                
                                in vec2 texCoordVarying;
                                out vec4 fragColor;
                                
                                void main(void) {
                                    vec3 col0 = texture(tex0,texCoordVarying).rgb;
                                    vec3 col1 = texture(tex1,texCoordVarying).rgb;
                                    vec3 col2 = texture(tex2,texCoordVarying).rgb;
                                    vec3 col3 = texture(tex3,texCoordVarying).rgb;
                                    vec3 col4 = texture(tex4,texCoordVarying).rgb;
                                    
                                    vec3 color = 1-(1-col0)*(1-col1)*(1-col2)*(1-col3)*(1-col4);
                                    fragColor = vec4(color,1.0);
                                }
                                
                                );
    
    
    
    compShader.setupShaderFromSource(GL_VERTEX_SHADER, getSimpleVertex());
    compShader.setupShaderFromSource(GL_FRAGMENT_SHADER, compFragment);
    compShader.bindDefaults();
    compShader.linkProgram();
    
    compShader.begin();
    
    for (int i=0; i<layers.size();i++) {
        compShader.setUniformTexture("tex"+ofToString(i+1), layers[i].fbo.getTextureReference(), i+3);
    }
    compShader.end();
    frameNum = 0;
    
    ofSetColor(255);
    
    state = STATE_PICKING; //STATE_IDLE
    bShowGui = false;
    ofHideCursor();
    bFirstIdle = false;
    bCaptureBg = true;
}

void ofApp::updateMesh(camera &cam) {
    
    cam.mesh.clear();
    cam.mesh.setMode(OF_PRIMITIVE_POINTS);
    
    int rows=cam.sensor.depthMode.height;
    int columns=cam.sensor.depthMode.width;
    
    int minE = cam.minEdge0*USHRT_MAX;
    int maxE = cam.maxEdge0*USHRT_MAX;
    int toler = tolerance*USHRT_MAX;
    
    switch (state) {
        case STATE_PICKING:
        case STATE_CALIBRATION:
        case STATE_BLOB:
        case STATE_IDLE:
        case STATE_RECORD:
            for(int iy = 0; iy < rows; iy++) {
                for(int ix = 0; ix < columns; ix++) {
                    short unsigned int depth = cam.sensor.getDepth()[iy*columns+ix];
                    if (depth && depth> minE && depth<maxE) {
                        cam.mesh.addVertex(cam.sensor.getWorldCoordinateAlt(ix, iy, depth));
                        
                    }
                }
            }
            break;
        case STATE_BACKGROUND:
            for(int iy = 0; iy < rows; iy++) {
                for(int ix = 0; ix < columns; ix++) {
                    short unsigned int depth = cam.background.getPixels()[iy*columns+ix];
                    if (depth && depth> minE && depth<maxE) {
                        cam.mesh.addVertex(cam.sensor.getWorldCoordinateAlt(ix, iy, depth));
                        
                    }
                }
            }
            break;
        default:
            for(int iy = 0; iy < rows; iy++) {
                for(int ix = 0; ix < columns; ix++) {
                    short unsigned int ref = cam.background.getPixels()[iy*columns+ix];
                    short unsigned int depth = cam.sensor.getDepth()[iy*columns+ix];
                    if (depth && abs((int)depth-(int)ref)>toler && depth> minE && depth<maxE) {
                        cam.mesh.addVertex(cam.sensor.getWorldCoordinateAlt(ix, iy, depth));
                        
                    }
                }
            }
            break;
    }
}

void ofApp::updateLayer(layer &l,ofFbo &depth,float decay) {
    ping.begin();
    strobeShader.begin();
    strobeShader.setUniformTexture("memTex", l.fbo.getTextureReference(), 2);
    strobeShader.setUniform1f("decay", decay);
    strobeShader.setUniform1i("frameNum", frameNum);
    strobeShader.setUniform1i("strobeRate", strobeRate);
    strobeShader.setUniform1f("hue", l.hueOffset+ofGetElapsedTimef()*hueRate/100);
    strobeShader.setUniform1f("sat", sat);
    strobeShader.setUniform1f("offset", offset);
    depth.draw(0,0);
    strobeShader.end();
    ping.end();
    
    if (frameNum%(strobeRate)==0) {
        
        pong.begin();
        blurShader.begin();
        blurShader.setUniform2f("dir", 1.0/depth.getWidth(), 0);
        ping.draw(0,0);
        blurShader.end();
        pong.end();
        
        l.fbo.begin();
        blurShader.begin();
        blurShader.setUniform2f("dir", 0, 1.0/depth.getHeight());
        pong.draw(0,0);
        blurShader.end();
        l.fbo.end();
    } else {
        l.fbo.begin();
        ping.draw(0,0);
        l.fbo.end();
    }
}


void ofApp::renderCam(camera &cam) {
    ofPushMatrix();
    glPointSize(pointSize);
    ofEnableDepthTest();
    cam.mesh.drawVertices();
    ofDisableDepthTest();
    ofPopMatrix();
    
}

//--------------------------------------------------------------
void ofApp::update(){
    
    ambientSound.setVolume(ambLevel);
    recSound.setVolume(recLevel);
    fps = ofToString(ofGetFrameRate());
    
    for (int i=0;i<CAMERAS_NUMBER;i++) {
        cam[i].sensor.update();
    }
    
#if (CAMERAS_NUMBER==2)
    if (cam[0].sensor.bNewDepth && cam[1].sensor.bNewDepth) {
#else
    if (cam[0].sensor.bNewDepth) {
#endif
        if (bCaptureBg) {
            bCaptureBg = false;
            for (int i=0;i<CAMERAS_NUMBER;i++) {
                cam[i].background.setFromPixels(cam[i].sensor.getDepth(), cam[i].sensor.depthMode.width, cam[i].sensor.depthMode.height, 1);
            }
        }
        
        depthFbo.begin();
        virtualCam.begin();
        ofClear(0);
        cloudShader.begin();
        cloudShader.setUniform1f("scale",pow(10,depthScale));
        
        for (int i=0; i<CAMERAS_NUMBER; i++) {
            
            cloudShader.setUniform1f("minEdge", cam[i].minEdge1);
            cloudShader.setUniform1f("maxEdge",cam[i].maxEdge1);
            ofPushMatrix();
            ofMultMatrix(cam[i].mat);
            updateMesh(cam[i]);
            renderCam(cam[i]);
            ofPopMatrix();
            
        }
        cloudShader.end();
        virtualCam.end();
        depthFbo.end();
        
//        blobFbo.begin();
//        subtractShader.begin();
//        subtractShader.setUniform1f("tolerance", tolerance);
//        depthFbo.draw(0, 0);
//        subtractShader.end();
//        blobFbo.end();
        
        ofPixels pixels;
        depthFbo.readToPixels(pixels);
        grayImg.setFromPixels(pixels);
        
        float size = grayImg.getWidth()*grayImg.getHeight();
        contour.findContours(grayImg, minArea*size, maxArea*size, 10, false);
        
        blobDetection = contour.nBlobs && contour.blobs[0].area>=minArea;
        
        if (blobDetection) {
            waitTimer = ofGetElapsedTimef()+ waitDuration;
        }
        
        recordTime ="";
        waitTime="";

        
        switch (state) {
            case STATE_IDLE:
                
                updateLayer(camLayer,camFbo,decay1);
                
                if (bFirstIdle && ofGetElapsedTimef()>idleTimer) {
                    bFirstIdle = false;
                }
                if (blobDetection && !bFirstIdle) {
                    if (!recorder.isInitialized()) {
                        for (vector<ofVideoPlayer>::iterator iter=players.begin();iter!=players.end();iter++) {
                            iter->setPaused(true);
                        }
                        
                        filename = "testMovie"+ofGetTimestampString()+".mov";
                        recorder.setup(filename, depthFbo.getWidth(), depthFbo.getHeight(), 30);
                        recSound.play();
                        recordTimer = ofGetElapsedTimef()+recordDuration;
                        
                        state = STATE_RECORD;
                        
                    }
                }
                break;

                
            case STATE_RECORD: {
                
                recordTime = ofToString(recordTimer-ofGetElapsedTimef());
                waitTime = ofToString(waitTimer-ofGetElapsedTimef());
                
                
//                if (recordTimer-ofGetElapsedTimef()>freezeDuration) {
//                    recordFbo.begin();
//                    depthFbo.draw(0,0);
//                    recordFbo.end();
//                }
                
                
                camFbo.begin();
                depthFbo.draw(0,0);
                camFbo.end();
                
                updateLayer(camLayer,camFbo,decay0);
                
                
                
                recorder.addFrame(pixels);
                
                videoQueue = ofToString(recorder.getVideoQueueSize());
                if (ofGetElapsedTimef()>recordTimer || ofGetElapsedTimef()>waitTimer) {
                    recorder.close();
                    
                    state = STATE_IDLE;
                    
                    camFbo.begin();
                    ofClear(0);
                    camFbo.end();
                    
                    float duration = ofGetElapsedTimef()-(recordTimer-recordDuration);
                    if (duration > minimumDuration) {
                    
                        memories.push_back(filename);
                        
                        players.front().loadMovie(filename);
                        players.front().setLoopState(OF_LOOP_NONE);
                        players.front().play();
                        
                        idleTimer = ofGetElapsedTimef()+idleInterval;
                        bFirstIdle = true;
                    } else {
                        idleTimer = ofGetElapsedTimef();
                    }
                
                }
            }
                break;
                
            default:
                break;
        }
        
        
        frameNum++;
        
    }
    
    
    
    if (state==STATE_IDLE) {
        
        
        
        if (!memories.empty()) {
            for (vector<ofVideoPlayer>::iterator iter=players.begin();iter!=players.end();iter++) {
                if (!iter->isLoaded()) {
                    iter->loadMovie(memories[(int)ofRandom(10000) % memories.size()]);
                    iter->setLoopState(OF_LOOP_NONE);
                    iter->play();
                    iter->setPaused(true);
                    layers[distance(players.begin(), iter)].hueOffset = ofRandomuf();
                }
            }
            
        }
        
        if (ofGetElapsedTimef()>idleTimer) {
            idleTimer = ofGetElapsedTimef() + idleInterval;
            for (vector<ofVideoPlayer>::iterator iter=players.begin();iter!=players.end();iter++) {
                if (iter->isLoaded() && iter->isPaused()) {
                    iter->setPaused(false);
                    break;
                }
            }
            
        }
       
        
        for (int i=0;i<LAYERS_NUMBER;i++) {
            
            players[i].update();
            
            
            if (players[i].getIsMovieDone()) {
                players[i].close();
                
            }
            
        }
       
    }
    
    if (state==STATE_IDLE || state == STATE_RECORD) {
        for (int i=0;i<LAYERS_NUMBER;i++) {
            
            if (players[i].isLoaded() && !players[i].isPaused()) {
                depthFbo.begin();
                players[i].draw(0, 0);
                depthFbo.end();
                updateLayer(layers[i], depthFbo,decay0);
                
            } else {
                depthFbo.begin();
                ofClear(0);
                depthFbo.end();
                updateLayer(layers[i], depthFbo,decay1);
            }
            
            
            
        }
        
        compFbo.begin();
        compShader.begin();
        for (int i=0; i<layers.size();i++) {
            compShader.setUniformTexture("tex"+ofToString(i+1), layers[i].fbo.getTextureReference(), i+3);
        }
        camLayer.fbo.draw(0, 0);
        compShader.end();
        compFbo.end();
    }
    
}

//--------------------------------------------------------------
void ofApp::draw(){
    
    ofBackground(0);
    ofSetColor(255);
    
    
    switch (state) {
        case STATE_PICKING:
            virtualCam.begin();
            for (int i=0; i<CAMERAS_NUMBER; i++) {
                renderCam(cam[i]);
            }
            virtualCam.end();
            break;
            
        case STATE_CALIBRATION:
        case STATE_BACKGROUND:
            virtualCam.begin();
            for (int i=0; i<CAMERAS_NUMBER; i++) {
                ofMultMatrix(cam[i].mat);
                renderCam(cam[i]);
            }
            virtualCam.end();
            break;
        case STATE_BLOB:
            depthFbo.draw(0, 0);
            for (int i = 0; i < contour.nBlobs; i++){
                contour.blobs[i].draw(0,0);
            }
            ofSetColor(255);
            break;
        
        
            
        default:
            
            compFbo.draw(0, 0);
            break;
    }
    
    
    
//    ofPopMatrix();
    if (bShowGui) {
        gui.draw();
    }
    
    
    
    if (state==STATE_PICKING && (ofGetMousePressed(OF_MOUSE_BUTTON_LEFT) || ofGetMousePressed(OF_MOUSE_BUTTON_RIGHT))) {
        int index = ofGetMousePressed(OF_MOUSE_BUTTON_LEFT) ? 0 : 1;
        int n = cam[index].mesh.getNumVertices();
        float nearestDistance = 0;
        ofVec2f nearestVertex;
        int nearestIndex = 0;
        ofVec2f mouse(mouseX, mouseY);
        for(int i = 0; i < n; i++) {
            ofVec3f ver = cam[index].mesh.getVertex(i);
            ofVec3f cur = virtualCam.worldToScreen(ver);
            float distance = cur.distance(mouse);
            if(i == 0 || distance < nearestDistance) {
                nearestDistance = distance;
                nearestVertex = cur;
                cam[index].tempMarker = ver;
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
    
    if (state==STATE_PICKING) {
        ofNoFill();
        
        ofSetLineWidth(2);
        for (int i=0;i<CAMERAS_NUMBER;i++) {
            ofSetColor(ofColor::red);
            ofCircle(virtualCam.worldToScreen(cam[i].tempMarker), 4);
            ofSetColor(ofColor::green);
            for (vector<ofVec3f>::iterator iter=cam[i].markers.begin();iter!=cam[i].markers.end();iter++) {
                ofCircle(virtualCam.worldToScreen(*iter), 4);
            }
            

        }
        ofSetLineWidth(1);
    }
}

void ofApp::exit() {
    for (int i=0;i<CAMERAS_NUMBER;i++) {
        cam[i].sensor.exit();
    }
    for (vector<ofVideoPlayer>::iterator iter=players.begin();iter!=players.end();iter++) {
        iter->close();
    }
    
}

void ofApp::saveScreenMatrix(int index,bool bInverse) {
    
    camera &thecam = cam[index];
    ofVec3f origin = thecam.markers[0];
    ofVec3f vecX = 2*(origin-thecam.markers[1]);
    ofVec3f vecY = origin-thecam.markers[2];
    
    ofVec3f lookAtDir = (vecX.cross(vecY)).normalize()*(bInverse ? -1 : 1);
    
    cout << lookAtDir << endl;
    ofVec3f center = thecam.markers[1]-0.5*vecY;
    thecam.mat.makeLookAtViewMatrix(center, center+lookAtDir, vecY);
    
    ofFile fileWrite("cam"+ofToString(index)+".txt",ofFile::WriteOnly);
    fileWrite << thecam.mat;
    
    
}

    
//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    switch (key) {
        case ' ':
            bShowGui=!bShowGui;
            if (bShowGui) {
                ofShowCursor();
            } else {
                ofHideCursor();
            }
            break;
        case '1':
        case '2':
        case '3':
            if (cam[0].markers.empty()) {
                cam[0].markers.assign(3, ofVec3f(0));
            }
            cam[0].markers[key-'1'] = cam[0].tempMarker;
            break;
        case '4':
        case '5':
            if (!cam[0].markers.empty()) {
                saveScreenMatrix(0,key=='4');
            }
            break;
        case '6':
        case '7':
        case '8':
            if (cam[1].markers.empty()) {
                cam[1].markers.assign(3, ofVec3f(0));
            }
            cam[1].markers[key-'6'] = cam[1].tempMarker;
            break;
        case '9':
        case '0':
            if (!cam[1].markers.empty()) {
                saveScreenMatrix(1,key=='9');
            }
            break;
        case 'm':
            cam[0].markers.clear();
            cam[1].markers.clear();
            break;
        case 'b':
            state = STATE_BLOB;
            break;
        case 'p':
            state = STATE_PICKING;
            break;
        case 'g':
            state = STATE_BACKGROUND;
            
            break;
        case 'c':
            state = STATE_CALIBRATION;
            break;
        
            
        case 'i':
            state = STATE_IDLE;
            
            break;
            
        case 't':
            ofToggleFullscreen();
            break;
            
        case 'u':
            createBlurShader(blurShader, radius, variance);
            break;
            
        case 's':
            bCaptureBg = true;
           
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
    if ((state==STATE_PICKING || state==STATE_CALIBRATION) && button==OF_MOUSE_BUTTON_MIDDLE) {
        virtualCam.setPosition(ofVec3f(0,0,downPos.z+(downPos.y-y)/ofGetHeight()));
    }
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
    downPos= ofVec3f(x,y,virtualCam.getPosition().z);
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
    if ((state==STATE_PICKING || state==STATE_CALIBRATION) && button==OF_MOUSE_BUTTON_MIDDLE) {
        position = virtualCam.getPosition().z;
    }
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
    float scale = MIN((float)w/STAGE_WIDTH,(float)h/STAGE_HEIGHT);
    mat.makeIdentityMatrix();
    mat.scale(scale, scale, 1.0);
    mat.translate(0.5*(ofVec2f(w,h)-scale*ofVec2f(STAGE_WIDTH,STAGE_HEIGHT)));
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}



