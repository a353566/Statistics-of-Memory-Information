#include <dirent.h>
#include <vector>
#include <string.h>
#include <stdio.h>
#include <iostream>

// ----- display part -----
//#define MAIN_display_File_name

// ----- "interval day" of training & test -----
#define ADD_TIME_SPACE_ON_HEAD // 開起前面加一些空白時段
// ----- parameter
int START_SPACE_TIME;
int TRAINING_INTERVAL_DAY;
int TEST_INTERVAL_DAY;

#define MORE_DETAIL_OUTPUT  //顯示細節

#include "include/MergeFile.hpp"
#include "include/DataMiningExperiment.hpp"
using namespace std;
int getdir(string dir, vector<string> &files);  // 取得資料夾中檔案的方法

int main(int argc, char** argv) {
	//{ ----- parameter initial
	string inputFolder("./data/");
	string file("fb5e43235974561d");
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
		temp = strstr(argv[i], "file=");
		if (temp != NULL) {
			file = string(temp+5);
			cout << "file is \"" << file << "\"\n" <<endl;
			continue;
		}
	}//}
	
	if (argc >= 5) {
		StringToNumber(string(argv[3]), &START_SPACE_TIME);
		StringToNumber(string(argv[4]), &TRAINING_INTERVAL_DAY);
		StringToNumber(string(argv[5]), &TEST_INTERVAL_DAY);
		cout << START_SPACE_TIME << "\n" << TRAINING_INTERVAL_DAY << "\n" << TEST_INTERVAL_DAY <<endl;
	} else {
		cout << "(error) the parameters is not enough" <<endl;
		return 0;
	}
	
	//{ ----- files initial
  // 取出檔案，並判斷有沒有問題
  if (getdir(inputFolder, fileVec) == -1) {
    cout << "Error opening" << inputFolder << endl;
    return -1;
  }//}
	
	string fileName;
	for (auto onefile =fileVec.begin(); onefile!=fileVec.end(); onefile++) {
		if (*onefile == file) {
					
			fileName = inputFolder + *onefile;
			
			// 讀檔
			MergeFile mergeFile;
			mergeFile.ReadData(fileName);
			mergeFile.filter();
			//string outputFolder("./data/no38");
			//mergeFile.SaveData(outputFolder);
			
			// 開始實驗
			mainOfExperiment(&mergeFile.allEventVec, &mergeFile.allAppNameVec); //mason
		}
	}

  //cout << " ┌-----------------┐\n" <<
	//        " | experiment over |\n" <<
	//				" └-----------------┘"   <<endl;
  return 0;
}

int getdir(string folder, vector<string> &files) {
  // 創立資料夾指標
  DIR *dp;
  struct dirent *dirp;
  if((dp = opendir(folder.c_str())) == NULL) {
    return -1;
  }
  // 如果dirent指標非空
  while((dirp = readdir(dp)) != NULL) {
    // 將資料夾和檔案名放入vector
    files.push_back(string(dirp->d_name));
  }
  // 關閉資料夾指標
  closedir(dp);
  return 0;
}
