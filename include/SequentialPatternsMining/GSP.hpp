#ifndef GSP_HPP
#define GSP_HPP
/**
 * Generalized Sequence Pattern (GSP) Algorithm
 */

#include <dirent.h>
#include <vector>
#include <map>
#include <list>
#include <string.h>
#include <stdio.h>
#include <iostream>

#include "../Sequence.hpp"
using namespace std;

class GSP {
	public :
		vector<elemType> literals;
		int min_support;
		
		map<elemType, int> oneElementMap;	// 單個 element 出現次數
		map<int, vector<Sequence> > miningPatternsMap;	// 第一個是長度(沒有 1) 2, 3, 4, 5, ~ n.
		GSP(vector<elemType> literals) {
			this->literals = literals;
			min_support = 2;
		}
		
		GSP(vector<elemType> literals, int min_support) {
			this->literals = literals;
			this->min_support = min_support;
		}
		
		void Mining() {
			// Scan the database to find 1-sequence.
			//-----------------------------first scan-----------------------------------
			GenerateOneSequence();
			//--------------------------first scan end----------------------------------
			
			// Get 1-frequency sequence
			vector<elemType> oneSeq;
			for(map<elemType, int>::iterator iter = oneElementMap.begin(); iter != oneElementMap.end(); iter ++)
			{
				oneSeq.push_back(iter->first);
			}
			
			// Get 2-frequency sequence
			vector<Sequence> candidateSet;
			candidateSet = GenerateTwoSequence(&oneSeq);
			miningPatternsMap[2] = candidateSet;
			
			// Get 3 and above frequency sequence
			for(int k = 3;; k ++)
			{
				candidateSet = JoinPhase(&candidateSet);
				if(candidateSet.size() == 0) {
					cout << "No further " << k-1 << "-frequency patterns." << endl;
					break;
				} else {
					miningPatternsMap[k] = candidateSet;
				}
			}
		}
		
		// First scan the database to get frequency is one itemset.
		void GenerateOneSequence() {
			map<elemType, int> dup;
			oneElementMap.clear();
			// count every item in the sequence.
			for(int k = 0; k < literals.size(); k ++) {
				dup[literals[k]]++;
			}
			
			// add up to become support.
			for (map<elemType, int>::iterator iter = dup.begin(); iter != dup.end(); iter ++) {
				if(iter->second >= min_support) {
					oneElementMap[iter->first] = iter->second;
				}
			}
			
			// add to miningPatternsMap
			vector<Sequence> oneSeqVec;
			for (map<elemType, int>::iterator iter = oneElementMap.begin(); iter != oneElementMap.end(); iter++) {
				Itemset item;
				item.item.push_back(iter->first);
				Sequence singleSeq;
				singleSeq.itemset.push_back(item);
				singleSeq.num = iter->second;
				oneSeqVec.push_back(singleSeq);
			}
			miningPatternsMap[1] = oneSeqVec;
		}
		
		// Generate two frequency sequences.
		vector<Sequence> GenerateTwoSequence(const vector<elemType> *oneSeq) {
			vector<Sequence> candidateSet;
			// gather all the two-frequency sequence.
			for(int i = 0; i < oneSeq->size(); i ++) {
				for(int j = 0; j < oneSeq->size(); j ++)
				{
					// <a,a> <a,b>
					Sequence duplicate;
					Itemset tmpItem1, tmpItem2;
					tmpItem1.item.push_back((*oneSeq)[i]);
					duplicate.itemset.push_back(tmpItem1);
					tmpItem2.item.push_back((*oneSeq)[j]);
					duplicate.itemset.push_back(tmpItem2);
					duplicate.num = CountSupport(&duplicate);
					if(duplicate.num >= min_support)
						candidateSet.push_back(duplicate);
				}
			}
			return candidateSet;
		}
		
