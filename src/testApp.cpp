#include "testApp.h"
#include "ofxSimpleGuiToo.h"


bool keys[255];
char oscStatus[255];

float camZoom, camPosX, camPosY, camRotX, camRotY;
ofVec3f centroid;
int pMouseX, pMouseY;

ofxSimpleGuiTitle *status;
ofxSimpleGuiToggle *calibratedButton;
bool bLoadMLP;
int frame;



//--------------------------------------------------------------
void testApp::setup(){
    
    ofBackground(0);
    ofSetFrameRate(60);
    
    int PORT = 12000;
    int REMOTE_PORT = 12000;
    string REMOTE_HOST = "169.254.0.1";
    
	receiver.setup(PORT);
    sender.setup(REMOTE_HOST, REMOTE_PORT);
    
    sprintf(oscStatus, "[LOCALPORT] %d\n[REMOTE PORT] (%s, %5d)\n", PORT, REMOTE_HOST.c_str(), REMOTE_PORT);
    
    gui.setup();
	gui.addTitle("TRACKER \n[i] hide controls");
    gui.addToggle("TRACK", bTracking);
    calibratedButton = &gui.addToggle("CALIBRATED", bCalibrated);
    gui.addToggle("SAVE", bSaving);
    gui.addButton("Load MeshLab File", bLoadMLP);
    gui.addButton("Reset player", bReset);

    gui.addSlider("Zoom", camZoom, -5000, 5000).setSmoothing(0.9);
    gui.addSlider("camPosX", camPosX, -200, 200).setSmoothing(0.9);;
    gui.addSlider("camPosY", camPosY, -200, 200).setSmoothing(0.9);;
    gui.addSlider("camRotX", camRotX, 0, 360).setSmoothing(0.9);;
    gui.addSlider("camRotY", camRotY, 0, 360).setSmoothing(0.9);;
    status = &gui.addTitle("STATUS");
    status->setNewColumn(true);
	gui.loadFromXML();
    gui.show();
    
    camRotX = 90;
    
    matrixData.setup();
    
    // Set space coordinates...
    center = ofVec2f(550, 2900);
    speaker1 = ofVec2f(-400, 4350);
    refVector = center - speaker1;

    for(int i = 0; i < K; i++){
        kinects[i].setMatrix(matrixData.getMatrix(i));
        kinects[i].setCenter(center, refVector);
    }
    
    bTracking = false;
    bCalibrated = false;
    bSaving = false;
    pbSaving = false;
    bReset = false;
    
    
    while(receiver.hasWaitingMessages()){
		ofxOscMessage m;
		receiver.getNextMessage(&m);
    }
    }

//--------------------------------------------------------------
void testApp::update(){
    for(int i = 0; i < K; i++)
        kinects[i].markAsOld();
    
    processOSC();
   
    
    //-------------------------
    
    if(bLoadMLP){
        bLoadMLP = false;
        matrixData.openMLP();
        for(int i = 0; i < K; i++)
            kinects[i].setMatrix(matrixData.getMatrix(i));
    }
    
    //-------------------------
    
    if(bTracking){
        if(!bCalibrated) {
            bCalibrated = match.startTracking(kinects, trackers);
        }
        else{
            for(int i = 0; i < N; i++)
                trackers[i].resetCandidates();
            
            match.matchCandidates(kinects, trackers);
            
            for(int i = 0; i < N; i++)
                trackers[i].match();
        }
    }
    else{
        bCalibrated = false;
    }
    
    //-------------------------
    
    sendDistances();
    sendPositions();
    sendAzimuts();
    if(bReset){
        sendReset();
        bReset = false;
    }
        
    if(bSaving){
        if(!pbSaving){
            frame = 0;
            pbSaving = true;
        }
        sendSaving(frame);
        frame += 1;
    }
    else{
        pbSaving = false;
    }
        
        
    if(ofGetFrameNum() % 30 == 0)
        sendPing();
    
    
    //-------------------------
    
    
    char msg[2048];
    matrixData.getStatus(msg);
    strcat(msg, oscStatus);
    
    char kinectMsg[500];
    for(int i = 0; i < K; i++){
        kinects[i].getStatus(kinectMsg, i);
        strcat(msg, kinectMsg);
    }
    char other[500];
    sprintf(other, "\n[CENTER] %4.f, %4.f", center.x, center.y);
    strcat(msg, other);
    
    
    float delta[3];
    int n = 0;

    for(int i = 0; i < N - 1; i++)
        for(int j = i + 1; j < N; j++){
            delta[n] = trackers[i].lerpedPos.distance(trackers[j].lerpedPos);
            n ++;
        }
    sprintf(other, "\n[DISTANCES] %4.f, %4.f, %4.f", delta[0], delta[1], delta[2]);
    strcat(msg, other);

    
    n = 0;
    for(int i = 0; i < N - 1; i++)
        for(int j = i + 1; j < N; j++){
            delta[n] = trackers[i].lerpedPos.distance(trackers[j].lerpedPos) -
            trackers[i].pLerpedPos.distance(trackers[j].pLerpedPos) ;
            n ++;
        }
    sprintf(other, "\n[DELTA DISTANCES] %4.f, %4.f, %4.f", delta[0], delta[1], delta[2]);
    strcat(msg, other);
    
    int b;
    bool cohesion = true;
    bool separation = true;
    for(int i = 0; i < n; i++){
        cohesion = cohesion && (delta[i] <= -2);
        separation = separation && (delta[i] >= 2);
    }
    if(cohesion) b = 1;
    else if(separation) b = -1;
    else b = 0;
    sprintf(other, "\n[BEHAVIOUR] %d", b);
    strcat(msg, other);

    
    
    n = 0;
    for(int i = 0; i < N; i++){
        delta[n] = abs((trackers[i].lerpedPos - trackers[i].pLerpedPos).length());
        n ++;
    }
    sprintf(other, "\n[SPEED] %4.f, %4.f, %4.f", delta[0], delta[1], delta[2]);
    strcat(msg, other);
    
    
    if(kinects[0].getCOMsize() > 0 && kinects[1].getCOMsize() > 0  ){
        sprintf(other, "\n[COM 0 dist] %4.f", kinects[0].getCOM(0).distance(kinects[1].getCOM(0)));
        strcat(msg, other);
    }
    if(bTracking && bCalibrated){
        for(int i = 0; i < N; i++){
            sprintf(other, "\n[TRACKER %d] %4.f %4.f %4.f", i, trackers[i].pos.x,
                    trackers[i].pos.y, trackers[i].pos.z);
            strcat(msg, other);
            
        }
    }
    
    status->setName(msg);
    status->setSize(300, 400);
    calibratedButton ->setValue(bCalibrated);
    
}

