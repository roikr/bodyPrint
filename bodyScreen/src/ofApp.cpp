#include "ofApp.h"
#include "Shaders.h"

#define STAGE_WIDTH 800
#define STAGE_HEIGHT 1080
//#define SCREEN_ASPECT_RATIO 139.5/200.5
#define DEPTH_SCALE 0.5

#define LAYERS_NUMBER 1
#define CAMERAS_NUMBER 2

#define STRINGIFY(A) #A

enum {
    STATE_PICKING,
    STATE_CAMERA,
    STATE_EDGES,
    STATE_VISUAL,
    STATE_BACKGROUND,
    STATE_DETECTION,
    STATE_IDLE,
    STATE_RECORD,
    
};


//--------------------------------------------------------------
void ofApp::setup(){
    cout << "version: "   << glGetString(GL_VERSION) << endl << "vendor: " << glGetString(GL_VENDOR) << endl <<
    "renderer: " << glGetString(GL_RENDERER) << endl;

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
    
    
    windowResized(ofGetWidth(), ofGetHeight());
    
    ofDisableArbTex();
    
    ofColor colors[]= {ofColor::red,ofColor::blue};
    
    for (int i=0;i<CAMERAS_NUMBER;i++) {
        cam[i].params.setName("cam"+ofToString(i));
        cam[i].params.add(cam[i].minEdge0.set("minEdge0", 0.0, 0.0, 1.0));
        cam[i].params.add(cam[i].maxEdge0.set("maxEdge0", 1.0, 0.0, 1.0));
        cam[i].params.add(cam[i].minEdge1.set("minEdge1", 0.0, 0.0, 2.0));
        cam[i].params.add(cam[i].maxEdge1.set("maxEdge1", 3.0, 0.0, 3.0));
        cam[i].params.add(cam[i].offset.set("offset",ofVec3f(0.0),ofVec3f(0.0),ofVec3f(1.0)));
        cam[i].color = colors[i];
        
        ofFile fileRead(cam[i].params.getName()+".txt");
        fileRead >> cam[i].mat;
    }
    
    
    
    
    
    gui.setup("panel");
    gui.add(fps.set("fps",""));
    
    gui.add(camZPos.set("camZPos",-0.01,-10,10));
    gui.add(pointSize.set("pointSize",1,1,5));
    
    gui.add(tolerance.set("tolerance", 0.1, 0.0, 1.0));
    //gui.add(depthScale.set("depthScale", 0, -5.0, 5.0)); // 10^-5

    for (int i=0;i<CAMERAS_NUMBER;i++) {
        gui.add(cam[i].params);
    }
    
    
    
    
    visualParams.setName("visual");
    visualParams.add(decay0.set("decay0", 0.9, 0.9, 1.0));
    visualParams.add(decay1.set("decay1", 0.9, 0.9, 1.0));
    visualParams.add(strobeRate.set("strobeRate", 15, 1, 30));
    visualParams.add(variance.set("variance", .143,0,10));
    visualParams.add(radius.set("radius", 7,0,20)); // fps drop above 14
    visualParams.add(hueRate.set("hueRate", 0.0,0.0,1.0));
    visualParams.add(sat.set("sat", 0.0,0.0,1.0));
    visualParams.add(offset.set("offset", 0.0,-0.5,0.5));
    gui.add(visualParams);
    
    levels.setName("levels");
    levels.add(inputBlack.set("inputBlack",0,0,1));
    levels.add(inputWhite.set("inputWhite",1,0,1));
    levels.add(gamma.set("gamma",1,0.3,3));
    levels.add(outputBlack.set("outputBlack",0,0,1));
    levels.add(outputWhite.set("outputWhite",1,0,1));
    gui.add(levels);
    
    detectionParams.setName("detection");
    detectionParams.add(blobPointSize.set("blobPointSize",3,1,5));
    detectionParams.add(minArea.set("minArea",0.05,0,0.1));
    detectionParams.add(maxArea.set("maxArea", 0.5, 0, 1));
    detectionParams.add(blobDetection.set("blobDetection",false));
    gui.add(detectionParams);
    
    timerParams.setName("timer");
    timerParams.add(recordTime.set("recordTime",""));
    timerParams.add(waitTime.set("waitTime",""));
    timerParams.add(recordDuration.set("recordDuration",10,10,30));
    timerParams.add(minimumDuration.set("minimumDuration",3,0,5));
//    gui.add(freezeDuration.set("freezeDuration",3,0,5));
    timerParams.add(waitDuration.set("waitDuration",2,0,10));
    timerParams.add(idleInterval.set("idleInterval",5,2,10));
    timerParams.add(videoQueue.set("videoQueue",""));
    gui.add(timerParams);
    
    gui.add(ambLevel.set("ambLevel", 0.5, 0.0, 1.0));
    gui.add(recLevel.set("recLevel", 0.5, 0.0, 1.0));
    
    gui.loadFromFile("settings.xml");
    
    virtualCam.lookAt(ofVec3f(0,0,1),ofVec3f(0,1,0));
    virtualCam.setPosition(0,0,camZPos);
    //virtualCam.setPosition(0,0,-0.01);
    virtualCam.setNearClip(0.001);
    
    recorder.setPixelFormat("gray");
    
#ifdef TARGET_OSX
    recorder.setVideoCodec("mpeg4");
    recorder.setVideoBitrate("800k");
#else
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
    cloudShader.begin();
    cloudShader.setUniform1f("scale",DEPTH_SCALE);
    cloudShader.end();
    
    ofFbo::Settings s;
    s.width = STAGE_WIDTH;
    s.height = STAGE_HEIGHT;
    s.internalformat = GL_R16; // GL_R8 is not used in ofGetImageTypeFromGLType()
    
    depthFbo.allocate(s);
    blobFbo.allocate(s);
    camFbo.allocate(s);
    camFbo.begin();
    ofClear(0);
    camFbo.end();
    

    
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
    
    createBlurShader(blurShader, radius, variance);
    
    string screenFragment = STRINGIFY(
                                    \n#version 150\n
                                      
                                    uniform sampler2D tex0;
                                    uniform sampler2D tex1;
                                    uniform float inputBlack;
                                    uniform float inputWhite;
                                    uniform float gamma;
                                    uniform float outputBlack;
                                    uniform float outputWhite;
                                    
                                    in vec2 texCoordVarying;
                                    out vec4 fragColor;
                                    
                                    void main(void) {
                                        vec3 col0 = texture(tex0,texCoordVarying).rgb;
                                        float inValue = (clamp(texture(tex1,texCoordVarying).r,inputBlack,inputWhite)-inputBlack)/(inputWhite-inputBlack);
                                        float outValue = mix(outputBlack,outputWhite,pow(inValue,gamma));
                                        
                                        vec3 color = 1-(1-col0)*(1-vec3(outValue));
                                        fragColor = vec4(color,1.0);
                                    }
                                    
                                    );
    
    
    
    screenShader.setupShaderFromSource(GL_VERTEX_SHADER, getSimpleVertex());
    screenShader.setupShaderFromSource(GL_FRAGMENT_SHADER, screenFragment);
    screenShader.bindDefaults();
    screenShader.linkProgram();
    
    
    grayImg.allocate(depthFbo.getWidth(), depthFbo.getHeight()); // for blob detection
    
    compFbo.allocate(depthFbo.getWidth(),depthFbo.getHeight());
    compFbo.begin();
    ofClear(0);
    compFbo.end();
    
    
    
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
    
    state = STATE_IDLE; //
    bShowGui = false;
    ofHideCursor();
    bShowMouse = false;
    bFirstIdle = false;
    bCaptureBg = true;
    bUseBg = true;
    
    warper.setup();
    warper.load();
    warper.hide();
    bEnableWarper = true;
}

