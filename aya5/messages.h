// 
// AYA version 5
//
// ���O���b�Z�[�W
// written by umeici. 2004
// 
// �p��̃��b�Z�[�W�͊�{�I�Ɉȉ��̃T�C�g���Ŏ����|�󂵂����̂ł��B
// excite�|��
// http://www.excite.co.jp/world/
//

#ifndef	MESSAGESH
#define	MESSAGESH

//----

// ���b�Z�[�W���
#define	E_I			0	/* �W���̃��b�Z�[�W */
#define	E_F			1	/* �t�F�[�^���G���[ */
#define	E_E			2	/* �G���[ */
#define	E_W			3	/* ���[�j���O */
#define	E_N			4	/* ���L */
#define	E_END		5	/* ���O�̏I�� */
#define	E_SJIS		16	/* �}���`�o�C�g�����R�[�h��SJIS */
#define	E_UTF8		17	/* �}���`�o�C�g�����R�[�h��UTF-8 */
#define	E_DEFAULT	32	/* �}���`�o�C�g�����R�[�h��OS�f�t�H���g�̃R�[�h */

#if !defined(POSIX) && !defined(__MINGW32__)
// �t�F�[�^���G���[������i���{��j
const wchar_t	msgfj[][128] = {
	L"",
};

// �G���[������i���{��j
const wchar_t	msgej[][128] = {
	L"error E0000 : ���m�̃G���[�ł�.",
	L"error E0001 : �Ή�����֐�����������܂���.",
	L"error E0002 : '}' �ߑ��ł�.",
	L"error E0003 : �s���Ȋ֐����ł�.",
	L"error E0004 : ��͕s�\. '{' ���K�v�Ɣ��f����܂�.",
	L"error E0005 : �t�@�C�����J���܂���.",
	L"error E0006 : �ȈՔz��̕ϐ���������܂���.",
	L"error E0007 : �s���S�ȕ�����萔�ł�.",
	L"error E0008 : �s���ȃ_�u���N�H�[�e�[�V�����ł�.",
	L"error E0009 : �����G���[1���������܂���.",
	L"error E0010 : ��͕s�\�ȕ�����.",
	L"error E0011 : �ϐ���(?)�Ɏg�p�ł��Ȃ��������܂܂�Ă��܂�.",
	L"error E0012 : �ϐ������\���ƈ�v���Ă��܂�.",
	L"error E0013 : ���ꖼ�̊֐����d�����Ē�`����Ă��܂�.",
	L"error E0014 : �ȈՔz��̏�������ł�.",
	L"error E0015 : �����G���[2���������܂���.",
	L"error E0016 : �����G���[3���������܂���.",
	L"error E0017 : �����񒆂ɖ��ߍ��܂ꂽ�ȈՔz��̕ϐ���������܂���.",
	L"error E0018 : �����񒆂ɖ��ߍ��܂ꂽ�ȈՔz��̏�������ł�.",
	L"error E0019 : ����()�����Ă��܂���.",
	L"error E0020 : �ȈՔz���[]���s���A�������͕��Ă��܂���.",
	L"error E0021 : �����G���[4���������܂���.",
	L"error E0022 : �����̋L�q�Ɍ�肪����܂�.",
	L"error E0023 : ','�ŋ�؂�ꂽ�v�f�𐳂����擾�ł��܂���.",
	L"error E0024 : ','�ŋ�؂�ꂽ�v�f�𐳂����擾�ł��܂���.",
	L"error E0025 : ','�ŋ�؂�ꂽ�����ɑΉ�����֐���������܂���.",
	L"error E0026 : �����G���[5���������܂���.",
	L"error E0027 : �L���ȗv�f�������L�q����Ă��܂���.",
	L"error E0028 : �����G���[23���������܂���.",
	L"error E0029 : ���̑���͏����ł��܂���.",
	L"error E0030 : �s���ȏd��������[�h���w�肳��܂���.",
	L"error E0031 : �����G���[6���������܂���.",
	L"error E0032 : �K�{�̊֐�������܂���.",		// no use
	L"error E0033 : �����G���[7���������܂���.",
	L"error E0034 : �����G���[8���������܂���.",
	L"error E0035 : 'if'�ɑ���'{'������܂���.",
	L"error E0036 : 'elseif'�ɑ���'{'������܂���.",
	L"error E0037 : 'else'�ɑ���'{'������܂���.",
	L"error E0038 : 'switch'�ɑ���'{'������܂���.",
	L"error E0039 : 'while'�ɑ���'{'������܂���.",
	L"error E0040 : 'for'�̏I�����������ُ�ł�.",
	L"error E0041 : 'for'�̃��[�v�����ُ�ł�.",
	L"error E0042 : 'for'�ɑ���'{'������܂���.",
	L"error E0043 : 'foreach'�̒l�擾�ϐ����s���ł�.",
	L"error E0044 : 'foreach'�ɑ���'{'������܂���.",
	L"error E0045 : 'when'���x���ɒ萔�łȂ��v�f���L�q����Ă��܂�.",
	L"error E0046 : 'when'���x���ɂ́A�ϐ�/�֐��𖄂ߍ��񂾕�����͎g�p�ł��܂���.",
	L"error E0047 : �Ή�����'if'�A'elseif'��������'case'��������܂���.",
	L"error E0048 : ����()�����Ă��܂���.",
	L"error E0049 : �����G���[9���������܂���.",
	L"error E0050 : 'when'���x���ɒ萔�łȂ��v�f���L�q����Ă��܂�.",
	L"error E0051 : 'case'�ɑ���'{'������܂���.",
	L"error E0052 : �����G���[10���������܂���.",
	L"error E0053 : 'when'���x���̏������s���ł�.",
	L"error E0054 : 'when'���x���̏������s���ł�.",
	L"error E0055 : �����G���[11���������܂���.",
	L"error E0056 : �����G���[12���������܂���.",
	L"error E0057 : �ϐ��l���t�@�C���ɕۑ��ł��܂���.",
	L"error E0058 : �����G���[13���������܂���.",
	L"error E0059 : �����̋L�q�Ɍ�肪����܂�.",
	L"error E0060 : �����񒆂�\"%()\"�Ŗ��ߍ��܂ꂽ�v�f��()�����Ă��܂���.",
	L"error E0061 : �����񒆂�\"%()\"�Ŗ��ߍ��܂ꂽ�v�f����ł�.",
	L"error E0062 : �����G���[14���������܂���.",
	L"error E0063 : 'when'�ɑΉ�����'case'������܂���.",
	L"error E0064 : 'when'�ɑΉ�����'case'������܂���.",
	L"error E0065 : 'when'���x���̏������s���ł�.",
	L"error E0066 : 'when'���x���̏������s���ł�.",
	L"error E0067 : �����G���[15���������܂���.",
	L"error E0068 : ����'elseif'��������'when'�������ł��܂���ł���.",
	L"error E0069 : ����'else'��������'others'�������ł��܂���ł���.",
	L"error E0070 : �����G���[16���������܂���.",
	L"error E0071 : ���݂��Ȃ��֐������s���悤�Ƃ��Ă��܂�.",
	L"error E0072 : �ȈՔz��̕ϐ���������܂���.",
	L"error E0073 : �ȈՔz��̎g�p�@�ɕs��������܂�. �ϐ���������܂���.",
	L"error E0074 : �v���v���Z�b�T�̏����Ɍ�肪����܂�.",
	L"error E0075 : �v���v���Z�b�T�̏����Ɍ�肪����܂�.",
	L"error E0076 : �s���ȃv���v���Z�X���ł�.",
	L"error E0077 : �����G���[17���������܂���.",
	L"error E0078 : �����񒆂�'%[]'�Ŗ��ߍ��܂ꂽ�v�f��[]�����Ă��܂���.",
	L"error E0079 : �����񒆂�'%[]'�Ŗ��ߍ��܂ꂽ�v�f����ł�.",
	L"error E0080 : �����G���[18���������܂���.",
	L"error E0081 : '%[]'�ɐ�s����֐��A�������͕ϐ��̌Ăяo��������܂���.",
	L"error E0082 : �����G���[19���������܂���.",
	L"error E0083 : �����G���[20���������܂���.",
	L"error E0084 : �����G���[21���������܂���.",		// no use
	L"error E0085 : �����G���[22���������܂���.",		// no use
	L"error E0086 : �����G���[.",
	L"error E0087 : �t�B�[�h�o�b�N���Z�q�̈ʒu���s���ł�.",
	L"error E0088 : �����G���[24���������܂���.",
	L"error E0089 : �����G���[25���������܂���.",
	L"error E0090 : �����G���[26���������܂���.",
	L"error E0091 : �����G���[27���������܂���.",
	L"error E0092 : ()�A��������[]�ɑ������Z�q��������܂���.",
	L"error E0093 : �s���ȃV���O���N�H�[�e�[�V�����ł�.",
	L"error E0094 : ����������{}���s���A�������͕��Ă��܂���.",
};

