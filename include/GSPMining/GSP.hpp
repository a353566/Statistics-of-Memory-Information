#ifndef GSP_HPP
#define GSP_HPP
/**
 * Generalized Sequence Pattern (GSP) Algorithm
 */

#include <vector>
#include <map>
#include <string.h>
#include <stdio.h>
#include <iostream>

#include "Sequence.hpp"
using namespace std;

typedef int elemType;

class GSP {
	public :
		vector<elemType> literals;
		int min_support;
		
		map<elemType, int> appCountMap;	// 單個 element 出現次數
		
		/** 第一個是長度(沒有 1) 2, 3, 4, 5, ~ n.
		 *  Sequence 順序是由舊到新
		 */
		map<int, vector<Sequence> > miningPatternsMap;
		
		GSP(vector<elemType> literals);
		GSP(vector<elemType> literals, int min_support);
		
		void Mining();
		// First scan the database to get frequency is one itemset.
		void GenerateOneSequence();
		
		// Generate two frequency sequences.
		vector<Sequence> GenerateTwoSequence(const vector<elemType> *oneSeq);
		
		// generate k >= 3-frequency sequences
		vector<Sequence> JoinPhase(const vector<Sequence> *tmpCandidateSet);
		
		bool canConnect(const vector<elemType> *frontSeq, const vector<elemType> *backSeq);
		
		// Count candidate set's support
		int CountSupport(const Sequence *curSequence);
		
		// 過濾掉一些沒有用的 pattern (ex:只有開關螢幕的 pattern)
		void Filter(const vector<int> *filterVec);
		
		// Output k-th candidateSet.
		void Output( vector<Sequence> *candidateSet, int id);
		
		// 將 appCountMap 和 miningPatternsMap 一起輸出
		void OutputAll();
		void OutputAllTop();
};

#endif /* GSP_HPP */