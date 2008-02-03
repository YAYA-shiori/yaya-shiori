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

#ifndef	WORDMATCHH
#define	WORDMATCHH

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <map>

#include "globaldef.h"

class CWordMatch
{
public:
  void addWord(const yaya::string_t& str, int value);
  int search(const yaya::string_t& str, int pos);
  ~CWordMatch();
  
private:
  
  struct nextmap_s;
  typedef std::map<yaya::char_t, nextmap_s*> word_map;
  typedef std::pair<yaya::char_t, nextmap_s*> word_pair;
  
  struct nextmap_s
  {
    word_map next;
    int value;
  };
  
  nextmap_s wordmap;

  void freemap(word_map* wordmap);

};


#endif

