/*
  g++ -pg ...で使う事を想定した、ダミーのmain。
  カレントディレクトリ ./ を引数にloadを呼んで、すぐunloadし終了する。
*/

#include "stdafx.h"

#include "aya5.h"
#include <stdlib.h>
#include <string.h>


int main(int argc, char* argv[]) {
    char* path = static_cast<char*>(malloc(3));
    memcpy(path, "./", 3);

    load(path, 2);
    unload();
    
    return 0;
}