		// generate k >= 3-frequency sequences
		vector<Sequence> JoinPhase(const vector<Sequence> *tmpCandidateSet) {
			// 先整理成 seq 比較好處理
			vector<vector<elemType> > seq;	// vector<vector<int> >
			for(int k = 0; k < tmpCandidateSet->size(); k ++)
			{
				vector<elemType> items;	// vector<int>
				for(int i = 0; i < (*tmpCandidateSet)[k].itemset.size(); i ++)
					items.push_back((*tmpCandidateSet)[k].itemset[i].item[0]);
				
				seq.push_back(items);
			}

			// 最後都要放到這裡面(candidateSet)再回傳
			vector<Sequence> candidateSet;
			
			for(vector<vector<elemType> >::iterator frontSeq = seq.begin(); frontSeq!=seq.end(); frontSeq ++) {
				for(vector<vector<elemType> >::iterator backSeq = seq.begin(); backSeq!=seq.end(); backSeq ++) {
					// 比對 frontSeq 和 backSeq 可不可以連接
					if (canConnect(&(*frontSeq), &(*backSeq))) {
						// 可以的話就創造 newSeq 做為新的 Sequence
						Sequence newSeq;
						for (int i = 0; i<frontSeq->size(); i ++) {
							Itemset newItem;
							newItem.item.push_back((*frontSeq)[i]);
							newSeq.itemset.push_back(newItem);
						}
						Itemset newItem;
						newItem.item.push_back((*backSeq)[backSeq->size()-1]);
						newSeq.itemset.push_back(newItem);
						
						// 用 newSeq 搜尋看看有多少個，並有超過 min_support 才加進去
						newSeq.num = CountSupport(&newSeq);
						if(newSeq.num >= min_support)
							candidateSet.push_back(newSeq);
					}
				}
			}
			return candidateSet;
		}
		
		bool canConnect(const vector<elemType> *frontSeq,const vector<elemType> *backSeq) {
			// 先比較長度 不一樣長就不行
			if (frontSeq->size() != backSeq->size())
				return false;
			
			// 比較中間的文字，一旦有不一樣的就回傳 false
			for (int i = 0; i+1 < frontSeq->size(); i++) {
				if ((*frontSeq)[i+1] != (*backSeq)[i]) {
					return false;
				}
			}
			return true;
		}
		
		// Count candidate set's support
		int CountSupport(const Sequence *curSequence) {
			int cnt = 0;
			//cout << "CountSupport" << endl;
			// 將 Sequence 轉換成 vector<int>
			// 方便後面搜尋
			vector<int> seq;
			for(int i = 0; i < curSequence->itemset.size(); i ++) {
			  seq.push_back(curSequence->itemset[i].item[0]);
			}
			
			// 開始一一搜尋
			for(int i = 0; i < literals.size(); i ++)
			{
				bool isfind = true;
				// 開始比對
				for(int j = 0; j < seq.size(); j ++) {
					// 看如果比對到不一樣的話就跳出，並把 isfind = false
					if (seq[j] != literals[i+j]) {
						isfind = false;
						break;
					}
				}
				
				// 有找到的話就 cnt++
				if (isfind) {
					cnt++;
				}
			}
			
			return cnt;
		}
		
		// 過濾掉一些沒有用的 pattern ex:只有開關螢幕的 pattern
		void Filter(const vector<int> *filterVec) {
			for(int lever = 2;; lever ++) {
				map<int, vector<Sequence> >::iterator PMIter = miningPatternsMap.find(lever);
				if(PMIter != miningPatternsMap.end()) {
					vector<Sequence> *seqVec;
					seqVec = &PMIter->second;
					for(vector<Sequence>::iterator iter = seqVec->begin(); iter != seqVec->end(); iter++)
					{
						bool isGood = false;
						for(int i=0; i<iter->itemset.size(); i++) {
							// iter->itemset[i].item[0]
							bool haveFilterData = false;
							for (int j=0; j<filterVec->size(); j++) {
							//for (vector<int>::iterator filter = filterVec->begin(); filter != filterVec->end(); filter++) {
								if (iter->itemset[i].item[0] == (*filterVec)[j]) {
								//if (iter->itemset[i].item[0] == *filter) {
									haveFilterData = true;
									break;
								}
							}
							if (!haveFilterData) {
								isGood = true;
								break;
							}
						}
						// isGood == false 代表此 pattern 只有不需要的資料
						if (!isGood) {
							iter = seqVec->erase(iter);
							iter--;
						}
					}
				} else {
					break;
				}
			}
		}
		