// ���[�j���O������i���{��j
const wchar_t	msgwj[][128] = {
	L"warning W0000 : ���@�G���[. ���̍s�͖������܂�.",
	L"warning W0001 : ���̍s�̕ϐ��̕����Ɏ��s���܂���. ���̍s����͂ł��܂���.",
	L"warning W0002 : ���̍s�̕ϐ��̕����Ɏ��s���܂���. �ϐ������擾�ł��܂���ł���.",
	L"warning W0003 : ���̍s�̕ϐ��̕����Ɏ��s���܂���. �ϐ��l�܂��̓f���~�^���擾�ł��܂���ł���.",
	L"warning W0004 : ���̍s�̕ϐ��̕����Ɏ��s���܂���. �ϐ��l���s���ł�.",
	L"warning W0005 : ���̍s�̕ϐ��̕����Ɏ��s���܂���. �f���~�^���擾�ł��܂���ł���.",
	L"warning W0006 : ���̍s���������ɓ����G���[���������܂���. ���̍s����͂ł��܂���.",
	L"warning W0007 : ���̕ϐ��̒l��ۑ��ł��܂���ł���.",
	L"warning W0008 : �������s�����Ă��܂�.",
	L"warning W0009 : �����̌^���s���ł�.",
	L"warning W0010 : �󕶎��͎w��ł��܂���.",
	L"warning W0011 : ���ʂ�ϐ��ɑ΂��Đݒ�ł��܂���.",
	L"warning W0012 : �w�肳�ꂽ�l�͔͈͊O�A�������͖����ł�.",
	L"warning W0013 : �����Ɏ��s���܂���.",
	L"warning W0014 : �w�肳�ꂽ���C�u�����̓��[�h����Ă��܂���.",
	L"warning W0015 : �w�肳�ꂽ�t�@�C���̓I�[�v�����Ă��܂���.",
	L"warning W0016 : ���K�\���Ɍ�肪����܂�. ��������܂���ł���.",
	L"warning W0017 : ���K�\���������ɖ���`�̃G���[���������܂���.",
	L"warning W0018 : �ϐ����w�肵�Ă�������.",
	L"warning W0019 : ����Z�Ƀ[�����w�肷�邱�Ƃ͂ł��܂���.",
	L"warning W0020 : �A�z�z��̐����ɂ͋����̗v�f���K�v�ł�.",
	L"warning W0021 : ��̃q�A�h�L�������g������܂�.",
	L"warning W0022 : �q�A�h�L�������g�J�n������(�q�A�h�L�������g����)������܂���.",
};

