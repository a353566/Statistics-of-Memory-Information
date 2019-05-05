#include <vector>
#include <map>
#include <string.h>
#include <stdio.h>
#include <iostream>

// ----- display part -----
//#define MAIN_display_File_name

#define MORE_DETAIL_OUTPUT  //顯示細節

#include "include/DataManager.hpp"
#include "include/EventManager.hpp"
#include "include/PredictResult.hpp"
#include "include/PredictionMethod.hpp"
#include "include/Experiment.hpp"
#include "include/GSPMining/GSP_Predict.hpp"
using namespace std;

int main(int argc, char** argv) {
	//{ ----- parameter initial
	string inputFolder("./source/new/");
	string file("user1,user2,user3,user4,user5");
	string blankTime("0,0,32,7,0");
	vector<string> fileVec;
	for (int i=1; i<argc; i++) {
		char *temp;
		// input
		temp = strstr(argv[i], "input=");
		if (temp != NULL) {
			inputFolder = string(temp+6);
			cout << "input folder is \"" << inputFolder << "\"" <<endl;
			continue;
		}
		// file
		/*temp = strstr(argv[i], "file=");
		if (temp != NULL) {
			file = string(temp+5);
			cout << "file is \"" << file << "\"\n" <<endl;
			continue;
		}*/
	}//}
	
	DataManager mDataManager(inputFolder, file, blankTime);
	vector<User> userVec = mDataManager.buildUserDatas();
	EventManager::saveEventType();
	//EventManager::output(); //debug
	
	// ----- add Method
	vector<PredictionMethod*> methodVec;
	// --- MRU
	MRU_Method tempMRU = MRU_Method();
	methodVec.push_back(&tempMRU);
	// --- FPA ours (level ver.)
	double parameter[2];
	parameter[0] = 0.00000001; // base
	parameter[1] = 5;          // powerNum
	FPA_Method_ours tempFPA = FPA_Method_ours(GSP_Predict::powerOfLevel, parameter, 2);
	methodVec.push_back(&tempFPA);
	
	// ----- start experiment
	Experiment_1 exp1(userVec, methodVec);
	exp1.start();
	
	
	
	// debug
	
	// all user file
	/*for (auto oneS: mDataManager.inputFiles) {
		printf("%s ", oneS.c_str());
	}
	printf("\n");*/
	
	// user 1 data
	/*User *tempUser;
	tempUser = &userVec.at(0);
	tempUser->output();*/
	
	return 0;
}