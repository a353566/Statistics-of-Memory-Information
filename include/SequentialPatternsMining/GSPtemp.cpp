/**
 * Generalized Sequence Pattern (GSP) Algorithm
 */
#include <dirent.h>
#include <vector>
#include <list>
#include <string.h>
#include <stdio.h>
#include <iostream>

using namespace std;

typedef int elemType;

// An element in thedata sequence.
struct Itemset {
	vector<elemType> item; // store the single itemset's items
	int timestamp;         // record current itemsettimestamp
	Itemset(){
		timestamp = 0;
	}
};

// data sequence.
struct Sequence {
	int tid;                 // transation id
	int num;                 // the number of itemset.
	vector<Itemset> itemset; // store the itemsets.
	Sequence() {
		tid = 0;
		num = 0;
	}
};

int main(int argc, char** argv) {
	cout << "hello world" <<endl;
	return 0;
}

void Gsp(int seqNum) {  
	freopen("output.txt", "w", stdout);
	map<elemType, int> cand;
	// Scan the database to find 1-sequence.
	//-----------------------------first scan-----------------------------------
	cand = GenerateOneSequence(seqNum);
	//--------------------------first scan end----------------------------------
  
    vector<elemType> oneSeq;
    // Get 1-frequency sequence
    for(map<elemType, int>::iterator iter1 = cand.begin();
        iter1 != cand.end(); iter1 ++) {
        oneSeq.push_back(iter1->first);
		}
    // Get 2-frequency sequence
    vector<Sequence> candidateSet;
    candidateSet = GenerateTwoSequence(oneSeq);
    Output(candidateSet, 2);
		
		// Get 3 and above frequency sequence
    for(int k = 3;; k ++)  
    {
        candidateSet = JoinPhase(candidateSet);
        if(candidateSet.size() == 0) {
            cout << "No further " << k-1 << "-frequency patterns." << endl;
            break;
        }
        else{
            //candidateSet = PrunePhase(candidateSet);
            Output(candidateSet, k);
        }
    }
}

// First scan the database to get frequency is one itemset.
map<elemType, int> GenerateOneSequence(int seqNum) {
	map<elemType, int> dup;
	map<elemType, int> cand;
	// count every item in the sequence.
	for(int k = 0; k < literals.size(); k ++) {
		map<elemType, int>  item;
		for(int i = 0; i < literals[k].itemset.size(); i ++)
		{
			for(int j = 0; j < literals[k].itemset[i].item.size(); j ++)
			{
				item[literals[k].itemset[i].item[j]] ++;
			}
		}
		
		map<elemType, int>::iterator iter;
		for(iter = item.begin(); iter != item.end(); iter ++)
		{
			if(iter->second > 0)
				dup[iter->first] ++;
		}
	}
		
	// add up to become support.
	map<elemType, int>::iterator iter1, iter2;
	for (iter1 = dup.begin(); iter1 != dup.end(); iter1 ++) {
		if(iter1->second >= min_support) {
			cand[iter1->first] = iter1->second;
		}
	}
	
	cout << "min_support = " << minSupport * seqNum << endl;
	cout << "1-frequent candidate set" << endl;
	for(iter1 = cand.begin(); iter1 != cand.end(); iter1 ++)
		printf("<%d>:  %d\n", iter1->first, iter1->second);
	return cand;
}

// Generate two frequency sequences.
vector<Sequence> GenerateTwoSequence(vector<int> oneSeq)
{
	vector<Sequence> candidateSet;
	// gather all the two-frequency sequence.
	for(int i = 0; i < oneSeq.size(); i ++) {
		for(int j = 0; j < oneSeq.size(); j ++)
		{
			// <aa> <ab>
			Sequence duplicate;
			Itemset tmpItem1, tmpItem2;
			tmpItem1.item.push_back(oneSeq[i]);
			duplicate.itemset.push_back(tmpItem1);
			tmpItem2.item.push_back(oneSeq[j]);
			duplicate.itemset.push_back(tmpItem2);
			int cnt = CountSupport(duplicate);
			if(cnt >= min_support)
				candidateSet.push_back(duplicate);
			else 
				infrequencySeq.push_back(duplicate);
		}
	}
  
	// Generate <(ab)>. <(ac)>
	for(int i = 0; i < oneSeq.size() - 1; i ++) {
		for(int j = i+1; j < oneSeq.size(); j ++)
		{
			Sequence duplicate;
			Itemset tmpItem;
			tmpItem.item.push_back(oneSeq[i]);
			tmpItem.item.push_back(oneSeq[j]);
			duplicate.itemset.push_back(tmpItem);
			int cnt = CountSupport(duplicate);
			duplicate.num = cnt;
			if(cnt >= min_support)
				candidateSet.push_back(duplicate);
			else 
				infrequencySeq.push_back(duplicate);
		}
  }
	//return PrunePhase(candidateSet);
	return candidateSet;
}

