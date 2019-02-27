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
		int pattern_long_max;
		
		map<elemType, int> appCountMap;	// 單個 element 出現次數
		
		/** 第一個是長度 (Range is from 2 to n, without 1.)
		 *  Sequence 順序是由舊到新
		 */
		map<int, vector<Sequence> > miningPatternsMap;
		
		/** 第一個是長度 (Range is from 2 to n, without 1.)
		 *  literals : source pattern
		 *  min_support : min support (至少要有多少 count )
		 *  pattern_long_max : pattern 限制可以有多長
		 */
		GSP(vector<elemType> literals);
		GSP(vector<elemType> literals, int min_support);
		GSP(vector<elemType> literals, int min_support, int pattern_long_max);
		
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
		
		// ┌--------------┐
		// |other function|
		// └--------------┘
		
		// 過濾掉一些沒有用的 pattern (ex:只有開關螢幕的 pattern)
		void Filter(const vector<int> *filterVec);
		
		// 加入新的 item (一要設定 pattern_long_max, 不然會跑不完)
		void addNewOne(elemType newItem);
		
		// Output k-th candidateSet.
		void Output( vector<Sequence> *candidateSet, int id);
		
		// 將 appCountMap 和 miningPatternsMap 一起輸出
		void OutputAll();
		void OutputAllTop();
};

#endif /* GSP_HPP */