#ifndef SEQUENCE_HPP
#define SEQUENCE_HPP

#include <vector>
#include <iostream>
using namespace std;

typedef int elemType;

// An element in thedata sequence.
struct Itemset {
	vector<elemType> item; // store the single itemset's items
	int timestamp;         // record current itemsettimestamp
	Itemset() {
		timestamp = 0;
	}
};

// data sequence.
// 由舊到新
struct Sequence {
	int tid;                 // transation id
	int num;                 // the number of itemset.
	vector<Itemset> itemset; // store the itemsets.
	Sequence() {
		tid = 0;
		num = 0;
	}
	
	void Output() {
		cout << num << "\t<";
		for(int i = 0; i < itemset.size(); i ++) {
			cout << itemset[i].item[0] << " ";
		}
		cout << ">" <<endl;
	}
};

#endif /* SEQUENCE_HPP */