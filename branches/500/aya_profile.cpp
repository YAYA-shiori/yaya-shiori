/*
  g++ -pg ...�Ŏg������z�肵���A�_�~�[��main�B
  �J�����g�f�B���N�g�� ./ ��������load���Ă�ŁA����unload���I������B
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

