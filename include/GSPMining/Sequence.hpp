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
	
	void clear() {
		item.clear();
	}
	
	// (bug) 如果 item 沒有排序過的話，那就會出問題
	bool operator == (const Itemset &another) {
		// 比較長度
		if (this->item.size() == another.item.size()) {
			for (int i=0; i<this->item.size(); i++) {
				if (this->item[i] != another.item[i]) {
					if (this->item.size()>=2) {
						printf("(warning) Itemset::operator'==', THE item.size (%zu) > 1, could be error.", this->item.size());
					}
					return false;
				}
			}
		} else {
			return false;
		}
		// 到這裡的話，就代表一樣
		return true;
	}
	
	bool operator != (const Itemset &another) {
		return !(*this == another);
	}
};

// data sequence.
// 由舊到新
struct Sequence {
	int tid;                 // transation id (沒加也沒差)
	int num;                 // the appear number of this itemset.
	vector<Itemset> itemset; // store the itemsets.
	Sequence() {
		tid = 0;
		num = 0;
	}
	
	void clear() {
		itemset.clear();
	}
	
	void Output() {
		cout << num << "\t<";
		for(int i = 0; i < itemset.size(); i ++) {
			cout << itemset[i].item[0] << " ";
		}
		cout << ">" <<endl;
	}
	
	void operator += (const elemType &elem) {
		Itemset tmpItem;
		tmpItem.item.push_back(elem);
		itemset.push_back(tmpItem);
	}
	
	void operator += (const Itemset &item) {
		itemset.push_back(item);
	}
	
	Sequence operator + (const Sequence &seq) {
		Sequence newSeq;
		newSeq.itemset = this->itemset;
		for (int i=0; i<seq.itemset.size(); i++) {
			newSeq += seq.itemset[i];
		}
		return newSeq;
	}
	
	// 在 itemset 一樣時會回傳 true
	bool operator == (const Sequence &seq) {
		// 檢察 itemset 長度
		if (this->itemset.size() == seq.itemset.size()) {
			// 檢查內文
			for (int i=0; i<this->itemset.size(); i++) {
				if (this->itemset[i] != seq.itemset[i]) {
					return false;
				}
			}
		} else {
			return false;
		}
		// 到這裡的話，就代表一樣
		return true;
	}
};

#endif /* SEQUENCE_HPP */