//--------------------------------------------------------------
void testApp::draw(){
    centroid = ofVec3f(0, 0, 4000);
    centroid.x = center.x;
    centroid.z = center.y;
    
	cam.setPosition(ofVec3f(0, 0,-2000));
    cam.lookAt(centroid, ofVec3f(0,1,0));
    cam.setFarClip(50000);
    
    cam.begin();
    
    ofPushMatrix();
    ofTranslate(camPosX, camPosY, camZoom);
    
    pivot(centroid, camRotX, camRotY, 0);
    ofScale(-1.0, 1.0, 1.0);
    drawAxes(centroid);
    
    ofPushStyle();
    
    //-------------------------
    for(int i = 0; i < K; i++)
        kinects[i].draw();
    
    
    if(bCalibrated && bTracking){
        for(int i = 0; i < N; i++)
            trackers[i].draw();
        
        for(int i = 0; i < N - 1; i++)
            for(int j = 1; j < N; j++){
                setLineColor(i + j);
                ofLine(trackers[i].lerpedPos, trackers[j].lerpedPos);
            }
        
        //-------------------------
        ofEnableAlphaBlending();
        ofSetColor(255, 0, 0, 50);
        ofFill();
        ofBeginShape();
        for(int i = 0; i < N; i++)
            ofVertex(trackers[i].lerpedPos);
        ofEndShape();
        ofDisableAlphaBlending();
        //-------------------------
    }
    
    ofPopStyle();
   

    ofPopMatrix();

    cam.end();
    
    gui.draw();    
}

void testApp::keyPressed(int key){
    
    switch (key ) {
        case 'i':
            gui.toggleDraw();
            break;
        case ' ':
            bTracking = !bTracking;
            gui.toggleDraw();
            break;
        case 'z':
            match.swap(trackers, 0, 1);
            break;
        case 'x':
            match.swap(trackers, 0, 2);
            break;
        case 'c':
            match.swap(trackers, 1, 2);
            break;
        default:
            break;
    }
    if(key > 0 && key <255)
        keys[key] = true;
    
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){
   
    if(key > 0 && key <255)
        keys[key] = false;
}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y){

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){
    if(keys['1']) camZoom -=  (mouseX - pMouseX) * 3;
    if(keys['2']) {
        camPosX -=  (mouseX - pMouseX) *2;
        camPosY -=  (mouseY - pMouseY) *2;
    }
    if(keys['3']) {
        camRotY -=  (mouseX - pMouseX) / 10.;
        camRotX -=  (mouseY - pMouseY) / 10.;
    }
    
    pMouseX = mouseX;
    pMouseY = mouseY;

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){
    pMouseX = mouseX;
    pMouseY = mouseY;

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){ 

}

