// 
// AYA version 5
//
// �萔
// written by umeici. 2004
// 

#ifndef	MANIFESTH
#define	MANIFESTH

#include "globaldef.h"

//----

extern const yaya::char_t *aya_name;
extern const yaya::char_t *aya_version;
extern const yaya::char_t *aya_author;

//----

// �ėp
#define	STRMAX					1024			/* �ėp�̕�����o�b�t�@�T�C�Y */

// �����R�[�h�Z�b�g
#define	CHARSET_SJIS			0				/* �f�t�H���g */
#define	CHARSET_UTF8			1
#define	CHARSET_EUCJP			2
#define	CHARSET_BIG5			3
#define	CHARSET_GB2312			4
#define	CHARSET_EUCKR			5
#define	CHARSET_JIS				6
#define	CHARSET_BINARY	126						/* �ϊ����Ȃ��i�ꕔ�ł̂ݗ��p�j */
#define	CHARSET_DEFAULT	127						/* OS�f�t�H���g�̌�����g�p���� */

// ���b�Z�[�W�̌���
#define	MSGLANG_JAPANESE		0				/* �f�t�H���g */
#define	MSGLANG_ENGLISH			1

// load/unload/request�n���h���̖��O
#define	FUNCNAME_LOAD			L"load"
#define	FUNCNAME_UNLOAD			L"unload"
#define	FUNCNAME_REQUEST		L"request"

// ���v���t�B�b�N�X������
#define	PREFIX_BASE_LEN	2
#define	PREFIX_BIN	L"0b"
#define	PREFIX_HEX	L"0x"

// �\����͌��ʃf�o�b�O�o�͎��̃C���f���g
#define	OUTDIC_INDENT			L"    "
#define	OUTDIC_INDENT_SIZE		4

// case���̕]���l���i�[���郍�[�J���ϐ��̃v���t�B�b�N�X
#define	PREFIX_CASE_VAR			L"_CaSe_ExPr_PrEfIx_"
#define	PREFIX_CASE_VAR_SIZE	18

// �W���̊ȈՔz��f���~�^
#define	VAR_DELIMITER			L","

// �t�@�C���ۑ�/�������Ƀ_�u���N�H�[�g/���䕶��(0x0000�`0x001f)���G�X�P�[�v���镶����
#define	ESC_DQ					L"_EsC_DoUbLe_QuOtE_"
#define	ESC_CTRL				L"_EsC_CtRl_CoDe_"

#define	CTRL_CH_START			'@'
#define	END_OF_CTRL_CH			0x1f

// �t�@�C���ۑ�/�������ɂ�����ėp�z��/�n�b�V���̋�v�f
#define	ESC_IARRAY				L"IARRAY"
#define ESC_IHASH				L"IHASH"
#define ESC_IVOID				L"IVOID"

// �X�e�[�g�����g�̎��
#define	ST_UNKNOWN				-1
#define	ST_NOP					0				/* no operation */

#define	ST_OPEN					1				/* { */
#define	ST_CLOSE				2				/* } */
#define	ST_COMBINE				3				/* -- */

#define	ST_BREAK				16				/* break */
#define	ST_CONTINUE				17				/* continue */
#define	ST_RETURN				18				/* return */
#define	ST_ELSE					19				/* else */

#define	ST_FORMULA				32				/* �����i���̒l�͍\����͎��̂݃t���O�Ƃ��Ďg���܂�) */
								// 33�A34����
#define	ST_FORMULA_OUT_FORMULA	35				/* �o�́i�����B�z��A�������֐����܂܂�܂��j */
#define	ST_FORMULA_SUBST		36				/* ��� */
#define	ST_IF					37				/* if */
#define	ST_ELSEIF				38				/* elseif */
#define	ST_WHILE				39				/* while */
#define	ST_SWITCH				40				/* switch */
#define	ST_FOR					41				/* for */
#define	ST_FOREACH				42				/* foreach */
#define	ST_WHEN					44				/* when�i���̒l�͍\����͎��̂݃t���O�Ƃ��Ďg���܂�) */
												/* AYA5�ł�case-when-others�\���͓�����if-elseif-else�\���ɕϊ�����܂� */
