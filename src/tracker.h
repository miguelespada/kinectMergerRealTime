//
//  tracker.h
//  kinectMergerRealTime
//
//  Created by miguel valero espada on 11/24/12.
//
//

#include "constants.h"

#ifndef kinectMergerRealTime_tracker_h
#define kinectMergerRealTime_tracker_h

class tracker{
public:
    vector<ofVec3f> candidates;
    ofVec3f pos;
    ofVec3f lerpedPos;
    float smoothFactor=0.5;
   
    void resetCandidates(){
        candidates.clear();
    }
    
    void lerp() {
        lerpedPos.interpolate(pos, smoothFactor);
    }
    
    void draw(){
        ofPushMatrix();
        ofSetColor(255);
        ofTranslate(pos);
        ofSphere(50);
        ofPopMatrix();
    }
    
    void addCandidate(ofVec3f c){
        candidates.push_back(c);
        
    }
    bool match() {
        ofVec3f *closest = NULL;
        
        for (vector<ofVec3f>::iterator it = candidates.begin(); it!=candidates.end(); ++it){
            if (closest == NULL) closest = &(*it);
            else if (pos.distance(*it) < pos.distance(*closest))
                closest = &(*it);
        }
        if(closest == NULL) return;
        
        pos.x = (*closest).x;
        pos.y = (*closest).y;
        pos.z = (*closest).z;
    }
    
};

#endif