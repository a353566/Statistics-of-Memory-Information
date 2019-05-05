#ifndef EXPEREMENT_HPP
#define EXPEREMENT_HPP

#include <vector>
#include <map>
#include <string.h>
#include <stdio.h>
#include <iostream>

#include "DataManager.hpp"   // User
#include "EventManager.hpp"  // Event
#include "PredictResult.hpp" // PredictResult
using namespace std;

class Experiment_1 {
	public :
		const vector<User> *userVec;
		vector<PredictionMethod*> predictionMethodVec;
		
		Experiment_1(const vector<User> &userVec,const vector<PredictionMethod*> &predictionMethodVec) {
			this->userVec = &userVec;
			this->predictionMethodVec = predictionMethodVec;
		}
		
		bool start() {
			for (auto tempMethod =predictionMethodVec.begin(); tempMethod !=predictionMethodVec.end(); tempMethod++) {
				PredictionMethod &oneMethod = **tempMethod;
				map<int, vector<int> > ranksResultVec; // 這個演算法的結果 (userID, ranks)
				for (auto oneUser =userVec->begin(); oneUser !=userVec->end(); oneUser++) {
					oneMethod.clear();
					const vector<Event> &eventVec = oneUser->eventVec;
					vector<int> &ranks = ranksResultVec[oneUser->userID];
					vector<Event>::const_iterator thisEvent = eventVec.cbegin();
					vector<Event>::const_iterator nextEvent = eventVec.cbegin()+1;
					for (nextEvent; nextEvent !=eventVec.end(); thisEvent++, nextEvent++) {
						// ----- update new event
						oneMethod.addNewEvent(*thisEvent);
						//thisEvent->output(); //debug
						
						// ----- prediction
						const PredictResult &result = oneMethod.predict();
						
						// 在 "目前使用的 app 切換出去"時，做預測並記錄是否有猜中
						// -1-. 檢查 app event == true
						if (EventManager::getEventType(*nextEvent).appEvent) {
							
							// -2-. 檢查 是否是切換 app (檢查上次使用和這次使用的 app 是否不一樣)
							if (EventManager::isSwitchApp(eventVec, nextEvent)) {
								const Event &lastEvent = EventManager::getLastApp(eventVec, nextEvent);
								//nextEvent->output(); result.output(); printf("\n"); //debug
								
								// -3-. 記錄預測結果
								int rank = result.getRank(nextEvent->eventID, lastEvent.eventID);
								ranks.push_back(rank);
								//printf("%d ", rank); //debug
							}
						}
						//printf("\n"); //debug
						
					}
				}
				printResult(oneMethod, ranksResultVec);
			}
		}
		
		void printResult(const PredictionMethod &method, const map<int, vector<int> > &ranksResultVec) const {
			const int candidates = 20;
			map<int, map<int, double> > avgResults; // 平均的結果 <userID, <candidate, accuracy> >
			
			// ----- 平均並紀錄到 avgResults
			for (auto oneResult =ranksResultVec.cbegin(); oneResult !=ranksResultVec.cend(); oneResult++) {
				const int &thisUserID = oneResult->first;
				const vector<int> &ranks = oneResult->second;
				
				int avgs[candidates+1];
				memset(avgs, 0, sizeof(int)* (candidates+1));
				for (int rank: ranks) {
					if (rank <= candidates) {
						avgs[rank]++;
					}
				}
				
				// 平均
				const int &allSwitchTimes = ranks.size();
				map<int, double> &thisUserResult = avgResults[thisUserID];
				int accumulation = 0;
				for (int i=1; i<=candidates; i++) {
					accumulation += avgs[i];
					thisUserResult[i] = ((double) accumulation) / ((double) allSwitchTimes);
				}
			}
			
			// ----- print
			printf(" ----- Experiment 1: Just predict next one.\n");
			printf(" Method: %s\n", method.MethodName.c_str());
			printf("      |   Number of application candidates\n");
			printf("userID|");
			for (int i=1; i<=candidates; i++) {
				printf("    %3d", i);
			}
			printf("\n");
			
			// --- part.1 --- parameter "avgResults" (map<int, map<int, double> >) <userID, <candidate, accuracy> >
			map<int, double> allUserAvg;
			for (auto oneResult =avgResults.cbegin(); oneResult !=avgResults.cend(); oneResult++) {
				const int &thisUserID = oneResult->first;
				const map<int, double> &thisUserResult = oneResult->second;
				
				printf("%6d ", thisUserID); //userID
				
				for (int i=1; i<=candidates; i++) {
					allUserAvg[i] += thisUserResult.at(i);
					printf(" %.4f", thisUserResult.at(i));
				}
				printf("\n");
			}
			
			// --- part.2 --- all User Avg
			const int &userAmount = avgResults.size();
			
			printf("   Avg ");
			for (int i=1; i<=candidates; i++) {
				allUserAvg[i] = allUserAvg[i]/userAmount;
				printf(" %.4f", allUserAvg[i]);
			}
			printf("\n");
		}
		
};

#endif // EXPEREMENT_HPP