//
//  kinectData.h
//  kinectCalibrator
//
//  Created by miguel valero espada on 11/17/12.
//
//
#include "ofMain.h"

#ifndef kinectCalibrator_kinectData_h
#define kinectCalibrator_kinectData_h

class kinectData{
    ofMesh points;
    vector<ofVec3f> coms;
    ofMatrix4x4 M;
    ofColor c;
    int lastData = 0;
    
    public:
    void clearOldData(int frameNum){
        if(lastData < frameNum){
            clear();
            lastData = frameNum;
        }
    }
    void setColor(ofColor _c){
        c = _c;
    }
    void setMatrix(ofMatrix4x4 _M){
        M = _M;
    }
    void addPoint(ofVec3f p){
        points.addVertex(M.postMult(p));
    }
    void draw(){
        ofPushStyle();
        ofSetColor(c);
        points.drawVertices();
        
        for (vector<ofVec3f>::iterator it = coms.begin(); it!=coms.end(); ++it){
            ofTranslate((*it).x, (*it).y, (*it).z);
            ofSetColor(0);
            ofSphere(10);
        }
        ofPopStyle();
        
    }
    int getNumVertices(){
        return points.getNumVertices();
    }
    ofVec3f getCentroid(){
        return points.getCentroid();
    }
    void clear(){
        points.clear();
        coms.clear();
    }
    void addCOM(ofVec3f c){
        coms.push_back(M.postMult(c));
    }
    void getStatus(char *str, int i){
        char comPos[256];
        for (vector<ofVec3f>::iterator it = coms.begin(); it!=coms.end(); ++it)
            sprintf(comPos, "(%4.f, %4.f, %4.f) ", (*it).x, (*it).y, (*it).z);
        
        sprintf(str, "[KINECT %1d]: %4d point data\n%s\n"
                , i, getNumVertices(), comPos);
    }

};
#endif