// Count candidate set's support
int CountSupport(Sequence curSequence)
{
	int cnt = 0;
	//cout << "CountSupport" << endl;
	// 將 Sequence 轉換成 vector<vector<int> >
	// 方便後面搜尋
	vector<vector<int> > seq;
	for(int i = 0; i < curSequence.itemset.size(); i ++)
	{
		vector<int> subSeq;
		for(int j = 0; j < curSequence.itemset[i].item.size(); j ++)
		{
			subSeq.push_back(curSequence.itemset[i].item[j]);
		}
		seq.push_back(subSeq);
	}
	
	
	for(int k = 0; k < literals.size(); k ++)
	{
		vector<vector<int> > storeTime;
		for(int i = 0; i < seq.size(); i ++)
		{
			int matchNum;
			int t = -1;
			vector<int> localTime;
			for(int p = 0; p < elem2time[k].size(); p ++)
			{
				matchNum = 0;
				for(int j = 0; j < seq[i].size(); j ++)
				{
					if(elem2time[k][p][seq[i][j]] > 0)
					{
						t = elem2time[k][p][seq[i][j]];
						matchNum ++;
					}
				}
				
				if(matchNum == seq[i].size())
				{
					localTime.push_back(t);
				}
			}
			if(localTime.size() > 0)
				storeTime.push_back(localTime);
		}

		if(storeTime.size() != seq.size())
			continue;

		for(int i = 0; i < storeTime[0].size(); i ++)
		{
			bool flag = false;
			flag = Dfs(storeTime, storeTime.size(), storeTime[0][i], 1);
			if (flag)
			{
				cnt ++;
				break;
			}
		}
	}
	return cnt;
}

// Output k-th candidateSet.
void Output(vector<Sequence> candidateSet, int id)
{
	int len = candidateSet.size();
	if(len != 0)
		cout << id << "-frequency candidate set" << endl;
	
	for(int k = 0; k < len; k ++)
	{
		cout << "<";
		for(int i = 0; i < candidateSet[k].itemset.size(); i ++)
		{
			cout << "(";
			for(int j = 0; j < candidateSet[k].itemset[i].item.size(); j ++)
			{
				cout << candidateSet[k].itemset[i].item[j] << " ";
			}
			cout << ")";
		}
		cout << ">:  ";
		cout << candidateSet[k].num << endl;
	}
	cout << endl;
}

// generate k >= 3-frequency sequences
vector<Sequence> JoinPhase(vector<Sequence> tmpCandidateSet)
{
	vector<vector<elemType> > seq;	// vector<vector<int> >
	vector<Sequence> candidateSet;

	for(int k = 0; k < tmpCandidateSet.size(); k ++)
	{
		vector<elemType> items;
		for(int i = 0; i < tmpCandidateSet[k].itemset.size(); i ++)
			for(int j = 0; j < tmpCandidateSet[k].itemset[i].item.size(); j ++)
				items.push_back(tmpCandidateSet[k].itemset[i].item[j]);
		
		seq.push_back(items);
	}

	for(int i = 0; i < seq.size(); i ++)
		for(int j = 0; j < seq.size(); j ++)
		{
			vector<elemType> s1;
			vector<elemType> s2;
			for(int p = 1; p < seq[i].size(); p ++)
				s1.push_back(seq[i][p]);
			for(int q = 0; q < seq[j].size(); q ++)
				s2.push_back(seq[j][q]);
			
			bool flag = false;
			for(int k = 0; k < s1.size(); k ++)
				if(s1[k] != s2[k])
					flag = true;
			
			if(!flag) {
				Sequence tmpSeq;
				for(int p = 0; p < tmpCandidateSet[i].itemset.size(); p ++)
				{
					Itemset tmpItems;
					for(int q = 0; q < tmpCandidateSet[i].itemset[p].item.size(); q ++)
						tmpItems.item.push_back(tmpCandidateSet[i].itemset[p].item[q]);
					tmpSeq.itemset.push_back(tmpItems);
				}
				int tp = tmpCandidateSet[j].itemset.size()-1;
				int tq = tmpCandidateSet[j].itemset[tp].item.size()-1;
				int sp = tmpCandidateSet[i].itemset.size()-1;
				int sq = tmpCandidateSet[i].itemset[sp].item.size()-1;

				if(tmpCandidateSet[j].itemset[tp].item.size() > 1) {
					tmpSeq.itemset[sp].item.push_back(s2[s2.size()-1]);
				} else {
					Itemset tmpItems;
					tmpItems.item.push_back(s2[s2.size()-1]);
					tmpSeq.itemset.push_back(tmpItems);
				}
				int cnt = CountSupport(tmpSeq);
				tmpSeq.num = cnt;
				if(cnt >= min_support)
					candidateSet.push_back(tmpSeq);
				else 
					infrequencySeq.push_back(tmpSeq);
			}
		}
	return candidateSet;
}
