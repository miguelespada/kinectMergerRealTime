//
//  kinectData.h
//  kinectCalibrator
//
//  Created by miguel valero espada on 11/17/12.
//
//
#include "ofMain.h"
#include "constants.h"
#include "tracker.h"

#ifndef kinectCalibrator_kinectData_h
#define kinectCalibrator_kinectData_h

class kinectData{
    ofMesh points;
    vector<ofVec3f> coms;
    ofMatrix4x4 M;
    ofColor c;
    int lastData = 0;
    int pFrame = -1;
    public:
   
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
            if((*it).z > 0){
                ofPushMatrix();
                ofTranslate((*it));
                ofSetColor(255);
                ofSphere(10);
                ofPopMatrix();
            }
        }
        ofPopStyle();
        
    }
    int getNumVertices(){
        return points.getNumVertices();
    }
    ofVec3f getCentroid(){
        return points.getCentroid();
    }
    void clearCOM(){
            coms.clear();
    }
    void clearMesh(int frame){
        if(frame != pFrame){
            points.clear();
            pFrame = frame;
        }
    }
    void addCOM(ofVec3f c){
        coms.push_back(c);
    }
    int getCOMsize() {
        return coms.size();
    }
    ofVec3f getCOM(int i){
        return coms[i];
    }
    void getStatus(char *str, int i){
        char comPos[50];
        char comPosAll[400];
        strcpy(comPosAll, "");
        for (vector<ofVec3f>::iterator it = coms.begin(); it!=coms.end(); ++it){
            sprintf(comPos, "(%4.f, %4.f, %4.f) ", (*it).x, (*it).y, (*it).z);
            strcat(comPosAll, comPos);
        }
        
    
        sprintf(str, "[KINECT %1d]: %4d point data\n%s\n"
                , i, getNumVertices(), comPosAll);
        
    }
    
   
};
#endif
