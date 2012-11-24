#include "testApp.h"
#include "ofxSimpleGuiToo.h"


bool keys[255];
char oscStatus[255];

float camZoom, camPosX, camPosY, camRotX, camRotY;
ofVec3f centroid;
int pMouseX, pMouseY;

ofxSimpleGuiTitle *status;
bool bLoadMLP, bLoadSeq, bSimulation, bSwap, bNext, bPrev;

void drawAxes(ofVec3f center){
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

void pivot(ofVec3f center, float aX, float aY, float aZ){
    ofTranslate(center);
    ofRotateX(aX);
    ofRotateY(aY);
    ofRotateZ(aZ);
    ofTranslate(-center);
}

//--------------------------------------------------------------
void testApp::setup(){
    
    ofBackground(0);
    ofSetFrameRate(60);
    
    int PORT = 12000;
	receiver.setup(PORT);
    
    sprintf(oscStatus, "[LOCALPORT] %d\n", PORT);
    
    gui.setup();
	gui.addTitle("TRACKER \n[i] hide controls");
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
    
    for(int i = 0; i < K; i++)
        switch(i){
            case 0:
                kinects[i].setColor(ofColor(0xE0D0AA));
                break;
            case 1:
                kinects[i].setColor(ofColor(0x8DA893));
                break;
            case 2:
                kinects[i].setColor(ofColor(0x1DA813));
                break;
            case 3:
                kinects[i].setColor(ofColor(0x0DFF192));
                break;
            default:
                kinects[i].setColor(ofColor(0xFFFFFF));
                break;
        }

    
}

//--------------------------------------------------------------
void testApp::update(){
    processOSC();
    
    if(bLoadMLP){
        bLoadMLP = false;
        matrixData.openMLP();
        for(int i = 0; i < K; i++)
            kinects[i].setMatrix(matrixData.getMatrix(i));
    }
    
    
    char msg[2048];
    matrixData.getStatus(msg);
    strcat(msg, oscStatus);
    for(int i = 0; i < K; i++){
        char kinectMsg[500];
        kinects[i].getStatus(kinectMsg, i);
        strcat(msg, kinectMsg);
    }
    
    status->setName(msg);
    status->setSize(400, 100);
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
    drawAxes(centroid);
    
    for(int i = 0; i < K; i++)
        kinects[i].draw();
    
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
		ofxOscMessage m;
		receiver.getNextMessage(&m);
		if(m.getAddress() == "/pc"){
            int _k = m.getArgAsInt32(0);
            kinects[_k].clearOldData(ofGetFrameNum());
            if(_k < K){
                unsigned long l;
                char * data = m.getArgAsBlob(2, l);
                for(int i = 0; i < l; i += 6){
                    ofVec3f p;
                    p.x = ((int)data[i + 1] << 8) | ((int)data[i] & 0xFF);
                    p.y = ((int)data[i + 3] << 8) | ((int)data[i + 2] & 0xFF);
                    p.z = ((int)data[i + 5] << 8) | ((int)data[i + 4] & 0xFF);
                    kinects[_k].addPoint(p);
                }
            }
            
		}
        
		if(m.getAddress() == "/com"){
            int _k = m.getArgAsInt32(0);
            kinects[_k].clearOldData(ofGetFrameNum());
            string s = m.getArgAsString(1);
            vector<string> tokens =ofSplitString(s, ",");
            for (vector<string>::iterator it = tokens.begin(); it!=tokens.end(); ++it) {
                vector<string>comData = ofSplitString(*it, " ");
                ofVec3f pos;
                pos.x = ofToFloat(comData[2]);
                pos.y = ofToFloat(comData[3]);
                pos.z = ofToFloat(comData[4]);
                kinects[_k].addCOM(pos);
            }

            
        }
        
	}
    
}