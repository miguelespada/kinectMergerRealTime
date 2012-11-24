//
//  matcher.h
//  kinectMergerRealTime
//
//  Created by miguel valero espada on 11/24/12.
//
//

#include "constants.h"
#include "tracker.h"
#include "kinectData.h"

#ifndef kinectMergerRealTime_matcher_h
#define kinectMergerRealTime_matcher_h

class matcher{
private:
    int bestSol[N];
    int bestValue;
  
    bool startTracking(kinectData kinect, tracker *trackers){
        if (kinect.getCOMsize() != N) return false;
        
        for (int i = 0; i < N; i++)
            trackers[i].pos = kinect.getCOM(i);
        
        
        return true;
                
    }
    
    void matchCandidates(kinectData kinect, tracker *trackers){
        int sol[N];
        
        bestValue = 100000;
        for(int i = 0; i < N; i ++) bestSol[i] = -1;
        
        matchCandidatesBT(0, sol, kinect, trackers);
        
        for (int i = 0; i < N; i ++) {
            if (bestSol[i] != -1)
                trackers[i].addCandidate(kinect.getCOM(bestSol[i]));
        }

    }
    
    bool isSolution(int k, int j, int *sol) {
        for (int i = 0; i < k; i++) {
            if (sol[i] == -1) continue;
            if (j == sol[i]) return false;
        }
        return true;
    }
    
    float distances(int *sol, kinectData kinect, tracker *trackers) {
        float v = 0;
        for (int i = 0; i < N; i ++) {
            if (sol[i] == -1) v += 1000;
            else v += kinect.getCOM(sol[i]).distance(trackers[i].pos);
        }
        return v;
    }
    
    void matchCandidatesBT(int k, int *sol, kinectData kinect, tracker *trackers){
        for (int j = -1; j < kinect.getCOMsize(); j++) {
            if (isSolution(k, j, sol)) {
                sol[k] = j;
                if (k == N - 1) {
                    float currentValue = distances(sol, kinect, trackers);
                    if (currentValue < bestValue) {
                        for (int i = 0; i < N; i ++)
                            bestSol[i] = sol[i];
                        bestValue = currentValue;
                    }
                }
                else
                    matchCandidatesBT(k + 1, sol, kinect, trackers);
            }
        }
    }
public:
    bool startTracking(kinectData *kinects, tracker *trackers) {
        for(int i = 0; i < K; i++)
            if(startTracking(kinects[i], trackers))
                return true;
        
        return false;
       
    }
    
    // Uses backtracking to find the best match
    
    void matchCandidates(kinectData *kinects, tracker *trackers){
        for(int i = 0; i < K; i++)
            matchCandidates(kinects[i], trackers);
    }
    
    /*
    
    void matchCandidatesBT(int k, int sol[]) {
        for (int j = -1; j < coms.size(); j++) {
            if (isSol(k, j, sol)) {
                sol[k] = j;
                if (k == N - 1) {
                    float currentValue = distancias(sol);
                    if (currentValue < bestValue) {
                        for (int i = 0; i < N; i ++)
                            bestSol[i] = sol[i];
                        bestValue = currentValue;
                    }
                }
                else
                    matchCandidatesBT(k + 1, sol);
            }
        }
    }*/
};

#endif