void ofApp::updateMesh(camera &cam) {
    
    cam.mesh.clear();
    cam.mesh.setMode(OF_PRIMITIVE_POINTS);
    
    int rows=cam.sensor.depthMode.height;
    int columns=cam.sensor.depthMode.width;
    
    int minE = cam.minEdge0*USHRT_MAX;
    int maxE = cam.maxEdge0*USHRT_MAX;
    int toler = tolerance*USHRT_MAX;

    for(int iy = 0; iy < rows; iy++) {
        for(int ix = 0; ix < columns; ix++) {
            short unsigned int ref = cam.background.getPixels()[iy*columns+ix];
            short unsigned int depth = cam.sensor.getDepth()[iy*columns+ix];
            if (state == STATE_BACKGROUND) {
                if (ref && ref> minE && ref<maxE) {
                    cam.mesh.addVertex(cam.sensor.getWorldCoordinateAlt(ix,iy, ref));
                }
            } else {
                if (bUseBg) {
                    if (depth && abs((int)depth-(int)ref)>toler && depth> minE && depth<maxE) {
                        cam.mesh.addVertex(cam.sensor.getWorldCoordinateAlt(ix,iy, depth));
                    }
                } else {
                    if (depth && depth> minE && depth<maxE) {
                        cam.mesh.addVertex(cam.sensor.getWorldCoordinateAlt(ix,iy, depth));
                    }
                }
            }
        }
    }
}