#define	ST_PARALLEL				45				/* parallel */
#define	ST_VOID					46				/* void */

// �����̍��̎��
#define	F_TAG_UNKNOWN			-2
#define	F_TAG_NOP				-1
#define F_TAG_VOID				-1				/* ����`�ϐ� */

#define	F_TAG_ORIGIN			0				/* ���Z�q�̒�`�J�n�ʒu */

#define	F_TAG_COMMA				0				/* , */
#define	F_TAG_PLUS				1				/* + */
#define	F_TAG_MINUS				2				/* - */
#define	F_TAG_MUL				3				/* * */
#define	F_TAG_DIV				4				/* / */
#define	F_TAG_SURP				5				/* % */
#define	F_TAG_IFEQUAL			6				/* == */
#define	F_TAG_IFDIFFER			7				/* != */
#define	F_TAG_IFGTEQUAL			8				/* >= */
#define	F_TAG_IFLTEQUAL			9				/* <= */
#define	F_TAG_IFGT				10				/* > */
#define	F_TAG_IFLT				11				/* < */
#define	F_TAG_IFIN				12				/* _in_ */
#define	F_TAG_IFNOTIN			13				/* !_in_ */
#define	F_TAG_OR				14				/* || */
#define	F_TAG_AND				15				/* && */
#define	F_TAG_FEEDBACK			16				/* & */
#define	F_TAG_EXC				17				/* ! */
#define	F_TAG_INCREMENT			18				/* ++�i������"+= 1"�ɕϊ������̂Ŏ��s���ɂ͑��݂��Ȃ��j */
#define	F_TAG_DECREMENT			19				/* --�i������"-= 1"�ɕϊ������̂Ŏ��s���ɂ͑��݂��Ȃ��j */

#define	F_TAG_EQUAL				20				/* = */
#define	F_TAG_PLUSEQUAL			21				/* += */
#define	F_TAG_MINUSEQUAL		22				/* -= */
#define	F_TAG_MULEQUAL			23				/* *= */
#define	F_TAG_DIVEQUAL			24				/* /= */
#define	F_TAG_SURPEQUAL			25				/* %= */
#define	F_TAG_EQUAL_D			26				/* := �iAYA5�ɂ����Ă�= �Ɠ���j */
#define	F_TAG_PLUSEQUAL_D		27				/* +:=�iAYA5�ɂ����Ă�+=�Ɠ���j */
#define	F_TAG_MINUSEQUAL_D		28				/* -:=�iAYA5�ɂ����Ă�-=�Ɠ���j */
#define	F_TAG_MULEQUAL_D		29				/* *:=�iAYA5�ɂ����Ă�*=�Ɠ���j */
#define	F_TAG_DIVEQUAL_D		30				/* /:=�iAYA5�ɂ����Ă�/=�Ɠ���j */
#define	F_TAG_SURPEQUAL_D		31				/* %:=�iAYA5�ɂ����Ă�%=�Ɠ���j */
#define	F_TAG_COMMAEQUAL		32				/* ,= */

#define	F_TAG_BRACKETIN			33				/* ( */
#define	F_TAG_BRACKETOUT		34				/* ) */
#define	F_TAG_HOOKBRACKETIN		35				/* [ */
#define	F_TAG_HOOKBRACKETOUT	36				/* ] */

#define	F_TAG_FUNCPARAM			37				/* �������֐��ɓn�����߂̉��Z�q */
#define	F_TAG_ARRAYORDER		38				/* �z��̏�����ϐ��ɓn�����߂̉��Z�q */
#define	F_TAG_SYSFUNCPARAM		39				/* �������V�X�e���֐��ɓn�����߂̉��Z�q */

#define	F_TAG_ORIGIN_VALUE		128				/* ���̒�`�J�n�ʒu */

#define	F_TAG_INT				128				/* ������ */
#define	F_TAG_DOUBLE			131				/* ������ */
#define	F_TAG_STRING			132				/* ������ */
#define	F_TAG_STRING_EMBED		133				/* �擪�ɕϐ��������͊֐������ߍ��܂�Ă���\�������镶���� */
#define	F_TAG_STRING_PLAIN		134				/* ���ߍ��ݓW�J����Ȃ������� */

