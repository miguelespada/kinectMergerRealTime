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
    
    int PORT = 12005;
    int REMOTE_PORT = 12003;
    string REMOTE_HOST = "169.254.0.1";
    
	receiver.setup(PORT);
    sender.setup(REMOTE_HOST, REMOTE_PORT);
    
    sprintf(oscStatus, "[LOCALPORT] %d\n[REMOTE PORT] (%s, %5d)\n", PORT, REMOTE_HOST.c_str(), REMOTE_PORT);
    
    gui.setup();
	gui.addTitle("TRACKER \n[i] hide controls");
    gui.addToggle("TRACK", bTracking);
    calibratedButton = &gui.addToggle("CALIBRATED", bCalibrated);
    gui.addToggle("SAVE", bSaving);
    status = &gui.addTitle("STATUS");
    gui.addButton("Load MeshLab File", bLoadMLP);
    gui.addSlider("Zoom", camZoom, -5000, 5000).setSmoothing(0.9);
    gui.addSlider("camPosX", camPosX, -200, 200).setSmoothing(0.9);;
    gui.addSlider("camPosY", camPosY, -200, 200).setSmoothing(0.9);;
    gui.addSlider("camRotX", camRotX, 0, 360).setSmoothing(0.9);;
    gui.addSlider("camRotY", camRotY, 0, 360).setSmoothing(0.9);;
    
	gui.loadFromXML();
    gui.show();
    
    for(int i = 0; i < K; i++)
        kinects[i].setMatrix(matrixData.getMatrix(i));
    
   
    
    bTracking = false;
    bCalibrated = false;
    bSaving = false;
    pbSaving = false;
    
    matrixData.setup();
    
    while(receiver.hasWaitingMessages()){
		ofxOscMessage m;
		receiver.getNextMessage(&m);
        cout << "-";
    }
}

//--------------------------------------------------------------
void testApp::update(){
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
    
    status->setName(msg);
    status->setSize(300, 200);
    calibratedButton ->setValue(bCalibrated);
    
}

//--------------------------------------------------------------
void testApp::draw(){
    centroid = ofVec3f(0, 0, 4000);
    
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
		if(m.getAddress() == "/pc"){
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
            
            kinects[_k].setHost(m.getRemoteIp(), m.getRemotePort());
            kinects[_k].clearCOM();
            
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
    m.setAddress("/positions");
    for(int i = 0; i < N; i++){
        m.addFloatArg(trackers[i].lerpedPos.x);
        m.addFloatArg(trackers[i].lerpedPos.y);
        m.addFloatArg(trackers[i].lerpedPos.z);
    }
    
    sender.sendMessage(m);
}

void testApp::sendDistances() {
    
    ofxOscMessage m;
    m.setAddress("/distances");
    for(int i = 0; i < N - 1; i++)
        for(int j = 1; j < N; j++){
            m.addFloatArg(trackers[i].lerpedPos.distance(trackers[j].lerpedPos));
        }
    sender.sendMessage(m);
}
void testApp::sendSaving(int frame){
    for(int i = 0; i < K; i++)
        kinects[i].sendSaving(frame);
    
}