// ���L������i���{��j
const wchar_t	msgnj[][128] = {
	L"note N0000 : �O����s���̕ϐ��l�̕����͍s���܂���ł���.",
	L"note N0001 : �����ȕ����R�[�h�Z�b�g���w�肳��܂���. OS�̃f�t�H���g�l���g�p���܂�.",
};

// ���̑��̃��O������i���{��j
const wchar_t	msgj[][96] = {
	L"// ���@���N�G�X�g���O\n// load ���� : ",
	L"// unload ���� : ",
	L"",	// ����
	L"// �����t�@�C���ǂݍ���\n",
	L"// �\����͌��ʂ̃��|�[�g (@:�֐��A$:�V�X�e���֐��A#:�z��)\n\n",
	L"// �ϐ���`��ԃ��|�[�g\n",
	L"// �ϐ��l�̕��� ",
	L"// �ϐ��l�̕ۑ� ",
	L"...����.\n\n",
	L"// �����̍\����͂ƒ��ԃR�[�h����\n",
};
#else
/* �c�O�Ȃ���A���C�h�����̖��ߍ��݂͕����R�[�h�𖾎��I�Ɏw�肷����@����������
   �ڐA��������܂���(���ߍ��ޑ���Ƀ��b�Z�[�W�t�@�C����ʂɎ����Ă����A
   �Ƃ����̂����݂̎嗬)�B�Ȃ̂œ��{�ꃁ�b�Z�[�W���[�h�ł��p����o���܂��c
 */