void ofApp::updateLayer(layer &l,ofFbo &depth,float decay) {
    ping.begin();
    strobeShader.begin();
    strobeShader.setUniformTexture("memTex", l.fbo.getTextureReference(), 1);
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
        
        ping.begin();
        blurShader.begin();
        blurShader.setUniform2f("dir", 0, 1.0/depth.getHeight());
        pong.draw(0,0);
        blurShader.end();
        
        ping.end();
    }
    
    screenShader.begin();
    screenShader.setUniformTexture("tex1", depth.getTextureReference(), 2);
    screenShader.setUniform1f("inputBlack", inputBlack);
    screenShader.setUniform1f("inputWhite", inputWhite);
    screenShader.setUniform1f("gamma", gamma);
    screenShader.setUniform1f("outputBlack", outputBlack);
    screenShader.setUniform1f("outputWhite", outputWhite);
    l.fbo.begin();
    ping.draw(0, 0);
    l.fbo.end();
    screenShader.end();
}


void ofApp::renderCam(camera &cam,int pointSize) {
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
    
    bool bNewFrame = cam[0].sensor.bNewDepth;
#if (CAMERAS_NUMBER==2)
    bNewFrame = bNewFrame && cam[1].sensor.bNewDepth;
#endif
        
    if (bNewFrame) {
        if (bCaptureBg) {
            bCaptureBg = false;
            for (int i=0;i<CAMERAS_NUMBER;i++) {
                cam[i].background.setFromPixels(cam[i].sensor.getDepth(), cam[i].sensor.depthMode.width, cam[i].sensor.depthMode.height, 1);
            }
        }
        
        blobFbo.begin();
        virtualCam.begin();
        ofClear(0);
        cloudShader.begin();
        
        
        for (int i=0; i<CAMERAS_NUMBER; i++) {
            updateMesh(cam[i]);
            cloudShader.setUniform1f("minEdge", cam[i].minEdge1);
            cloudShader.setUniform1f("maxEdge",cam[i].maxEdge1);
            ofPushMatrix();
            ofTranslate(cam[i].offset);
            ofMultMatrix(cam[i].mat);
            renderCam(cam[i],blobPointSize);
            ofPopMatrix();
            
        }
        cloudShader.end();
        virtualCam.end();
        blobFbo.end();
        
        ofPixels pixels;
        blobFbo.readToPixels(pixels);
        grayImg.setFromPixels(pixels);
        
        float size = grayImg.getWidth()*grayImg.getHeight();
        contour.findContours(grayImg, minArea*size, maxArea*size, 10, false);
        
        blobDetection = contour.nBlobs && contour.blobs[0].area>=minArea;
        
        if (blobDetection) {
            waitTimer = ofGetElapsedTimef()+ waitDuration;
        }
        
        recordTime ="";
        waitTime="";
        
        depthFbo.begin();
        virtualCam.begin();
        ofClear(0);
        cloudShader.begin();

        
        for (int i=0; i<CAMERAS_NUMBER; i++) {
            updateMesh(cam[i]);
            cloudShader.setUniform1f("minEdge", cam[i].minEdge1);
            cloudShader.setUniform1f("maxEdge",cam[i].maxEdge1);
            ofPushMatrix();
            ofTranslate(cam[i].offset);
            ofMultMatrix(cam[i].mat);
            renderCam(cam[i],pointSize);
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
        
        

        
        switch (state) {
            case STATE_VISUAL:
                camFbo.begin();
                depthFbo.draw(0,0);
                camFbo.end();
                updateLayer(camLayer,camFbo,decay1);

                break;
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
        
        videoQueue = ofToString(recorder.getVideoQueueSize());
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
                ofSetColor(cam[i].color);
                renderCam(cam[i],pointSize);
            }
            virtualCam.end();
            ofSetColor(255);
            break;
        case STATE_CAMERA:
        case STATE_EDGES:
        case STATE_BACKGROUND:
            virtualCam.begin();
            for (int i=0; i<CAMERAS_NUMBER; i++) {
                ofSetColor(cam[i].color);
                ofPushMatrix();
                ofTranslate(cam[i].offset);
                ofMultMatrix(cam[i].mat);
                renderCam(cam[i],pointSize);
                ofPopMatrix();
            }
            virtualCam.end();
            ofSetColor(255);
            break;
        case STATE_DETECTION:
            blobFbo.draw(0, 0);
            for (int i = 0; i < contour.nBlobs; i++){
                contour.blobs[i].draw(0,0);
            }
            ofSetColor(255);
            break;
        
        
        case STATE_VISUAL:
            ofPushMatrix();
            if (bEnableWarper) {
                ofMultMatrix(warper.getMatrix());
                
            }
            ofMultMatrix(mat);
            camLayer.fbo.draw(0, 0);
            ofPopMatrix();
            break;
        default:
            ofPushMatrix();
            if (bEnableWarper) {
                ofMultMatrix(warper.getMatrix());
                
            }
            ofMultMatrix(mat);
            compFbo.draw(0, 0);
            ofPopMatrix();
            break;
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
    
    warper.draw();
    
    if (bShowGui) {
        gui.draw();
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

void ofApp::saveScreenMatrix(camera &cam) {
    
    
    ofVec3f origin = cam.markers[0];
    ofVec3f vecX = 2*(origin-cam.markers[1])*(cam.bFlipX ? -1 : 1);
    ofVec3f vecY = origin-cam.markers[2]*(cam.bFlipY ? -1 : 1);
    
    ofVec3f lookAtDir = (vecX.cross(vecY)).normalize()*(cam.bFlipZ ? -1 : 1);
    
    cout << lookAtDir << endl;
    ofVec3f center = cam.markers[1]-0.5*vecY;
    cam.mat.makeLookAtViewMatrix(center, center+lookAtDir, vecY);
    
    ofFile fileWrite(cam.params.getName()+".txt",ofFile::WriteOnly);
    fileWrite << cam.mat;
    
    
}

    
//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    
    if (ofGetMousePressed(OF_MOUSE_BUTTON_LEFT) || ofGetMousePressed(OF_MOUSE_BUTTON_RIGHT)) {
        int index = ofGetMousePressed(OF_MOUSE_BUTTON_LEFT) ? 0 : 1;
        
        switch (key) {
            case '1':
            case '2':
            case '3':
                
                if (cam[index].markers.empty()) {
                    cam[index].markers.assign(3, ofVec3f(0));
                }
                cam[index].markers[key-'1'] = cam[index].tempMarker;
                break;
            
            case 'm':
                if (!cam[index].markers.empty()) {
                    saveScreenMatrix(cam[index]);
                }
                break;
            case 'c':
                //cam[index].markers.clear();
                
                break;
            case 'x':
                cam[index].bFlipX=!cam[index].bFlipX;
                if (!cam[index].markers.empty()) {
                    saveScreenMatrix(cam[index]);
                }
                break;
            case 'y':
                cam[index].bFlipY=!cam[index].bFlipY;
                if (!cam[index].markers.empty()) {
                    saveScreenMatrix(cam[index]);
                }
                break;
            case 'z':
                cam[index].bFlipZ=!cam[index].bFlipZ;
                if (!cam[index].markers.empty()) {
                    saveScreenMatrix(cam[index]);
                }
                break;
                
            default:
                break;
        }
    }
    
    if (ofGetMousePressed()) {
        switch (key) {
            case 'g':
                bCaptureBg = true;
                break;
            case 'b':
                createBlurShader(blurShader, radius, variance);
                break;
            case 's':
                warper.save();
                break;
            case 'w': {
                
                int w = STAGE_WIDTH;
                int h = STAGE_HEIGHT;
                int x = (ofGetWidth() - w) * 0.5;       // center on screen.
                int y = (ofGetHeight() - h) * 0.5;     // center on screen.
                
                
                warper.setSourceRect(ofRectangle(x, y, w,h ));              // this is the source rectangle which is the size of the image and located at ( 0, 0 )
                warper.setTopLeftCornerPosition(ofPoint(x, y));             // this is position of the quad warp corners, centering the image on the screen.
                warper.setTopRightCornerPosition(ofPoint(x + w, y));        // this is position of the quad warp corners, centering the image on the screen.
                warper.setBottomLeftCornerPosition(ofPoint(x, y + h));      // this is position of the quad warp corners, centering the image on the screen.
                warper.setBottomRightCornerPosition(ofPoint(x + w, y + h)); // this is position of the quad warp corners, centering the image on the screen.
            }  break;
            default:
                break;
        }
    } else {
    
        switch (key) {
            case ' ':
                bShowGui=!bShowGui;
                break;
            case 'm':
                bShowMouse=!bShowMouse;
                if (bShowMouse) {
                    ofShowCursor();
                } else {
                    ofHideCursor();
                }
                break;
            
            case 'b':
                bUseBg = !bUseBg;
                break;
            case 'd':
                state = STATE_DETECTION;
                break;
            case 'p':
                state = STATE_PICKING;
                break;
            case 'g':
                state = STATE_BACKGROUND;
                break;
            case 'c':
                state = STATE_CAMERA;
                break;
            case 'e':
                state = STATE_EDGES;
                break;
            case 'v':
                state = STATE_VISUAL;
                break;
            case 'i':
                state = STATE_IDLE;
                break;
            case 't':
                ofToggleFullscreen();
                break;
            case 'w':
                bEnableWarper =!bEnableWarper;
                break;
            case 'q':
                warper.toggleShow();
                break;
            default:
                break;
        }
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
    if (state==STATE_CAMERA) {
        switch (button) {
            case OF_MOUSE_BUTTON_LEFT:
                cam[0].offset = cam[0].downOffset+(downPos-ofVec2f(x,y))/ofGetHeight();
                break;
            case OF_MOUSE_BUTTON_RIGHT:
#if (CAMERAS_NUMBER == 2)
                cam[1].offset = cam[1].downOffset+(downPos-ofVec2f(x,y))/ofGetHeight();
#endif
                break;
                
                
            default:
                break;
        }
    }
    
    if ((state==STATE_CAMERA || state==STATE_PICKING) && button==OF_MOUSE_BUTTON_MIDDLE) {
        virtualCam.setPosition(ofVec3f(0,0,downCamZPos+(downPos.y-y)/ofGetHeight()));
        camZPos = virtualCam.getPosition().z;
    }

    
    
   
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
    downPos= ofVec2f(x,y);
    downCamZPos = virtualCam.getPosition().z;
    for (int i=0;i<CAMERAS_NUMBER;i++) {
        cam[i].downOffset = cam[i].offset;
    }
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
    
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