#define	F_TAG_SYSFUNC			256				/* �V�X�e���֐� */
#define	F_TAG_USERFUNC			512				/* ���[�U�[�֐� */

#define	F_TAG_VARIABLE			1024			/* �ϐ� */
#define	F_TAG_LOCALVARIABLE		2048			/* ���[�J���ϐ� */

#define	F_TAG_ARRAY				4096			/* �ėp�z�� */
#define F_TAG_HASH              8192            /* �n�b�V�� */

// �L���ȉ��Z�q�̐�
// ���Z�q�̖��O
const yaya::char_t	formulatag[][19] = {
	L",",   L"+",   L"-",    L"*",     L"/",
	L"%",   L"==",  L"!=",   L">=",    L"<=",
	L">",   L"<",   L"_in_", L"!_in_", L"||",
	L"&&",  L"&",   L"!",    L"++",    L"--",

	L"=",   L"+=",  L"-=",   L"*=",    L"/=",
	L"%=",  L":=",  L"+:=",  L"-:=",   L"*:=",
    L"/:=", L"%:=", L",=",

	L"(",   L")",   L"[",    L"]",

	L"_Op_FuNc_PaRaM_", L"_Op_ArArY_OrDeR_", L"_Op_SySfUnC_PaRaM_"
};

#define	FORMULATAG_NUM			(sizeof(formulatag) / sizeof(formulatag[0]))

// ���Z�q�̖��O�̒���
const int	formulatag_len[FORMULATAG_NUM] = {
	1, 1, 1, 1, 1,
	1, 2, 2, 2, 2,
	1, 1, 4, 5, 2,
	2, 1, 1, 2, 2,

	1, 2, 2, 2, 2,
	2, 2, 3, 3, 3,
	3, 3, 2,

	1, 1, 1, 1,

	15, 16, 18
};

// ���Z�q�Ŋ֌W���鍀�̐��̍ŏ��l�F�G���[���o�p�i�P�����Z�q���񍀉��Z�q���j
const int	formulatag_params[FORMULATAG_NUM] = {
	2, 2, 2, 2, 2,
	2, 2, 2, 2, 2,
	2, 2, 2, 2, 2,
	2, 2, 1, 1, 1,

	2, 2, 2, 2, 2,
	2, 2, 2, 2, 2,
	2, 2, 2,

	1, 1, 1, 1,

	1, 1, 1
};


// ���Z�q�̗D�揇��
// ���l���傫���قǗD��x�͍����Ȃ�܂��B
// ()��[]�ł́A���̒��ɓ���ۂɗD��x���グ�A�o��ۂɖ߂��悤�ɂ��܂��B
const int	formulatag_depth[FORMULATAG_NUM] = {
	0, 7, 7, 8,  8,
	8, 5, 5, 5,  5,
	5, 5, 5, 5,  3,
	4, 6, 10, 9, 9,

	2, 1, 1, 1, 1,
	1, 2, 1, 1, 1,
	1, 1, 1,

	20, -20, 20, -20,

	11, 11, 11
};

// ����\��
#define	FLOWCOM_NUM	(sizeof(flowcom) / sizeof(flowcom[0]))

const yaya::char_t	flowcom[][16] = {
	L"if",
	L"elseif",
	L"else",
	L"case",
	L"when",
	L"others",
	L"switch",
	L"while",
	L"for",
	L"break",
	L"continue",
	L"return",
	L"foreach",
	L"parallel",
	L"void",
};

// {}�̎��
										// 0�`��switch�\���̌�⒊�o�ʒu
#define	BRACE_SWITCH_OUT_OF_RANGE	-1	/* switch�\���ɂ�����͈͊O�̌�⒊�o�ʒu */
#define	BRACE_DEFAULT				-2	/* �f�t�H���g */
#define	BRACE_LOOP					-3	/* ���[�v�\�� */

//----

// swap
template <class T>
void exchange(T& a, T& b)
{
	T t_value = a;
	a = b;
	b = t_value;
}

//----

#endif


