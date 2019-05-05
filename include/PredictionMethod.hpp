#ifndef PREDICTION_METHOD_HPP
#define PREDICTION_METHOD_HPP

#include <vector>
#include <string.h>
#include <stdio.h>
#include <iostream>

#include "EventManager.hpp"
#include "PredictResult.hpp"
#include "GSPMining/GSP.hpp"
#include "GSPMining/GSP_Predict.hpp"
#include "GSPMining/Sequence.hpp"
using namespace std;

// 方法的 interface
class PredictionMethod {
	public :
		string MethodName;
		virtual void addNewEvent(const Event &addEvent) = 0;
		virtual const PredictResult& predict() = 0;
		virtual void clear() = 0;
};

#define FPA_MIN_SUPPORT 1
#define FPA_max_pattern_long 10
class FPA_Method_ours : public PredictionMethod {
	public :
		FPA_Method_ours(int method, const double *parameter, int paraSize) {
			this->method = method;
			for (int i=0; i<paraSize; i++) {
				this->parameter[i] = parameter[i];
			}
			gsp = GSP(FPA_MIN_SUPPORT, FPA_max_pattern_long);
			updatePoint = 0;
			MethodName = "FPA_our";
		}
		
		void addNewEvent(const Event &addEvent) {
			pastEventVec.push_back(addEvent);
		}
		
		const PredictResult& predict() {
			updateGSP();
			return FPAresult;
		}
		
		void clear() {
			gsp.clear();
			pastEventVec.clear();
			nowSeq.clear();
			FPAresult.clear();
			updatePoint = 0;
		}
		
	private :
		GSP gsp;
		vector<Event> pastEventVec;
		Sequence nowSeq;
		PredictResult FPAresult;
		int updatePoint;
		int method;
		double parameter[16];
		
		void updateGSP() {
			if (updatePoint == pastEventVec.size()) {
				// already update
				return;
			}
			
			for ( ; updatePoint < pastEventVec.size(); updatePoint++) {
				// update One Event
				auto newestEvent = pastEventVec.cbegin() + updatePoint;
				//DateTime time = pastEventVec[updatePoint].time; // didn't use
				const EventType eventType = EventManager::getEventType(newestEvent->eventID);
				// 是 app event 且和上一個不一樣才更新
				if (eventType.appEvent) {
					if (EventManager::isSwitchApp(pastEventVec, newestEvent)) {
						nowSeq += newestEvent->eventID;
						gsp.addNewOne(newestEvent->eventID);
						GSP_Predict gsp_Predict(&gsp);
						FPAresult = gsp_Predict.predictResult_byMethod(method, parameter, &nowSeq, 500);
						FPAresult.sort();
					}
				}
			}
		}
};

class MRU_Method : public PredictionMethod {
	public :
		MRU_Method() {
			updatePoint = 0;
			MethodName = "MRU";
		}
		
		void addNewEvent(const Event &addEvent) {
			pastEventVec.push_back(addEvent);
		}
		
		const PredictResult& predict() {
			updateLRUresult();
			return LRUresult;
		}
		
		void clear() {
			pastEventVec.clear();
			LRUresult.clear();
			updatePoint = 0;
		}
		
	private :
		vector<Event> pastEventVec;
		PredictResult LRUresult;
		int updatePoint;
		
		void updateLRUresult() {
			if (updatePoint == pastEventVec.size()) {
				// already update
				return;
			}
			
			for ( ; updatePoint < pastEventVec.size(); updatePoint++) {
				// update One Event
				int eventID = pastEventVec[updatePoint].eventID;
				//DateTime time = pastEventVec[updatePoint].time; // didn't use
				const EventType eventType = EventManager::getEventType(eventID);
				
				// 是 app event 才更新
				if (eventType.appEvent) {
					// 直接加入最高權重 (weight最高)
					LRUresult.sort();
					double maxWeight = (LRUresult.size() == 0) ? 0 : LRUresult.getPairWithIndex(0).second;
					pair<elemType, double> highestPair = make_pair(eventType.eventID, maxWeight+1);
					//printf("||%3d, %3d, %3.2f|", eventType.eventID, eventID, maxWeight+1);
					LRUresult.updatePair(highestPair);
				}
			}
		}
};

#endif // PREDICTION_METHOD_HPP