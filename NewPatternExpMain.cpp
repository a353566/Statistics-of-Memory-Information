#include <dirent.h>
#include <vector>
#include <string.h>
#include <stdio.h>
#include <iostream>

// ----- display part -----
//#define MAIN_display_File_name

#define MORE_DETAIL_OUTPUT  //顯示細節

#include "include/Event.hpp"
//#include "include/MergeFile.hpp"
//#include "include/DataMiningExperiment.hpp" //mason
using namespace std;
int getNameID(string *name, vector<string> *allAppNameVec);
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
	
	FileManager mFileManager(inputFolder);
	mFileManager.initialUserFiles();
	UserManager userManager;
	userManager = mFileManager.buildUserManager();
	mFileManager.saveEventType(userManager.eventTypeVec);
	/*for (auto oneS: mFileManager.inputFiles) {
		printf("%s ", oneS.c_str());
	}
	printf("\n");*/
	
	
	// debug
	/*User *tempUser;
	tempUser = &userManager.userVec.at(0);
	tempUser->output();*/
	
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