#define msgfj msgfe
#define msgej msgee
#define msgwj msgwe
#define msgnj msgne
#define msgj  msge
#endif

// �t�F�[�^���G���[������i�p��j
const wchar_t	msgfe[][128] = {
	L"",
};

// �G���[������i�p��j
const wchar_t	msgee[][256] = {
	L"error E0000 : Unknown error.",
	L"error E0001 : Function name is required.",
	L"error E0002 : Too much '}' exist.",
	L"error E0003 : Invalid name is used for the function.",
	L"error E0004 : Unanalyzable. Probably, '{' is required here.",
	L"error E0005 : Failed to open the file.",
	L"error E0006 : Missing name of the array.",
	L"error E0007 : Incomplete string exists.",
	L"error E0008 : Invalid doublequote exists.",
	L"error E0009 : Internal error (1).",
	L"error E0010 : Unanalyzable string.",
	L"error E0011 : Some invalid characters are used in the identifier.",
	L"error E0012 : Invalid name is used for the variable.",
	L"error E0013 : Duplicated function names exist.",
	L"error E0014 : Ordinal number is required for the array.",
	L"error E0015 : Internal error (2).",
	L"error E0016 : Internal error (3).",
	L"error E0017 : Missing name of array embedded into the string.",
	L"error E0018 : Missing ordinal number of array embedded into the string.",
	L"error E0019 : The parenthesis () has not been closed.",
	L"error E0020 : The parenthesis [] is invalid or has not been closed correctly.",
	L"error E0021 : Internal error (4).",
	L"error E0022 : Invalid expression.",
	L"error E0023 : Operator ',' is not used correctly.",
	L"error E0024 : Operator ',' is not used correctly.",
	L"error E0025 : Function not found.",
	L"error E0026 : Internal error (5).",
	L"error E0027 : No significant elements exist.",
	L"error E0028 : Internal error (23).",
	L"error E0029 : Invalid substitution exists.",
	L"error E0030 : Undefined function flag (anti-duplication) is specified.",
	L"error E0031 : Internal error (6).",
	L"error E0032 : Indispensable functions are not found.",		// no use
	L"error E0033 : Internal error (7).",
	L"error E0034 : Internal error (8).",
	L"error E0035 : Missing '{' after 'if'.",
	L"error E0036 : Missing '{' after 'elseif'.",
	L"error E0037 : Missing '{' after 'else'.",
	L"error E0038 : Missing '{' after 'switch'.",
	L"error E0039 : Missing '{' after 'while'.",
	L"error E0040 : Invalid expression is used for termination condition of the syntax 'for'.",
	L"error E0041 : Invalid expression is used for continual condition of the syntax 'for'.",
	L"error E0042 : Missing '{' after 'for'.",
	L"error E0043 : The container variable of 'foreach' is incorrectly placed.",
	L"error E0044 : Missing '{' after 'foreach'.",
	L"error E0045 : The label of 'when' must be a constant.",
	L"error E0046 : The label of 'when' must be a constant.",
	L"error E0047 : 'if' 'elseif' 'case' (corresponding to this line) was not found.",
	L"error E0048 : The parenthesis () has not brrn closed.",
	L"error E0049 : Internal error (9).",
	L"error E0050 : The label of 'when' must be a constant.",
	L"error E0051 : Missing '{' after 'case'.",
	L"error E0052 : Internal error (10).",
	L"error E0053 : Syntex error.",
	L"error E0054 : Syntex error.",
	L"error E0055 : Internal error (11).",
	L"error E0056 : Internal error (12).",
	L"error E0057 : The value of variable has not been saved.",
	L"error E0058 : Internal error (13).",
	L"error E0059 : Invalid expression.",
	L"error E0060 : The parenthesis '%()' has not been closed in embedded string.",
	L"error E0061 : The parenthesis '%()' is empty in embedded string.",
	L"error E0062 : Internal error (14).",
	L"error E0063 : There was no 'case' corresponding to 'when'.",
	L"error E0064 : There was no 'case' corresponding to 'when'.",
	L"error E0065 : Syntex error.",
	L"error E0066 : Syntex error.",
	L"error E0067 : Internal error (15).",
	L"error E0068 : This 'elseif'(or 'when') was not able to processed.",
	L"error E0069 : This 'else'(or 'others') was not able to processed.",
	L"error E0070 : Internal error (16).",
	L"error E0071 : Undefined function is being called.",
	L"error E0072 : Missing name of variable.",
	L"error E0073 : Invalid use of array: array variable was not found.",
	L"error E0074 : Syntax error of preprocessor.",
	L"error E0075 : Syntax error of preprocessor.",
	L"error E0076 : Invalid preprocess directive.",
	L"error E0077 : Internal error (17).",
	L"error E0078 : The parenthesis '%[]' has not been closed in embedded string.",
	L"error E0079 : The parenthesis '%[]' is empty in embedded string.",
	L"error E0080 : Internal error (18).",
	L"error E0081 : This '%[]' is unusual.",
	L"error E0082 : Internal error (19).",
	L"error E0083 : Internal error (20).",
	L"error E0084 : Internal error (21).",		// no use
	L"error E0085 : Internal error (22).",		// no use
	L"error E0086 : Syntax error.",
	L"error E0087 : The position of operator '&' is wrong.",
	L"error E0088 : Internal error (24).",
	L"error E0089 : Internal error (25).",
	L"error E0090 : Internal error (26).",
	L"error E0091 : Internal error (27).",
	L"error E0092 : There was no operator which should follow () or [].",
	L"error E0093 : Invalid singlequote exists.",
	L"error E0094 : The parenthesis {} is invalid or has not been closed correctly.",
};