		// Output k-th candidateSet.
		void Output(const vector<Sequence> *candidateSet, int id) {
			int len = candidateSet->size();
			if(len != 0)
				cout << id << "-frequency candidate set" << endl;
			
			for(int k = 0; k < len; k ++)
			{
				cout << "<";
				for(int i = 0; i < (*candidateSet)[k].itemset.size(); i ++)
				{
					cout << "(";
					for(int j = 0; j < (*candidateSet)[k].itemset[i].item.size(); j ++)
					{
						cout << (*candidateSet)[k].itemset[i].item[j] << " ";
					}
					cout << ")";
				}
				cout << ">:  ";
				cout << (*candidateSet)[k].num << endl;
			}
			cout << endl;
		}
		
		// 將 oneElementMap 和 miningPatternsMap 一起輸出
		void OutputAll() {
			// 1-frequent
			cout << "min_support = " << min_support << endl;
			cout << "1-frequent candidate set" << endl;
			for(map<elemType, int>::iterator iter = oneElementMap.begin(); iter != oneElementMap.end(); iter ++)
				printf("<%d>:  %d\n", iter->first, iter->second);
			cout << endl;
			
			// 2~end-lever frequent
			for(int lever = 2;; lever ++) {
				map<int, vector<Sequence> >::iterator iter = miningPatternsMap.find(lever);
				if(iter != miningPatternsMap.end()) {
					Output(&iter->second, lever);
				} else {
					cout << "No further " << lever-1 << "-frequency patterns." << endl;
					break;
				}
			}
		}
		
		void OutpurAllTop() {
			multimap<int ,Sequence*> topMap;
			
			// 收集
			for(int lever = 2;; lever ++) {
				map<int, vector<Sequence> >::iterator PMIter = miningPatternsMap.find(lever);
				if(PMIter != miningPatternsMap.end()) {
					vector<Sequence> *seqVec;
					seqVec = &PMIter->second;
					for(vector<Sequence>::iterator iter = seqVec->begin(); iter != seqVec->end(); iter++) {
						topMap.insert(pair<int, Sequence*>(iter->num, &(*iter)));
					}
				} else {
					break;
				}
			}
			
			// Output
			for (multimap<int ,Sequence*>::iterator iter = topMap.begin(); iter != topMap.end(); iter++) {
				iter->second->Output();
			}
		}
};

// 往之前用過的查詢
class ESLaterTree {
  public :
		elemType myElem;  // 自己的 elem 雖然不太重要
		int count;  			// 此 Sequence 出現的次數
    int length; 			// 此 Sequence 的長度
		ESLaterTree *nextElement; // 接回上一個 statistics，如果只有一個的話則 =0 (有點可有可無)
		map<elemType, int> elemStatsMap; // 接下來會接的 elem, count (可能會出現的 elem)
		
		// 根據下一個 elem 回傳多加"下一個 elem"的,連續的，長度為 length+1 的 ESLaterTree
		map<elemType, ESLaterTree> lastElemMap;
		
		ESLaterTree() {
			myElem = -1;
			count = 0;
			length = 0;
			nextElement = 0;
		}
		
		ESLaterTree(elemType myElem, int count, int length, ESLaterTree *nextElement) {
			this->myElem = myElem;
			this->count = count;
			this->length = length;
			this->nextElement = nextElement;
		}
		
