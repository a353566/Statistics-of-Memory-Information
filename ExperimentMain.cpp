#include <dirent.h>
#include <vector>
#include <string.h>
#include <stdio.h>
#include <iostream>

// ----- display part -----
//#define MAIN_display_File_name
#define CANCEL_MORE_DETAIL_OUTPUT  // 取消所有多餘細節的顯示

#include "include/MergeFile.hpp"
#include "include/DataMiningExperiment.hpp" //mason
using namespace std;
int getdir(string dir, vector<string> &files);  // 取得資料夾中檔案的方法

int main(int argc, char** argv) {
	//{ ----- parameter initial
	string inputFolder("./data/");
	vector<string> fileVec;
	for (int i=1; i<argc; i++) {
		char *temp;
		temp = strstr(argv[i], "input=");
		if (temp != NULL) {
			inputFolder = string(temp+6);
			cout << "input folder is \"" << inputFolder << "\"" <<endl;
			continue;
		}
	}//}
	
	//{ ----- files initial
  // 取出檔案，並判斷有沒有問題
  if (getdir(inputFolder, fileVec) == -1) {
    cout << "Error opening" << inputFolder << endl;
    return -1;
  }//}
	
	string fileName;
	fileName = inputFolder + fileVec[0];
	cout << fileName <<endl;
	
  // 讀檔
  MergeFile mergeFile;
	mergeFile.ReadData(fileName);
	
	// 開始實驗
	mainOfExperiment(&mergeFile.allEventVec, &mergeFile.allAppNameVec); //mason

#ifndef CANCEL_MORE_DETAIL_OUTPUT
  cout << " ┌------┐\n" <<
	        " | over |\n" <<
					" └------┘"   <<endl;
#endif
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
