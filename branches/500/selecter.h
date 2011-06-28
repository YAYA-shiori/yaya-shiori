// 
// AYA version 5
//
// �o�͂̑I�����s�Ȃ��N���X�@CSelecter/CDuplEvInfo
// written by umeici. 2004
// 
// CSelecter�͏o�͂̑I�����s�Ȃ��܂��B
// CDuplEvInfo�͏d��������s�Ȃ��܂��B
//

#ifndef	SELECTERH
#define	SELECTERH

//----

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <vector>

#include "cell.h"
#include "value.h"
#include "variable.h"

#define	CHOICETYPE_RANDOM		0	/* ��ɖ���ׂɃ����_���i�f�t�H���g�j*/
#define	CHOICETYPE_NONOVERLAP	1	/* �����_�������ꏄ����܂ŏd���I�����Ȃ� */
#define	CHOICETYPE_SEQUENTIAL	2	/* ���ԂɑI������ */
#define	CHOICETYPE_VOID			3	/* �o�͂Ȃ� */
#define	CHOICETYPE_ARRAY		4	/* �ȈՔz��Ґ� */

#define	CHOICETYPE_NUM			5

const wchar_t	choicetype[CHOICETYPE_NUM][16] = {
	L"random",
	L"nonoverlap",
	L"sequential",
	L"void",
	L"array"
};

class CAyaVM;

//----

class CVecValue
{
public:
	std::vector<CValue> array;
};

//----

class	CDuplEvInfo
{
protected:
	int				type;			// �I�����

	std::vector<int>	num;			// --�ŋ�؂�ꂽ�̈斈�̌�␔
	std::vector<int>	roundorder;		// ���񏇏�

	int				lastroundorder; // ���O�̏��񏇏��l
	int				total;			// �o�͌��l�̑���
	int				index;			// ���݂̏���ʒu

private:
	CDuplEvInfo(void);

public:
	CDuplEvInfo(int tp)
	{
		type = tp;
		total = 0;
		index = 0;
		lastroundorder = -1;
	}

	int		GetType(void) { return type; }

	CValue	Choice(CAyaVM &vm,int areanum, const std::vector<CVecValue> &values, int mode);

protected:
	void	InitRoundOrder(CAyaVM &vm,int mode);
	char	UpdateNums(int areanum, const std::vector<CVecValue> &values);
	CValue	GetValue(CAyaVM &vm,int areanum, const std::vector<CVecValue> &values);
};

//----

class CSelecter
{
protected:
	CAyaVM &vm;
	std::vector<CVecValue>	values;			// �o�͌��l
	int					areanum;		// �o�͌���~�ς���̈�̐�
	CDuplEvInfo			*duplctl;		// �Ή�����d��������ւ̃|�C���^
	int					aindex;			// switch�\���Ŏg�p

private:
	CSelecter(void);
public:
	CSelecter(CAyaVM &vmr, CDuplEvInfo *dc, int aid);

	void	AddArea(void);
	void	Append(const CValue &value);
	CValue	Output(void);

protected:
	CValue StructArray1(int index);
	CValue StructArray(void);
	CValue	ChoiceRandom(void);
	CValue	ChoiceRandom1(int index);
	CValue	ChoiceByIndex(void);
	CValue	ChoiceByIndex1(int index);
};

//----

#endif