		void Build(GSP *gsp) {
			map<int, vector<Sequence> > *miningPatternsMap = &gsp->miningPatternsMap;	// 第一個是長度(沒有 1) 2, 3, 4, 5, ~ n.
			vector<Sequence> *oneSeqVec = &(*miningPatternsMap)[1];
			
			// 統計次數
			for (vector<Sequence>::iterator iter = oneSeqVec->begin(); iter != oneSeqVec->end(); iter ++) {
				count += iter->num;
			}
			
			for (vector<Sequence>::iterator iter = oneSeqVec->begin(); iter != oneSeqVec->end(); iter ++) {
				elemType elem = iter->itemset[0].item[0];
				// 加入單個 element
				elemStatsMap[elem] = iter->num;
				// 宣告 single's ESLaterTree
				lastElemMap[elem] = ESLaterTree(elem, iter->num, 1, this);
				ESLaterTree *singleES = &lastElemMap[elem];

				// 往下長
				if (!singleES->Grow(&(*iter), 1, miningPatternsMap)) {
					// 沒有往下長成功的話 要把根刪掉
					lastElemMap.erase(elem);
				}
			}
		}
		
		/** 這邊是用疊代的方式讓他長
		 *  ES : 目前的、需要往下長的
		 *  miningPatternsMap : Patterns 都在這裡面
		 *  level : 目前傳進來的是多少個
		 */
		bool Grow(Sequence *rootSeq, int level, map<int, vector<Sequence> > *miningPatternsMap) {
			bool found = false;	// 檢察看有沒有找到
			vector<Sequence> *seqVct = &((*miningPatternsMap)[level+1]);
			
			// 找到前面一樣的 pattern 並統計接下來會接的 elem
			for (vector<Sequence>::iterator iter = seqVct->begin(); iter != seqVct->end(); iter++) {
				if (Similar_front(rootSeq, &(*iter))) {
					found = true;	// 確實有找到
					// 將他加入到 elemStatsMap
					elemStatsMap[iter->itemset[level].item[0]] = iter->num;
				}
				// 找找看後面一樣的
				else if (Similar_back(rootSeq, &(*iter))) {
					elemType elem = iter->itemset[0].item[0];
					
					// 宣告新的 ESLaterTree 給初始值，並將他加入到 lastElemMap
					lastElemMap[elem] = ESLaterTree(elem, iter->num, level+1, this);
				  ESLaterTree *ES_addone = &lastElemMap[elem];
				
					// 最後讓他長
					if (!ES_addone->Grow(&(*iter), level+1, miningPatternsMap)) {
						// 沒有往下長的話 要把根刪掉
						lastElemMap.erase(elem);
					}
				}
			}
			return found;
		}
		
		static bool Similar_front(Sequence *front, Sequence *seq) {
			// 長度 front 要比 seq 少 1 不然一定哪裡有問題
			if (front->itemset.size()+1 != seq->itemset.size()) {
				cout << "(error) Similar_front(...) sequence size different!";
				cout << " front:" << front->itemset.size() << "seq:" << seq->itemset.size() <<endl;
				return false;
			} else if (front->itemset.size() == 0) { // 沒內容也一定有問題
				cout << "(error) Similar_front(...) front->size() = 0" <<endl;
				return false;
			}
			
			// 一旦發現不一樣 回傳false
			for (int i=0; i<front->itemset.size(); i++) {
				if (front->itemset[i].item[0] != seq->itemset[i].item[0]) {
					return false;
				}
			}
			return true;
		}
		
		static bool Similar_back(Sequence *front, Sequence *seq) {
			// 長度 front 要比 seq 少 1 不然一定哪裡有問題
			if (front->itemset.size()+1 != seq->itemset.size()) {
				cout << "(error) Similar_back(...) sequence size different!" <<endl;
				return false;
			} else if (front->itemset.size() == 0) { // 沒內容也一定有問題
				cout << "(error) Similar_back(...) front->size() = 0" <<endl;
				return false;
			}
			
			// 一旦發現不一樣 回傳false
			for (int i=0; i<front->itemset.size(); i++) {
				if (front->itemset[i].item[0] != seq->itemset[i+1].item[0]) {
					return false;
				}
			}
			return true;
		}
		