void testApp::processOSC(){
   
	while(receiver.hasWaitingMessages()){
        
        char *data;
		ofxOscMessage m;
        receiver.getNextMessage(&m);
        
        if(m.getAddress() == "/ping"){
            int _k = m.getArgAsInt32(0);
            int port = m.getArgAsInt32(1);
            kinects[_k].setHost(m.getRemoteIp(), port);
        }
		else if(m.getAddress() == "/pc"){
            int _k = m.getArgAsInt32(0);
            int f = m.getArgAsInt32(4);
            kinects[_k].clearMesh(f);
            if(_k < K){
                unsigned long l;
                int id = m.getArgAsInt32(1);
                data = m.getArgAsBlob(2, l);
                int d = m.getArgAsInt32(3);
                for(int i = 0; i < d * 6; i += 6){
                    ofVec3f p;
                    p.x = ((short)data[i + 1] << 8) | ((short)data[i] & 0xFF);
                    p.y = ((short)data[i + 3] << 8) | ((short)data[i + 2] & 0xFF);
                    p.z = ((short)data[i + 5] << 8) | ((short)data[i + 4] & 0xFF);
                    kinects[_k].addPoint(p, id);
                }
            }
            
		}
        
		if(m.getAddress() == "/com"){
            int _k = m.getArgAsInt32(0);
            
            kinects[_k].clearCOM();
            kinects[_k].markAsNew();
            
            string s = m.getArgAsString(1);
            vector<string> tokens =ofSplitString(s, ",");
            
            for (vector<string>::iterator it = tokens.begin(); it!=tokens.end(); ++it) {
                vector<string>comData = ofSplitString(*it, " ");
                if(comData.size() == 5){
                    ofVec3f pos;
                    
                    pos.x = ofToFloat(comData[2]);
                    pos.y = ofToFloat(comData[3]);
                    pos.z = ofToFloat(comData[4]);
                    kinects[_k].addCOM(pos);
                }
            }
            
            if(_k == 0){
                //kinects[_k].addCOM(ofVec3f(-1700, 1000, 3900)); //NUNO
                //kinects[_k].addCOM(ofVec3f(1500, 0, 3700)); //NUNO2
                //kinects[0].addCOM(ofVec3f(0, 0, 1500)); //NUNO3
            }

            

        }
	}
    
}

void testApp::setLineColor(int i){
    switch(i){
        case 1:
            ofSetHexColor(0xFF0000);
            break;
        case 2:
            ofSetHexColor(0x00FF00);
            break;
        case 3:
            ofSetHexColor(0x0000FF);
            break;
        default:
            ofSetHexColor(0xFFFFFF);
            break;
    }
}
void testApp::drawAxes(ofVec3f center){
    ofPushMatrix();
    ofTranslate(center);
    ofPushStyle();
    ofSetColor(255, 0, 0);
    ofLine(0, 0, 0, 200, 0, 0);
    
    ofSetColor(0, 255, 0);
    ofLine(0, 0, 0, 0, 200, 0);
    
    ofSetColor(0, 0, 255);
    ofLine(0, 0, 0, 0, 0, 200);
    ofPopStyle();
    ofPopMatrix();
}

void testApp::pivot(ofVec3f center, float aX, float aY, float aZ){
    ofTranslate(center);
    ofRotateX(aX);
    ofRotateY(aY);
    ofRotateZ(aZ);
    ofTranslate(-center);
}
void testApp::sendPing(){
    ofxOscMessage m;
    m.setAddress("/ping");
    sender.sendMessage(m);
}

void testApp::sendPositions(){
    
    ofxOscMessage m;
    m.setAddress("/speed");
    for(int i = 0; i < N; i++){
        ofVec3f delta = trackers[i].lerpedPos - trackers[i].pLerpedPos;
        m.addFloatArg(abs(delta.length()));
    }
    
    sender.sendMessage(m);
}



void testApp::sendAzimuts() {
    
    ofxOscMessage m;
    m.setAddress("/azimuts");
    for(int i = 0; i < N; i++){
        ofVec2f v(trackers[i].lerpedPos.x - center.x, trackers[i].lerpedPos.z - center.y);
        float angle  = v.angle(-refVector);
        m.addFloatArg(angle);
        m.addFloatArg(v.length());
    }
    sender.sendMessage(m);
    
}

void testApp::sendDistances() {
    
        float delta[N];
        int n = 0;
        for(int i = 0; i < N - 1; i++)
            for(int j = i + 1; j < N; j++){
                delta[n] = trackers[i].lerpedPos.distance(trackers[j].lerpedPos) -
                   trackers[i].pLerpedPos.distance(trackers[j].pLerpedPos) ;
                n ++;
            }
        
    int b;
    
    ofxOscMessage m;
        m.setAddress("/behaviour");
        bool cohesion = true;
        bool separation = true;
        for(int i = 0; i < n; i++){
            cohesion = cohesion && (delta[i] <= -2);
            separation = separation && (delta[i] >= 2);
        }
        if(cohesion) b = 1;
        else if(separation) b = -1;
        else b = 0;
        m.addIntArg(b);
        sender.sendMessage(m);
    
    m.clear();
    m.setAddress("/distances");
    for(int i = 0; i < N - 1; i++)
        for(int j = i + 1; j < N; j++){
            float d = trackers[i].lerpedPos.distance(trackers[j].lerpedPos);
            m.addFloatArg(d);
        }
    sender.sendMessage(m);
}
void testApp::sendSaving(int frame){
    for(int i = 0; i < K; i++)
        kinects[i].sendSaving(frame);
    
}
void testApp::sendReset(){
    for(int i = 0; i < K; i++)
        kinects[i].sendReset();
    
}