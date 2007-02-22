// 
// AYA version 5
//
// �����P��ւ̍����}�b�`���s���@CWordMatch
// written by Mikage Sawatari. 2004
// 
// CWordMatch �́C�\�ߕ����̌������o�^���Ă����C�w�蕶����̎w��ʒu����C
// �����ꂪ���݂��Ȃ����}�b�`�������s���܂��D
// �e������ɂ� int �l���P�֘A�Â��邱�Ƃ��o���C�}�b�`�����ꍇ�͊֘A�Â����l�C
// �}�b�`���Ȃ������ꍇ��-1���Ԃ�܂��D
// 

#include "globaldef.h"
#include "wordmatch.h"

//#include <iostream>

void CWordMatch::addWord(const yaya::string_t& str, int value)
{
	word_map* wordmapptr = &wordmap.next;
	word_map::iterator wordptr;
	size_t count = 1;
	//int strsize = str.size();
//	wcout << str << endl;
	for(yaya::string_t::const_iterator it = str.begin(); it != str.end(); ++it, ++count) {
      //      cout << "searct..." << endl;
		wordptr = wordmapptr->find(*it);
		if(wordptr != wordmapptr->end()) {
//			cout << "exists..." << endl;
			word_pair word = *wordptr;
			if(count == str.size()) {
				word.second->value = value;
//				cout << "set value1: " << value << endl;
			}
		
		nextmap_s* nextmap = word.second;
		wordmapptr = &nextmap->next;
		}
		else {
//			cout << "new..." << endl;
			word_pair* word = new word_pair;
			word->first = *it;
			nextmap_s* nextwordmap = new nextmap_s;
			word->second = nextwordmap;
			if(count == str.size()) {
				nextwordmap->value = value;
//				cout << "set value2: " << value << endl;
			}
			else {
				nextwordmap->value = -1;
			}
	  
		wordmapptr->insert(*word);
		wordmapptr = &nextwordmap->next;
		}
	}
  
}

int CWordMatch::search(const yaya::string_t& str, int pos)
{
	word_map* wordmapptr = &wordmap.next;
	word_map::iterator wordptr;
//	wcout << "[" << str << "] pos: " << pos << endl;
	int result = -1;
	for(unsigned int i = pos; i < str.size(); ++i) {
//		cout << "search: " << str[i] << endl;
		wordptr = wordmapptr->find(str[i]);
		if(wordptr != wordmapptr->end()) {
//			cout << "exists. " << result << endl;
			word_pair word = *wordptr;
//			cout << "value: " << word.second->value << endl;
			if(word.second->value != -1) {
				result = word.second->value;
			}
			nextmap_s* nextmap = word.second;
			wordmapptr = &nextmap->next;
		}
		else {
//			cout << "not found. " << result << endl;
			break;
		}
	}
  
//	cout << "result: " << result << endl;
	return result;
}

CWordMatch::~CWordMatch()
{
  freemap(&wordmap.next);
}

void CWordMatch::freemap(word_map* wordmap)
{
  for(word_map::iterator it = (*wordmap).begin();
      it != (*wordmap).end(); ++it)
    freemap(&(*it).second->next);
  
}