		void Output() {
			vector<elemType> seq;
			string front = string(" ");
			// 輸出 elemStatsMap
			for (map<elemType, int>::iterator iter = elemStatsMap.begin(); iter != elemStatsMap.end(); iter++) {
				cout << iter->first << ":" << iter->second <<endl;
			}
			// 輸出 lastElemMap
			for (map<elemType, ESLaterTree>::iterator iter = lastElemMap.begin(); iter != lastElemMap.end(); iter++) {
				iter->second.Output(&seq, &front);
			}
		}
	
		void Output(vector<elemType> *seq, const string *front) {
			vector<elemType> newSeq = *seq;
			newSeq.insert(newSeq.begin(), myElem);
			
			cout << *front << "< ";
			for (vector<elemType>::iterator iter = newSeq.begin(); iter != newSeq.end(); iter++) {
				cout << *iter << " ";
			}
			cout << " > " << length <<endl;
			
			// 輸出 elemStatsMap
			for (map<elemType, int>::iterator iter = elemStatsMap.begin(); iter != elemStatsMap.end(); iter++) {
				cout << *front << iter->first << ":" << iter->second <<endl;
			}
			
			// 輸出 lastElemMap
			string newFront = *front + "  ";
			for (map<elemType, ESLaterTree>::iterator iter = lastElemMap.begin(); iter != lastElemMap.end(); iter++) {
				iter->second.Output(&newSeq, &newFront);
			}
		}
};

// 往接下來會用的方向查詢
class ESForwardTree {
  public :
		elemType myElem;  // 自己的 elem 雖然不太重要
		int count;  			// 此 Sequence 出現的次數
    int length; 			// 此 Sequence 的長度
		ESForwardTree *nextElement; // 接回上一個 statistics，如果只有一個的話則 =0 (有點可有可無)
		map<elemType, int> elemStatsMap; // 接下來會接的 elem, count (可能會出現的 elem)
		
		// 根據下一個 elem 回傳多加"下一個 elem"的,連續的，長度為 length+1 的 ESForwardTree
		map<elemType, ESForwardTree> lastElemMap;
		
		ESForwardTree() {
			myElem = -1;
			count = 0;
			length = 0;
			nextElement = 0;
		}
		
		ESForwardTree(elemType myElem, int count, int length, ESForwardTree *nextElement) {
			this->myElem = myElem;
			this->count = count;
			this->length = length;
			this->nextElement = nextElement;
		}
		
		void Build(GSP *gsp) {
			map<int, vector<Sequence> > *miningPatternsMap = &gsp->miningPatternsMap;	// 第一個是長度(沒有 1) 2, 3, 4, 5, ~ n.
			vector<Sequence> *oneSeqVec = &(*miningPatternsMap)[1];
			
			for (vector<Sequence>::iterator iter = oneSeqVec->begin(); iter != oneSeqVec->end(); iter ++) {
				count += iter->num;
			}
			
			for (vector<Sequence>::iterator iter = oneSeqVec->begin(); iter != oneSeqVec->end(); iter ++) {
				elemType elem = iter->itemset[0].item[0];
				// 加入單個 element
				elemStatsMap[elem] = iter->num;
				// 宣告 single's ESForwardTree
				lastElemMap[elem] = ESForwardTree(elem, iter->num, 1, this);
				ESForwardTree *singleES = &lastElemMap[elem];

				// 往下長
				if (!singleES->Grow(&(*iter), 1, miningPatternsMap)) {
					// 沒有往下長成功的話 要把根刪掉
					lastElemMap.erase(elem);
				}
			}
		}
		
