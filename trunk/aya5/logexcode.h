// 
// AYA version 5
//
// ���ԃR�[�h�����O�ɏo�͂���N���X�@CLogExCode
// written by umeici. 2004
// 
// �f�o�b�O�p�̋@�\�ł��B������ɂ͊֌W���܂���B
// �����̊֐����f���o���o�͂́A�\�[�X�R�[�h�𗝉����Ă��Ȃ��Ɠǂݓ���̂ł��B
//

#ifndef	LOGEXCODEH
#define	LOGEXCODEH

//----

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <vector>

#include "function.h"
#include "globaldef.h"

class CAyaVM;

class	CLogExCode
{
private:
	CAyaVM &vm;

	CLogExCode(void);
public:
	CLogExCode(CAyaVM &vmr) : vm(vmr) {
	}

	void	OutExecutionCodeForCheck(void);
	void	OutVariableInfoForCheck(void);
protected:
	void	StructCellString(std::vector<CCell> *cellvector, yaya::string_t &formula);
	void	StructSerialString(CStatement *st, yaya::string_t &formula);
	void	StructArrayString(const std::vector<CValueSub> &vs, yaya::string_t &enlist);
};

//----

#endif