// ���[�j���O������i�p��j
const wchar_t	msgwe[][128] = {
	L"warning W0000 : Syntax error. This line will be ignored.",
	L"warning W0001 : Restoration failed for the variable of this line: unparsable line.",
	L"warning W0002 : Restoration failed for the variable of this line: invalid variable name.",
	L"warning W0003 : Restoration failed for the variable of this line: invalid value or delimiter",
	L"warning W0004 : Restoration failed for the variable of this line: invalid value",
	L"warning W0005 : Restoration failed for the variable of this line: invalid delimiter",
	L"warning W0006 : While processing this line, the internal error occurred. This line is unanalyzable.",
	L"warning W0007 : The value of this variable has not been saved.",
	L"warning W0008 : Missing arguments.",
	L"warning W0009 : Mismatced type of argument(s).",
	L"warning W0010 : Null string is invalid.",
	L"warinig W0011 : The result can't be set to the variable.",
	L"warning W0012 : The numeric value is out of range or invalid.",
	L"warning W0013 : Processing failed.",
	L"warning W0014 : Requested library isn't loaded.",
	L"warning W0015 : Requested file isn't opened.",
	L"warning W0016 : Syntax error in regular expression.",
	L"warning W0017 : Unknown error occurred at regular expression.",
	L"warning W0018 : It is necessary to specify the variable.",
	L"warning W0019 : Integer/Double divided by zero.",
	L"warning W0020 : Requires even number of elements to create hash.",
	L"warning W0021 : Empty here document found.",
	L"warning W0022 : Here document start sequence found.",
};

// ���L������i�p��j
const wchar_t	msgne[][128] = {
	L"note N0000 : Restoration of a variable value was not performed.",
	L"note N0001 : Invalid character code set was specified. Using the default setting of OS instead.",
};

// ���̑��̃��O������i�p��j
const wchar_t	msge[][96] = {
	L"// AYA request log\n// load time: ",
	L"// unload time: ",
	L"",	// ����
	L"// Loading of dictionary file\n",
	L"// Parser report (@:function,$:system function,#:array)\n\n",
	L"// Variable definition report\n",
	L"// Loading of variable value ",
	L"// Saving of variable value ",
	L"...Done.\n\n",
	L"// Parsing of dictionary\n",
};

//----

#endif