		/** 這邊是用疊代的方式讓他長
		 *  ES : 目前的、需要往下長的
		 *  miningPatternsMap : Patterns 都在這裡面
		 *  level : 目前傳進來的是多少個
		 */
		bool Grow(Sequence *rootSeq, int level, map<int, vector<Sequence> > *miningPatternsMap) {
			bool found = false;	// 檢察看有沒有找到
			vector<Sequence> *seqVct = &((*miningPatternsMap)[level+1]);
			// 找到前面一樣的 pattern 並將重要內容寫入，並且再疊代一次 Grow
			for (vector<Sequence>::iterator iter = seqVct->begin(); iter != seqVct->end(); iter++) {
				if (Similar_front(rootSeq, &(*iter))) {
					found = true;	// 確實有找到
					elemType elem = iter->itemset[level].item[0];
					// 先將他加入到 elemStatsMap
					elemStatsMap[elem] = iter->num;
					
					// 再來宣告新的 ESForwardTree 給初始值，並將他加入到 lastElemMap
					lastElemMap[elem] = ESForwardTree(elem, iter->num, level+1, this);
				  ESForwardTree *ES_addone = &lastElemMap[elem];
				
					// 最後讓他長
					if (!ES_addone->Grow(&(*iter), level+1, miningPatternsMap)) {
						// 沒有往下長的話 要把根刪掉
						lastElemMap.erase(elem);
					}
				}
			}
			return found;
		}
		
		static bool Similar_front(Sequence *front, Sequence *seq) {
			// 長度 front 要比 seq 少一 不然一定哪裡有問題
			if (front->itemset.size()+1 != seq->itemset.size()) {
				cout << "(error) Similar_front(...) sequence size different!" <<endl;
				return false;
			} else if (front->itemset.size() == 0) { // 沒內容也一定有問題
				cout << "(error) Similar_front(...) front->size() = 0" <<endl;
				return false;
			}
			
			// 一旦發現不一樣 回傳false
			for (int i=0; i<front->itemset.size(); i++) {
				if (front->itemset[i].item[0] != seq->itemset[i].item[0]) {
					return false;
				}
			}
			return true;
		}
		
		void Output() {
			vector<elemType> seq;
			string front = string(" ");
			// 輸出 elemStatsMap
			for (map<elemType, int>::iterator iter = elemStatsMap.begin(); iter != elemStatsMap.end(); iter++) {
				cout << iter->first << ":" << iter->second <<endl;
			}
			// 輸出 lastElemMap
			for (map<elemType, ESForwardTree>::iterator iter = lastElemMap.begin(); iter != lastElemMap.end(); iter++) {
				iter->second.Output(&seq, &front);
			}
		}
	
		void Output(vector<elemType> *seq, const string *front) {
			vector<elemType> newSeq = *seq;
			newSeq.push_back(myElem);
			
			cout << *front << "< ";
			for (vector<elemType>::iterator iter = newSeq.begin(); iter != newSeq.end(); iter++) {
				cout << *iter << " ";
			}
			cout << " > " << length <<endl;
			
			// 輸出 elemStatsMap
			for (map<elemType, int>::iterator iter = elemStatsMap.begin(); iter != elemStatsMap.end(); iter++) {
				cout << *front << iter->first << ":" << iter->second <<endl;
			}
			
			// 輸出 lastElemMap
			string newFront = *front + "  ";
			for (map<elemType, ESForwardTree>::iterator iter = lastElemMap.begin(); iter != lastElemMap.end(); iter++) {
				iter->second.Output(&newSeq, &newFront);
			}
		}
};

// 合併以上兩種方法，並實作 prediction funtion
class ElemStatsTree {
	public :
		ESLaterTree laterTree ;
		ESForwardTree forwardTree;
		void Build(GSP *gsp) {
			laterTree.Build(gsp);
			forwardTree.Build(gsp);
		}
		
		void Output() {
			cout << "===== forwardTree ====================" <<endl;
			forwardTree.Output();
			cout << "===== laterTree ======================" <<endl;
			laterTree.Output();
		}
};

#endif /* GSP_HPP */