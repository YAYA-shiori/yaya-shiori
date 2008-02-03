// 
// AYA version 5
//
// 複数単語への高速マッチを行う　CWordMatch
// written by Mikage Sawatari. 2004
// 
// CWordMatch は，予め複数の検索語を登録しておき，指定文字列の指定位置から，
// 検索語が存在しないかマッチ処理を行います．
// 各検索語には int 値を１つ関連づけることが出来，マッチした場合は関連づけた値，
// マッチしなかった場合は-1が返ります．
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

