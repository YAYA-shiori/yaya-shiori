// 
// AYA version 5
//
// �\�����/���ԃR�[�h�̐������s���N���X�@CParser0
// written by umeici. 2004
// 

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <string>
#include <vector>

#include "parser0.h"

#include "ayavm.h"

#include "value.h"
#include "variable.h"

#include "basis.h"
#include "comment.h"
#include "function.h"
#include "log.h"
#include "messages.h"
#include "misc.h"
#include "parser1.h"
#if defined(POSIX)
# include "posix_utils.h"
#endif
#include "globaldef.h"
#include "sysfunc.h"
#include "wsex.h"

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

/* -----------------------------------------------------------------------
 *  �֐���  �F  CParser0::Parse
 *  �@�\�T�v�F  �w�肳�ꂽ�����t�@�C���Q��ǂݎ��A���s�\�Ȋ֐��Q���쐬���܂�
 *
 *  �Ԓl�@�@�F  0/1=����/�G���[
 * -----------------------------------------------------------------------
 */
char	CParser0::Parse(int charset, const std::vector<CDic1>& dics, int &lindex, int &ulindex, int &rindex)
{
	// �ǂݎ��A�\����́A���ԃR�[�h�̐���
	vm.logger().Message(3);
	std::vector<CDefine>	gdefines;
	int	errcount = 0;
	for(std::vector<CDic1>::const_iterator it = dics.begin(); it != dics.end(); it++) {
		vm.logger().Write(L"// ");
		vm.logger().Filename(it->path);
		errcount += LoadDictionary1(it->path, gdefines, it->charset);
	}
	vm.logger().Message(8);
	vm.logger().Message(9);

	errcount += AddSimpleIfBrace();

	errcount += SetCellType();
	errcount += MakeCompleteFormula();

	// ���ԃR�[�h�����̌㏈���ƌ���
	errcount += vm.parser1().CheckExecutionCode();

	// load/unload/request�֐��̈ʒu���擾
	lindex  = GetFunctionIndexFromName(FUNCNAME_LOAD);
//	if (lindex == -1) {
//		vm.logger().Error(E_E, 32, FUNCNAME_LOAD);
//		errcount++;
//	}
	ulindex = GetFunctionIndexFromName(FUNCNAME_UNLOAD);
//	if (ulindex == -1) {
//		vm.logger().Error(E_E, 32, FUNCNAME_UNLOAD);
//		errcount++;
//	}
	rindex  = GetFunctionIndexFromName(FUNCNAME_REQUEST);
//	if (rindex == -1) {
//		vm.logger().Error(E_E, 32, FUNCNAME_REQUEST);
//		errcount++;
//	}
	vm.logger().Message(8);

	return (errcount) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CParser0::ParseEmbedString
 *  �@�\�T�v�F  ����������Z�\�Ȑ����ɕϊ����܂�
 *
 *  �Ԓl�@�@�F  0/1=����/�G���[
 * -----------------------------------------------------------------------
 */
char	CParser0::ParseEmbedString(yaya::string_t& str, CStatement &st, const yaya::string_t& dicfilename, int linecount)
{
	// ������𐔎��ɐ��`
	if (!StructFormula(str, st.cell(), dicfilename, linecount))
		return 1;

	// �����̍��̎�ނ𔻒�
	if ( st.cell_size() ) { //�������p
		for(std::vector<CCell>::iterator it = st.cell().begin(); it != st.cell().end(); it++) {
			if (it->value_GetType() != F_TAG_NOP)
				continue;

			if (SetCellType1(*it, 0, dicfilename, linecount))
				return 1;
		}
	}

	// ()�A[]�̐���������
	if (CheckDepth1(st, dicfilename))
		return 1;

	// ���ߍ��ݗv�f�̕���
	if (ParseEmbeddedFactor1(st, dicfilename))
		return 1;

	// �V���O���N�H�[�g�������ʏ핶����֒u��
	ConvertPlainString1(st, dicfilename);

	// ���Z��������
	if (CheckDepthAndSerialize1(st, dicfilename))
		return 1;

	// �㏈���ƌ���
	if (vm.parser1().CheckExecutionCode1(st, dicfilename))
		return 1;

	return 0;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CParser0::LoadDictionary1
 *  �@�\�T�v�F  ��̎����t�@�C����ǂݎ��A��G�c�ɍ\�������߂��Ē~�ς��Ă����܂�
 *
 *  �Ԓl�@�@�F  1/0=�G���[/����
 * -----------------------------------------------------------------------
 */
char	CParser0::LoadDictionary1(const yaya::string_t& filename, std::vector<CDefine>& gdefines, int charset)
{
	yaya::string_t file = filename;
#if defined(POSIX)
	fix_filepath(file);
#endif
	// �t�@�C�����J��
	FILE	*fp = yaya::w_fopen((wchar_t *)file.c_str(), L"r");
	if (fp == NULL) {
		vm.logger().Error(E_E, 5, file);
		return 1;
	}

	// �ǂݎ��
	CComment	comment;
	char	ciphered = IsCipheredDic(file);
	yaya::string_t	linebuffer;
	int	depth = 0;
	int	targetfunction = -1;
	std::vector<CDefine>	defines;
	char	errcount = 0;

	int	 isInHereDocument = 0; //2 = �_�u���N�I�[�g 1 = �V���O���N�I�[�g
	bool isHereDocumentFirstLine = true;

	yaya::string_t	readline;
	std::vector<yaya::string_t>	factors;
	int	ret;

	for (int i = 1; ; i++) {
		// 1�s�ǂݍ��݁@�Í����t�@�C���̏ꍇ�͕������s�Ȃ�
		ret = yaya::ws_fgets(readline, fp, charset, ciphered, i);
		if (ret == yaya::WS_EOF)
			break;

		// �I�[�̉��s������
		CutCrLf(readline);
		if ( ! isInHereDocument ) {
			// �s�v�ȋ󔒁i�C���f���g���j������
			CutSpace(readline);
			// �R�����g����
			comment.Process_Top(readline);
			comment.Process(readline);
			// ��s�i�������͑S�̂��R�����g�s�������j�Ȃ玟��
			if (readline.size() == 0)
				continue;
		}
		else {
			// �s�v�ȋ󔒁i�C���f���g���j������
			CutStartSpace(readline);
		}
		
		// �ǂݎ��σo�b�t�@�֌���
		if ( isInHereDocument ) {
			//�q�A�h�L�������g������
			if ( isInHereDocument == 1 ) {
				if (readline.compare(0,3,L"'>>") == 0) {
					readline.erase(0,3);
					isInHereDocument = 0;
					
					linebuffer.append(L") ");
					linebuffer.append(readline);
				}
			}
			else {
				if (readline.compare(0,3,L"\">>") == 0) {
					readline.erase(0,3);
					isInHereDocument = 0;
					
					linebuffer.append(L") ");
					linebuffer.append(readline);
				}
			}

			//��������Ă��Ȃ��i�q�A�h�L�������g�����e�L�X�g�����̂܂�܌����j
			if ( isInHereDocument ) {
				if ( isHereDocumentFirstLine ) {
					linebuffer.append(L" (");
					isHereDocumentFirstLine = false;
				}
				else {
					linebuffer.append(L" + CHR(0xd,0xa) + ");
				}

				if ( isInHereDocument == 1 ) {
					yaya::ws_replace(readline, L"\'", L"\' + CHR(0x27) + \'");
					linebuffer.append(L"'");
					linebuffer.append(readline);
					linebuffer.append(L"'");
				}
				else {
					yaya::ws_replace(readline, L"\"", L"\" + CHR(0x22) + \"");
					linebuffer.append(L"\"");
					linebuffer.append(readline);
					linebuffer.append(L"\"");
				}
				continue;
			}
		}
		else {
			linebuffer.append(readline);
			// �I�[��"/"�Ȃ猋���Ȃ̂�"/"�������Ď���ǂ�
			if (readline[readline.size() - 1] == L'/') {
				linebuffer.erase(linebuffer.end() - 1);
				continue;
			}
			//�q�A�h�L�������g�J�n����
			else if ( readline.size() >= 3 ) {
				if ( readline.compare(readline.size()-3,3,L"<<'") == 0 ) {
					isInHereDocument = 1;
					isHereDocumentFirstLine = true;
					
					linebuffer.erase(linebuffer.size() - 3,3);
					continue;
				}
				else if ( readline.compare(readline.size()-3,3,L"<<\"") == 0 ) {
					isInHereDocument = 2;
					isHereDocumentFirstLine = true;

					linebuffer.erase(linebuffer.size() - 3,3);
					continue;
				}
			}
		}

		// �v���v���Z�b�T�̏ꍇ�͎擾
		int	pp = GetPreProcess(linebuffer, defines, gdefines, file, i);
		// �v���v���Z�b�T�ł������炱�̍s�̏����͏I���A����
		// �ُ�ȃv���v���Z�X�s�̓G���[
		if (pp == 1)
			continue;
		else if (pp == 2) {
			errcount = 1;
			continue;
		}
		// �v���v���Z�b�T#define�A#globaldefine����
		ExecDefinePreProcess(linebuffer, defines);	// #define
		ExecDefinePreProcess(linebuffer, gdefines);	// #globaldefine
		// �R�����g����(2)
		comment.Process_Tail(linebuffer);
		// {�A}�A;�ŕ���
		factors.clear();
		SeparateFactor(factors, linebuffer);
		// �������ꂽ���������͂��Ċ֐����쐬���A�����̃X�e�[�g�����g��~�ς��Ă���
		if (DefineFunctions(factors, file, i, depth, targetfunction))
			errcount = 1;
	}

	// �t�@�C�������
	::fclose(fp);

	if ( depth != 0 ) {
		vm.logger().Error(E_E, 94, filename, -1);
		errcount = 1;
	}

	return errcount;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CParser0::GetPreProcess
 *  �@�\�T�v�F  ���͂�#define/#globaldefine�v���v���Z�b�T�ł������ꍇ�A�����擾���܂�
 *
 *  �Ԓl�@�@�F  0/1/2=�v���v���Z�X�ł͂Ȃ�/�v���v���Z�X/�G���[
 * -----------------------------------------------------------------------
 */
char	CParser0::GetPreProcess(yaya::string_t &str, std::vector<CDefine>& defines, std::vector<CDefine>& gdefines, const yaya::string_t& dicfilename,
			int linecount)
{
#if !defined(POSIX) && !defined(__MINGW32__)
	static const yaya::string_t space_delim(L" \t�@");
#else
	static const yaya::string_t space_delim(L" \t\u3000");
#endif

	// �擪1�o�C�g��"#"�ł��邩���m�F
	// �i���̊֐��ɗ���܂łɋ󕶎���͏��O����Ă���̂ŁA�����Ȃ�[0]���Q�Ƃ��Ă����Ȃ��j
	if (str[0] != L'#')
		return 0;

	// �f���~�^�ł���󔒂������̓^�u��T��
	int	sep_pos = str.find_first_of(space_delim);
	if (sep_pos == -1) {
		vm.logger().Error(E_E, 74, dicfilename, linecount);
		return 2;
	}
	int sep_pos_end = sep_pos + 1;
	while (IsSpace(str[sep_pos_end])) { ++sep_pos_end; }

	// �������͖��O�ƒl�̋�؂�
	int	rep_pos = str.find_first_of(space_delim,sep_pos_end);
	if (rep_pos == -1) {
		vm.logger().Error(E_E, 75, dicfilename, linecount);
		return 2;
	}
	int rep_pos_end = rep_pos + 1;
	while (IsSpace(str[rep_pos_end])) { ++rep_pos_end; }

	// �v���v���Z�X���́A�ϊ��O������A�ϊ��㕶������擾
	// �擾�ł��Ȃ���΃G���[
	yaya::string_t	pname, bef, aft;

	pname.assign(str, 0, sep_pos);
	CutSpace(pname);

	bef.assign(str, sep_pos_end, rep_pos - sep_pos_end);
	CutSpace(bef);

	aft.assign(str, rep_pos_end, (int)str.size() - rep_pos_end);
	CutSpace(aft);

	//aft�̓J���ł��悢
	if (!pname.size() || !bef.size()) {
		vm.logger().Error(E_E, 75, dicfilename, linecount);
		return 2;
	}

	str.erase(); //�s�S�̂��O�����Ώۂ������̂ŏ���

	// ��ʂ̔���Ə��̕ێ�
	if (!pname.compare(L"#define")) {
		CDefine	adddefine(bef, aft);
		defines.push_back(adddefine);
	}
	else if (!pname.compare(L"#globaldefine")) {
		CDefine	adddefine(bef, aft);
		gdefines.push_back(adddefine);
	}
	else {
		vm.logger().Error(E_E, 76, pname, dicfilename, linecount);
		return 2;
	}

	return 1;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CParser0::ExecDefinePreProcess
 *  �@�\�T�v�F  #define/#globaldefine�����B�������u�����܂�
 * -----------------------------------------------------------------------
 */
void	CParser0::ExecDefinePreProcess(yaya::string_t &str, const std::vector<CDefine>& defines)
{
	for(std::vector<CDefine>::const_iterator it = defines.begin(); it != defines.end(); it++) {
		if (str.size() >= it->before.size()) {
			yaya::ws_replace(str, it->before.c_str(), it->after.c_str());
		}
	}
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CParser0::IsCipheredDic
 *  �@�\�T�v�F  �t�@�C�����Í����t�@�C�������t�@�C���g���q�����Ĕ��f���܂�
 *
 *  �Ԓl�@�@�F  1/0=�g���q��.ayc�ł���/.ayc�ł͂Ȃ�
 * -----------------------------------------------------------------------
 */
char	CParser0::IsCipheredDic(const yaya::string_t& filename)
{
	int	len = filename.size();
	if (len < 5)
		return 0;

	return ((filename[len - 3] == L'a' || filename[len - 3] == L'A') &&
		    (filename[len - 2] == L'y' || filename[len - 2] == L'Y') &&
		    (filename[len - 1] == L'c' || filename[len - 1] == L'C'))
			? 1 : 0;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CParser0::SeparateFactor
 *  �@�\�T�v�F  �^����ꂽ�������"{"�A"}"�A";"�̈ʒu�ŕ������܂��B";"�͈ȍ~�s�v�Ȃ̂ŏ������݂܂�
 * -----------------------------------------------------------------------
 */
void	CParser0::SeparateFactor(std::vector<yaya::string_t> &s, yaya::string_t &line)
{
	yaya::string_t::size_type separatepoint = 0;
	yaya::string_t::size_type apoint        = 0;
	yaya::string_t::size_type len           = line.size();
	yaya::string_t::size_type nextdq        = 0;
	char	executeflg    = 0;

	for( ; ; ) {
		// { } ; ����
		yaya::string_t::size_type newseparatepoint = line.find_first_of(L"{};",separatepoint);
		// ����������Ȃ���ΏI���
		if (newseparatepoint == yaya::string_t::npos)
			break;

		// �����ʒu���_�u���N�H�[�g���Ȃ疳�����Đ�֐i��
		nextdq = IsInDQ(line, separatepoint, newseparatepoint);
		if ( nextdq < IsInDQ_npos) {
			separatepoint = nextdq;
			continue;
		}
		//�N�I�[�g�����ĂȂ�������ȏ�݂͂���Ȃ�
		if ( nextdq == IsInDQ_runaway ) {
			break;
		}

		// �����ʒu�̎�O�̕�������擾
		if (newseparatepoint > apoint) {
			yaya::string_t	tmpstr;
			tmpstr.assign(line, apoint, newseparatepoint - apoint);
			CutSpace(tmpstr);
			s.push_back(tmpstr);
		}
		// ���������̂�"{"��������"}"�Ȃ炱����擾
		yaya::char_t c = line[newseparatepoint];
		if (c == L'{') {
			s.push_back(L"{");
		}
		else if (c == L'}') {
			s.push_back(L"}");
		}
		// �����J�n�ʒu���X�V
		apoint = separatepoint = newseparatepoint + 1;
		if (separatepoint >= len) {
			executeflg = 1;
			break;
		}
	}

	// �܂������񂪎c���Ă���Ȃ炻����ǉ�
	if (executeflg == 0) {
		yaya::string_t	tmpstr;
		tmpstr.assign(line, apoint, len - apoint);
		CutSpace(tmpstr);
		s.push_back(tmpstr);
	}

	// ���̕�����̓N���A����
	line = L"";
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CParser0::DefineFunctions
 *  �@�\�T�v�F  ������Q����͂��Ċ֐��̌��^���`����
 *
 *  �Ԓl�@�@�F  1/0=�G���[/����
 * -----------------------------------------------------------------------
 */
char	CParser0::DefineFunctions(std::vector<yaya::string_t> &s, const yaya::string_t& dicfilename, int linecount, int &depth, int &targetfunction)
{
	char	retcode = 0;

	for(std::vector<yaya::string_t>::iterator it = s.begin(); it != s.end(); it++) {
		// ��s�̓X�L�b�v
		if (!(it->size()))
			continue;

		// {}����q�̐[�������Ċ֐�������������
		// �[����0�Ȃ�{}�̊O�Ȃ̂Ŋ֐������擾���ׂ��ʒu�ł���
		if (!depth) {
			// �֐��̍쐬
			if (targetfunction == -1) {
				// �֐����Əd������I�v�V�������擾
				yaya::string_t	d0, d1;
				if (!Split(*it, d0, d1, L":"))
					d0 = *it;
				// �֐����̐���������
				if (IsLegalFunctionName(d0)) {
					if (!it->compare(L"{"))
						vm.logger().Error(E_E, 1, L"{", dicfilename, linecount);
					else if (!it->compare(L"}"))
						vm.logger().Error(E_E, 2, dicfilename, linecount);
					else
						vm.logger().Error(E_E, 3, d0, dicfilename, linecount);
					return 1;
				}
				// �d������I�v�V�����̔���
				int	chtype = CHOICETYPE_RANDOM;
				if (d1.size()) {
				    int i = 0;
					for(i = 0; i < CHOICETYPE_NUM; i++) {
						if (!d1.compare(choicetype[i])) {
							chtype = i;
							break;
						}
					}
					if (i == CHOICETYPE_NUM) {
						vm.logger().Error(E_E, 30, d1, dicfilename, linecount);
						return 1;
					}
				}
				// �쐬
				targetfunction = MakeFunction(d0, chtype, dicfilename);
				if (targetfunction == -1) {
				        vm.logger().Error(E_E, 13, *it, dicfilename, linecount);
					return 1;
				}
				continue;
			}
			// �֐����̎��̃X�e�[�g�����g�@�����"{"�łȂ���΂Ȃ�Ȃ�
			else {
				if (it->compare(L"{")) {
					vm.logger().Error(E_E, 4, dicfilename, linecount);
					return 1;
				}
				// ����q�̐[�������Z
				depth++;
			}
		}
		else {
			// �֐����̃X�e�[�g�����g�̒�`�@{}����q�̌v�Z�������ōs��
			if (!StoreInternalStatement(targetfunction, *it, depth, dicfilename, linecount))
				retcode = 1;
			// ����q�[����0�ɂȂ����炱�̊֐���`����E�o����
			if (!depth)
				targetfunction = -1;
		}
	}

	return retcode;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CParser0::MakeFunction
 *  �@�\�T�v�F  ���O���w�肵�Ċ֐����쐬���܂�
 *
 *  �Ԓl�@�@�F  �쐬���ꂽ�ʒu��Ԃ��܂�
 *  �@�@�@�@�@  �w�肳�ꂽ���O�̊֐������ɍ쐬�ς̏ꍇ�̓G���[�ŁA-1��Ԃ��܂�
 * -----------------------------------------------------------------------
 */
int	CParser0::MakeFunction(const yaya::string_t& name, int chtype, const yaya::string_t& dicfilename)
{
	int	i = GetFunctionIndexFromName(name);
	if(i != -1)
		return -1;
/*	for(std::vector<CFunction>::iterator it = vm.function().begin(); it != vm.function().end(); it++)
		if (!name.compare(it->name))
			return -1;
*/

	vm.function().push_back(CFunction(vm, name, chtype, dicfilename));
	vm.functionmap().insert(yaya::indexmap::value_type(name,static_cast<int>(vm.function().size()-1)));

	return vm.function().size() - 1;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CParser0::StoreInternalStatement
 *  �@�\�T�v�F  �֐����̃X�e�[�g�����g����ނ𔻒肵�Ē~�ς��܂�
 *
 *  �Ԓl�@�@�F  0/1=�G���[/����
 * -----------------------------------------------------------------------
 */
char	CParser0::StoreInternalStatement(int targetfunc, yaya::string_t &str, int& depth, const yaya::string_t& dicfilename, int linecount)
{
	// �p�����[�^�̂Ȃ��X�e�[�g�����g

	// {
	if (!str.compare(L"{")) {
		depth++;
		vm.function()[targetfunc].statement.push_back(CStatement(ST_OPEN, linecount));
		return 1;
	}
	// }
	else if (!str.compare(L"}")) {
		depth--;
		vm.function()[targetfunc].statement.push_back(CStatement(ST_CLOSE, linecount));
		return 1;
	}
	// others�@else�֏��������Ă��܂�
	else if (!str.compare(L"others")) {
		vm.function()[targetfunc].statement.push_back(CStatement(ST_ELSE, linecount));
		return 1;
	}
	// else
	else if (!str.compare(L"else")) {
		vm.function()[targetfunc].statement.push_back(CStatement(ST_ELSE, linecount));
		return 1;
	}
	// break
	else if (!str.compare(L"break")) {
		vm.function()[targetfunc].statement.push_back(CStatement(ST_BREAK, linecount));
		return 1;
	}
	// continue
	else if (!str.compare(L"continue")) {
		vm.function()[targetfunc].statement.push_back(CStatement(ST_CONTINUE, linecount));
		return 1;
	}
	// return
	else if (!str.compare(L"return")) {
		vm.function()[targetfunc].statement.push_back(CStatement(ST_RETURN, linecount));
		return 1;
	}
	// --
	else if (!str.compare(L"--")) {
		vm.function()[targetfunc].statement.push_back(CStatement(ST_COMBINE, linecount));
		return 1;
	}

	// �p�����[�^�̂���X�e�[�g�����g�i���䕶�j
	yaya::string_t	st, par;
	if (!Split(str, st, par, L" "))
		st = str;
	yaya::string_t	t_st, t_par;
	if (!Split(str, t_st, t_par, L"\t"))
		t_st = str;
	if (st.size() > t_st.size()) {
		st  = t_st;
		par = t_par;
	}
	// if
	if (!st.compare(L"if")) {
		str = par;
		return MakeStatement(ST_IF, targetfunc, str, dicfilename, linecount);
	}
	// elseif
	else if (!st.compare(L"elseif")) {
		str = par;
		return MakeStatement(ST_ELSEIF, targetfunc, str, dicfilename, linecount);
	}
	// while
	else if (!st.compare(L"while")) {
		str = par;
		return MakeStatement(ST_WHILE, targetfunc, str, dicfilename, linecount);
	}
	// switch
	else if (!st.compare(L"switch")) {
		str = par;
		return MakeStatement(ST_SWITCH, targetfunc, str, dicfilename, linecount);
	}
	// for
	else if (!st.compare(L"for")) {
		str = par;
		return MakeStatement(ST_FOR, targetfunc, str, dicfilename, linecount);
	}
	// foreach
	else if (!st.compare(L"foreach")) {
		str = par;
		return MakeStatement(ST_FOREACH, targetfunc, str, dicfilename, linecount);
	}
	// case�@����Ȗ��O�̃��[�J���ϐ��ւ̑���ɏ��������Ă��܂�
	else if (!st.compare(L"case")) {
		str = PREFIX_CASE_VAR + vm.function()[targetfunc].name;
		str += yaya::ws_itoa(linecount);
		str += L"=";
		str += par;
		return MakeStatement(ST_FORMULA, targetfunc, str, dicfilename, linecount);
	}
	// when
	else if (!st.compare(L"when")) {
		str = par;
		return MakeStatement(ST_WHEN, targetfunc, str, dicfilename, linecount);
	}
	// parallel
	else if (!st.compare(L"parallel")) {
		str = par;
		return MakeStatement(ST_PARALLEL, targetfunc, str, dicfilename, linecount);
	}
	// void
	else if (!st.compare(L"void")) {
		str = par;
		return MakeStatement(ST_VOID, targetfunc, str, dicfilename, linecount);
	}

	// ����܂ł̂��ׂĂɃ}�b�`���Ȃ�������͐����ƔF�������
	return MakeStatement(ST_FORMULA, targetfunc, str, dicfilename, linecount);
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CParser0::MakeStatement
 *  �@�\�T�v�F  �֐����̃X�e�[�g�����g���쐬�A�~�ς��܂�
 *
 *  �Ԓl�@�@�F  0/1=�G���[/����
 * -----------------------------------------------------------------------
 */
char	CParser0::MakeStatement(int type, int targetfunc, yaya::string_t &str, const yaya::string_t& dicfilename, int linecount)
{
	if (!str.size()) {
		vm.logger().Error(E_E, 27, dicfilename, linecount);
		return 0;
	}

	CStatement	addstatement(type, linecount);
	if (type == ST_WHEN) {
		if (!StructWhen(str, addstatement.cell(), dicfilename, linecount))
			return 0;
	}
	else {
		if (!StructFormula(str, addstatement.cell(), dicfilename, linecount))
			return 0;
	}

	vm.function()[targetfunc].statement.push_back(addstatement);
	return 1;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CParser0::StructFormula
 *  �@�\�T�v�F  ������𐔎��̍��Ɖ��Z�q�ɕ������A��{�I�Ȑ��`���s���܂�
 *
 *  �Ԓl�@�@�F  0/1=�G���[/����
 *
 *  �n���ꂽ������͂��̊֐��Ŕj�󂳂�܂�
 * -----------------------------------------------------------------------
 */
char	CParser0::StructFormula(yaya::string_t &str, std::vector<CCell> &cells, const yaya::string_t& dicfilename, int linecount)
{
	// ���Z�q�ƍ��ɕ����@���̎�ʂ͂��̎��_�ł͒��ׂĂ��Ȃ�
	StructFormulaCell(str, cells);

	// ���`�Ə����G���[�̌��o
	char	bracket = 0;
	for(std::vector<CCell>::iterator it = cells.begin(); it != cells.end(); ) {
		// ���O��")""]"�̏ꍇ�A���͉��Z�q�����Ȃ���΂Ȃ�Ȃ�
		if (bracket) {
			bracket = 0;
			if (it->value_GetType() == F_TAG_NOP) {
				vm.logger().Error(E_E, 92, dicfilename, linecount);
				return 0;
			}
		}
		// �������i+�̒��O�̍������������邢��")""]"�ȊO�̉��Z�q�@���̍��͏����Ă悢�j
		if (it->value_GetType() == F_TAG_PLUS) {
			if (it == cells.begin()) {
				it = cells.erase(it);
				continue;
			}
			std::vector<CCell>::iterator	itm = it;
			itm--;
			if (itm->value_GetType() != F_TAG_NOP &&
				itm->value_GetType() != F_TAG_BRACKETOUT &&
				itm->value_GetType() != F_TAG_HOOKBRACKETOUT) {
				it = cells.erase(it);
				continue;
			}
		}
		// �������i-�̒��O�̍������������邢��")"�ȊO�̉��Z�q�@"-1*"�ɐ��`����j
		if (it->value_GetType() == F_TAG_MINUS) {
			if (it == cells.begin()) {
				it->value_SetType(F_TAG_NOP);
				it->value().s_value = L"-1";
				it++;
				CCell	addcell2(F_TAG_MUL);
				it = cells.insert(it, addcell2);
				it++;
				continue;
			}
			std::vector<CCell>::iterator	itm = it;
			itm--;
			if (itm->value_GetType() != F_TAG_NOP &&
				itm->value_GetType() != F_TAG_BRACKETOUT &&
				itm->value_GetType() != F_TAG_HOOKBRACKETOUT) {
				it->value_SetType(F_TAG_NOP);
				it->value().s_value = L"-1";
				it++;
				CCell	addcell2(F_TAG_MUL);
				it = cells.insert(it, addcell2);
				it++;
				continue;
			}
		}
		// �C���N�������g�i"+=1"�ɐ��`����j
		if (it->value_GetType() == F_TAG_INCREMENT) {
			it->value_SetType(F_TAG_PLUSEQUAL);
			it++;
			CCell	addcell(F_TAG_NOP);
			addcell.value().s_value = L"1";
			it = cells.insert(it, addcell);
			it++;
			continue;
		}
		// �f�N�������g�i"-=1"�ɐ��`����j
		if (it->value_GetType() == F_TAG_DECREMENT) {
			it->value_SetType(F_TAG_MINUSEQUAL);
			it++;
			CCell	addcell(F_TAG_NOP);
			addcell.value().s_value = L"1";
			it = cells.insert(it, addcell);
			it++;
			continue;
		}
		// !�A&���Z�q�i��O�Ƀ_�~�[��0��ǉ��j
		if (it->value_GetType() == F_TAG_FEEDBACK) {
			if (it == cells.begin()) {
				vm.logger().Error(E_E, 87, dicfilename, linecount);
				return 0;
			}
			CCell	addcell(F_TAG_NOP);
			addcell.value().s_value = L"0";
			it = cells.insert(it, addcell);
			it += 2;
			continue;
		}
		if (it->value_GetType() == F_TAG_EXC) {
			CCell	addcell(F_TAG_NOP);
			addcell.value().s_value = L"0";
			it = cells.insert(it, addcell);
			it += 2;
			continue;
		}
		// �ȈՔz�񏘐��A�N�Z�X���Z�q�i"["���O�ɕt�^�j
		if (it->value_GetType() == F_TAG_HOOKBRACKETIN) {
			if (it == cells.begin()) {
				vm.logger().Error(E_E, 6, dicfilename, linecount);
				return 0;
			}
			CCell	addcell(F_TAG_ARRAYORDER);
			it = cells.insert(it, addcell);
			it += 2;
			continue;
		}
		// �֐��v�f�w�艉�Z�q�i"("���O�����Z�q�łȂ���Εt�^�j
		if (it->value_GetType() == F_TAG_BRACKETIN) {
			if (it != cells.begin()) {
				std::vector<CCell>::iterator	itm = it;
				itm--;
				if (itm->value_GetType() == F_TAG_NOP) {
					CCell	addcell(F_TAG_FUNCPARAM);
					it = cells.insert(it, addcell);
					it += 2;
					continue;
				}
			}
		}
		// ��̃J�b�R�i"()"�@�����@�֐��v�f�w�艉�Z�q���������ꍇ�͂���������j
		// �֐�����2�ȏ�ŋ�̈����i",)"�̏ꍇ�A""��⊮�j
		if (it->value_GetType() == F_TAG_BRACKETOUT) {
			bracket = 1;
			if (it != cells.begin()) {
				std::vector<CCell>::iterator	itm = it;
				itm--;
				if (itm->value_GetType() == F_TAG_BRACKETIN) {
					it = cells.erase(itm);
					it = cells.erase(it);
					if (it != cells.begin()) {
						itm = it;
						itm--;
						if (itm->value_GetType() == F_TAG_FUNCPARAM)
							it = cells.erase(itm);
					}
					continue;
				}
				else if (itm->value_GetType() == F_TAG_COMMA) {
					CCell	addcell(F_TAG_NOP);
					addcell.value().s_value = L"\"\"";
					it = cells.insert(it, addcell);
					it += 2;
					continue;
				}
			}
		}
		// �����J�b�R�i"[]"�@�G���[�j
		if (it->value_GetType() == F_TAG_HOOKBRACKETOUT) {
			bracket = 1;
			if (it != cells.begin()) {
				std::vector<CCell>::iterator	itm = it;
				itm--;
				if (itm->value_GetType() == F_TAG_HOOKBRACKETIN) {
					vm.logger().Error(E_E, 14, dicfilename, linecount);
					return 0;
				}
			}
		}
		// �֐�����2�ȏ�ŋ�̈����i"(,"�����",,"�̏ꍇ�A""��⊮�j
		if (it->value_GetType() == F_TAG_COMMA) {
			if (it != cells.begin()) {
				std::vector<CCell>::iterator	itm = it;
				itm--;
				if (itm->value_GetType() == F_TAG_BRACKETIN || itm->value_GetType() == F_TAG_COMMA) {
					CCell	addcell(F_TAG_NOP);
					addcell.value().s_value = L"\"\"";
					it = cells.insert(it, addcell);
					it += 2;
					continue;
				}
			}
		}
		// ���̍���
		it++;
	}

	return 1;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CParser0::StructWhen
 *  �@�\�T�v�F  ������𐔎��̍��Ɖ��Z�q�ɕ������A��{�I�Ȑ��`���s���܂��iwhen�\���p�j
 *
 *  �Ԓl�@�@�F  0/1=�G���[/����
 *
 *  �n���ꂽ������͂��̊֐��Ŕj�󂳂�܂�
 * -----------------------------------------------------------------------
 */
char	CParser0::StructWhen(yaya::string_t &str, std::vector<CCell> &cells, const yaya::string_t& dicfilename, int linecount)
{
	// ���Z�q�ƍ��ɕ����@���̎�ʂ͂��̎��_�ł͒��ׂĂ��Ȃ�
	StructFormulaCell(str, cells);

	// ���`�Ə����G���[�̌��o
	for(std::vector<CCell>::iterator it = cells.begin(); it != cells.end(); ) {
		// �������i+�̒��O�̍������������邢��")"�ȊO�̉��Z�q�@���̍��͏����Ă悢�j
		if (it->value_GetType() == F_TAG_PLUS) {
			if (it == cells.begin()) {
				it = cells.erase(it);
				continue;
			}
			std::vector<CCell>::iterator	itm = it;
			itm--;
			if (itm->value_GetType() != F_TAG_NOP && itm->value_GetType() != F_TAG_BRACKETOUT) {
				it = cells.erase(it);
				continue;
			}
		}
		// �������i-�̒��O�̍��������@���̍��ɒP����-��t������j
		if (it->value_GetType() == F_TAG_MINUS) {
			if (it == cells.begin()) {
				it = cells.erase(it);
				it->value().s_value.insert(0, L"-");
				continue;
			}
			std::vector<CCell>::iterator	itm = it;
			itm--;
			if (itm->value_GetType() != F_TAG_NOP && itm->value_GetType() != F_TAG_BRACKETOUT) {
				it = cells.erase(it);
				it->value().s_value.insert(0, L"-");
				continue;
			}
		}
		// �f�N�������g �����Ȍ`�󂩂������̏�A- �Ǝ����ւ�-�t�^�Ƃ��ď�������
		if (it->value_GetType() == F_TAG_DECREMENT) {
			if (it == cells.begin()) {
				vm.logger().Error(E_E, 65, dicfilename, linecount);
				return 0;
			}
			it->value_SetType(F_TAG_MINUS);
			it++;
			if (it == cells.end()) {
				vm.logger().Error(E_E, 66, dicfilename, linecount);
				return 0;
			}
			it->value().s_value.insert(0, L"-");
			continue;
		}
		// ���̍���
		it++;
	}
	// ","��"||"�ցA"-"��"&&"�֕ϊ�����@�����ȉ��Z�q�ł̓G���[
	for(std::vector<CCell>::iterator it = cells.begin(); it != cells.end(); it++) {
		if (it->value_GetType() == F_TAG_COMMA)
			it->value_SetType(F_TAG_OR);
		else if (it->value_GetType() == F_TAG_MINUS)
			it->value_SetType(F_TAG_AND);
		else if (it->value_GetType() >= F_TAG_ORIGIN && it->value_GetType() < F_TAG_ORIGIN_VALUE) {
			vm.logger().Error(E_E, 50, dicfilename, linecount);
			return 0;
		}
	}

	return 1;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CParser0::StructFormulaCell
 *  �@�\�T�v�F  ������𐔎��̍��Ɖ��Z�q�ɕ������܂�
 *
 *  �Ԓl�@�@�F  0/1=�G���[/����
 *
 *  �n���ꂽ������͂��̊֐��Ŕj�󂳂�܂�
 * -----------------------------------------------------------------------
 */
//#include <iostream>
void	CParser0::StructFormulaCell(yaya::string_t &str, std::vector<CCell> &cells)
{
	yaya::string_t cell_name;
	yaya::string_t bstr;

	for( ; ; ) {
		// �����ʒu���擾�@�ł���O�ōł����O�������A�N�H�[�g����Ă��Ȃ����Z�q��T��
		int	tagpoint = -1;
		int	tagtype  = 0;
		int	taglen   = 0;
		
//		wcout << endl << "str: " << str << endl;
//		wcout << "WordMatch:" << endl;
		int in_dq = 0;
		int	in_sq = 0;
		size_t strlen = str.size();
		for(size_t i = 0; i < strlen; ++i) {
			if (str[i] == L'\"') {
				if (!in_sq)
					in_dq = 1 - in_dq;
				continue;
			}
			if (str[i] == L'\'') {
				if (!in_dq)
					in_sq = 1 - in_sq;
				continue;
			}
			if (in_dq || in_sq)
				continue;

			int result = -1;
			int maxlen = 0;
			for ( size_t r = 0 ; r < FORMULATAG_NUM ; ++r ) {
				if ( static_cast<size_t>(formulatag_len[r]) <= strlen - i ) {
					if ( str.compare(i,formulatag_len[r],formulatag[r]) == 0 ) {
						if ( maxlen < formulatag_len[r] ) {
							result = r;
							maxlen = formulatag_len[r];
						}
					}
				}
			}

			if ( result >= 0 ) {
				tagtype = result;
				taglen  = formulatag_len[tagtype];
				
				bstr.assign(str, i + taglen, str.size());
				CutSpace(bstr);

				tagpoint = i;

//				wcout << "d0: " << d0 << endl;
//				wcout << "d1: " << d1 << endl;
				break;
			}
		}
/*		wcout << "  tagpoint: " << tagpoint << endl;
		wcout << "  tagtype: " << tagtype << endl;
		wcout << "  taglen: " << taglen << endl;
		wcout << "  bstr: " << bstr << endl;

		wcout << "Aya5:" << endl;
		tagpoint = -1;
		tagtype  = 0;
		taglen   = 0;

		for(size_t i = 0; i < FORMULATAG_NUM; i++) {
		yaya::string_t	d0, d1;
			if (!Split_IgnoreDQ(str, d0, d1, (wchar_t *)formulatag[i]))
				continue;
			int	d_point = d0.size();
			if (tagpoint == -1 ||
				(tagpoint != -1 && ((tagpoint > d_point) || tagpoint == d_point && taglen < formulatag_len[i]))) {
				tagpoint = d_point;
				tagtype  = i;
				taglen   = formulatag_len[i];
				bstr = d1;
//				wcout << "d0: " << d0 << endl;
//				wcout << "d1: " << d1 << endl;
			}
		}

		wcout << "  tagpoint: " << tagpoint << endl;
		wcout << "  tagtype: " << tagtype << endl;
		wcout << "  taglen: " << taglen << endl;
		wcout << "  bstr: " << bstr << endl;
*/
		// ������Ȃ��ꍇ�͍Ō�̍���o�^���Ĕ�����
		if (tagpoint == -1) {
			CCell	addcell(F_TAG_NOP);
			addcell.value().s_value = str;
			CutSpace(addcell.value().s_value);
			if (addcell.value_const().s_value.size())
				cells.push_back(addcell);
			break;
		}
		// ���������̂œo�^����
		else {
			// ���̓o�^�@�󕶎���͓o�^���Ȃ�
			if (tagpoint > 0) {
				cell_name.assign(str, 0, tagpoint);
				CutSpace(cell_name);
				if (cell_name.length()) {
					CCell	addcell(F_TAG_NOP);
					addcell.value().s_value = cell_name;
					cells.push_back(addcell);
				}
			}
			// ���Z�q�̓o�^
			cells.push_back(CCell(tagtype + F_TAG_ORIGIN));
			// ���̕����񂩂���o���ς̗v�f�����
			str = bstr;
		}
	}
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CParser0::AddSimpleIfBrace
 *  �@�\�T�v�F  if/elseif/else/when/others�̒����'{'�������ꍇ�A���̍s��{}�ň݂͂܂�
 *
 *  �Ԓl�@�@�F  1/0=�G���[/����
 * -----------------------------------------------------------------------
 */
char	CParser0::AddSimpleIfBrace(void)
{
	for(std::vector<CFunction>::iterator it = vm.function().begin(); it != vm.function().end(); it++) {
		int	beftype = ST_UNKNOWN;
		for(std::vector<CStatement>::iterator it2 = it->statement.begin(); it2 != it->statement.end(); it2++) {
			if (beftype == ST_IF ||
				beftype == ST_ELSEIF ||
				beftype == ST_ELSE ||
				beftype == ST_WHEN) {
				if (it2->type != ST_OPEN) {
					// { �ǉ�
					it2 = it->statement.insert(it2, CStatement(ST_OPEN, it2->linecount));
					it2 += 2;
					// } �ǉ�
					it2 = it->statement.insert(it2, CStatement(ST_CLOSE, it2->linecount));
				}
			}
			beftype = it2->type;
		}
	}
			
	return 0;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CParser0::SetCellType
 *  �@�\�T�v�F  �����̍��̎�ʁi���e�����A�֐��A�ϐ�...�Ȃǁj�𒲂ׂĊi�[���Ă����܂�
 *
 *  �Ԓl�@�@�F  1/0=�G���[/����
 * -----------------------------------------------------------------------
 */
char	CParser0::SetCellType(void)
{
	int	errorflg = 0;

	for(std::vector<CFunction>::iterator it = vm.function().begin(); it != vm.function().end(); it++)
		for(std::vector<CStatement>::iterator it2 = it->statement.begin(); it2 != it->statement.end(); it2++) {
			// �����ȊO�͔�΂�
			if (it2->type < ST_FORMULA)
				continue;

			if ( it2->cell_size() ) { //�������p
				for(std::vector<CCell>::iterator it3 = it2->cell().begin(); it3 != it2->cell().end(); it3++) {
					// ���Z�q�͔�΂�
					if (it3->value_GetType() != F_TAG_NOP)
						continue;

					// ����ʎ擾
					errorflg += SetCellType1(*it3, 0, it->dicfilename, it2->linecount);
					// when�̏ꍇ�A���̓��e�����������蓾�Ȃ�
					if (it2->type == ST_WHEN) {
						if (it3->value_GetType() != F_TAG_INT && 
							it3->value_GetType() != F_TAG_DOUBLE && 
							it3->value_GetType() != F_TAG_STRING && 
							it3->value_GetType() != F_TAG_STRING_PLAIN && 
							it3->value_GetType() != F_TAG_DOUBLE) {
							vm.logger().Error(E_E, 45, it->dicfilename, it2->linecount);
							errorflg++;
						}
					}
				}
			}
		}
	
	return (errorflg) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CParser0::SetCellType1
 *  �@�\�T�v�F  �n���ꂽ���̎�ʁi���e�����A�֐��A�ϐ�...�Ȃǁj��ݒ肵�܂�
 *  �����@�@�F  emb 0/1=�ʏ�̐����̍�/������ɖ��ߍ��܂�Ă��������̍�
 *
 *  �Ԓl�@�@�F  1/0=�G���[/����
 * -----------------------------------------------------------------------
 */
char	CParser0::SetCellType1(CCell& scell, char emb, const yaya::string_t& dicfilename, int linecount)
{
	// �֐�
	int	i = GetFunctionIndexFromName(scell.value_const().s_value);
	if(i != -1) {
		scell.value_SetType(F_TAG_USERFUNC);
		scell.index     = i;
		scell.value_Delete();
		return 0;
	}

/*
	int i = 0;
	for(std::vector<CFunction>::iterator it = vm.function().begin(); it != vm.function().end(); it++, i++)
		if (!scell.value_const().s_value.compare(it->name)) {
			scell.value_SetType(F_TAG_USERFUNC);
			scell.index     = i;
			scell.value_Delete();
//			wcout << "Aya5:" << endl;
//			wcout << "  result: " << i << endl;
			return 0;
		}
*/

	// �V�X�e���֐�
	int sysidx = CSystemFunction::FindIndex(scell.value_const().s_value);
	if ( sysidx >= 0 ) {
		scell.value_SetType(F_TAG_SYSFUNC);
		scell.index     = sysidx;
		scell.value_Delete();
		return 0;
	}

	// �������e����(DEC)
	if (IsIntString(scell.value_const().s_value)) {
		scell.value() = yaya::ws_atoi(scell.value_const().s_value, 10);
		return 0;
	}
	// �������e����(BIN)
	if (IsIntBinString(scell.value_const().s_value, 1)) {
		scell.value() = yaya::ws_atoi(scell.value_const().s_value, 2);
		return 0;
	}
	// �������e����(HEX)
	if (IsIntHexString(scell.value_const().s_value, 1)) {
		scell.value() = yaya::ws_atoi(scell.value_const().s_value, 16);
		return 0;
	}
	// �������e����
	if (IsDoubleButNotIntString(scell.value_const().s_value)) {
		scell.value() = yaya::ws_atof(scell.value_const().s_value);
		return 0;
	}
	// �����񃊃e����(�_�u���N�H�[�g)
	i = IsLegalStrLiteral(scell.value_const().s_value);
	if (!i) {
		CutDoubleQuote(scell.value().s_value);
		if (!emb)
			scell.value_SetType(F_TAG_STRING);
		else {
			if (!scell.value_const().s_value.size())
				scell.value_SetType(F_TAG_STRING);
			else {
				if (scell.value_const().s_value[0] == L'%') {
					if (scell.value_const().s_value.size() == 1)
						scell.value_SetType(F_TAG_STRING);
					else {
						scell.value().s_value.erase(0, 1);
						scell.value_SetType(F_TAG_STRING_EMBED);
					}
				}
				else
					scell.value_SetType(F_TAG_STRING);
			}
		}
		return 0;
	}
	else if (i == 1) {
		scell.value_Delete();
		vm.logger().Error(E_E, 7, scell.value_const().s_value, dicfilename, linecount);
		return 1;
	}
	else if (i == 2) {
		scell.value_Delete();
		vm.logger().Error(E_E, 8, scell.value_const().s_value, dicfilename, linecount);
		return 1;
	}
	// �����񃊃e����(�V���O���N�H�[�g)
	i = IsLegalPlainStrLiteral(scell.value_const().s_value);
	if (!i) {
		CutSingleQuote(scell.value().s_value);
		scell.value_SetType(F_TAG_STRING_PLAIN);
		return 0;
	}
	else if (i == 1) {
		scell.value_Delete();
		vm.logger().Error(E_E, 7, scell.value_const().s_value, dicfilename, linecount);
		return 1;
	}
	else if (i == 2) {
		scell.value_Delete();
		vm.logger().Error(E_E, 93, scell.value_const().s_value, dicfilename, linecount);
		return 1;
	}
	// �����܂Ŏc�������͕̂ϐ��̌��
	CVariable	addvariable;
	std::string	errstr;
	switch(IsLegalVariableName(scell.value_const().s_value)) {
	case 0:
		// �O���[�o���ϐ�
		scell.value_SetType(F_TAG_VARIABLE);
		scell.index     = vm.variable().Make(scell.value_const().s_value, 1);
		scell.value_Delete();
		return 0;
	case 16:
		// ���[�J���ϐ�
		scell.value_SetType(F_TAG_LOCALVARIABLE);
		scell.name      = scell.value_const().s_value;
		scell.value_Delete();
		return 0;
	case 3:
		// �G���[�@�ϐ����̌��
		vm.logger().Error(E_E, 10, scell.value_const().s_value, dicfilename, linecount);
		scell.value_Delete();
		return 1;
	case 4:
		// �G���[�@�g�p�ł��Ȃ��������܂�
		vm.logger().Error(E_E, 11, scell.value_const().s_value, dicfilename, linecount);
		scell.value_Delete();
		return 1;
	case 6:
		// �G���[�@�\���ƏՓ�
		vm.logger().Error(E_E, 12, scell.value_const().s_value, dicfilename, linecount);
		scell.value_Delete();
		return 1;
	default:
		// �����G���[
		vm.logger().Error(E_E, 9, scell.value_const().s_value, dicfilename, linecount);
		scell.value_Delete();
		return 1;
	};
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CParser0::MakeCompleteFormula
 *  �@�\�T�v�F  ���ߍ��ݗv�f�̎��ւ̓W�J�Ɖ��Z�����̌�����s�Ȃ��܂�
 *
 *  �Ԓl�@�@�F  1/0=�G���[/����
 * -----------------------------------------------------------------------
 */
char	CParser0::MakeCompleteFormula(void)
{
	int	errcount = 0;

	errcount += ParseEmbeddedFactor();
	ConvertPlainString();
	errcount += CheckDepthAndSerialize();

	return (errcount) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CParser0::ParseEmbeddedFactor
 *  �@�\�T�v�F  "%"�Ŗ��ߍ��܂ꂽ�v�f����������𕪉����Đ��������A���̎��ƌ������܂�
 *
 *  �Ԓl�@�@�F  1/0=�G���[/����
 * -----------------------------------------------------------------------
 */
char	CParser0::ParseEmbeddedFactor(void)
{
	int	errcount = 0;

	for(std::vector<CFunction>::iterator it = vm.function().begin(); it != vm.function().end(); it++)
		for(std::vector<CStatement>::iterator it2 = it->statement.begin(); it2 != it->statement.end(); it2++)
		    errcount += ParseEmbeddedFactor1(*it2, it->dicfilename);

	return (errcount) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CParser0::ParseEmbeddedFactor1
 *  �@�\�T�v�F  "%"�Ŗ��ߍ��܂ꂽ�v�f����������𕪉����Đ��������A���̎��ƌ������܂�
 *
 *  �Ԓl�@�@�F  1/0=�G���[/����
 * -----------------------------------------------------------------------
 */
char	CParser0::ParseEmbeddedFactor1(CStatement& st, const yaya::string_t& dicfilename)
{
	if (st.type < ST_FORMULA)
		return 0;

	int	errcount = 0;

	// ���Z����������Ȃ��悤�ɂ��邽�߁A%���܂ޗv�f��()�ň͂�
	if ( st.cell_size() ) { //�������p
		for(std::vector<CCell>::iterator it = st.cell().begin(); it != st.cell().end(); ) {
			if (it->value_GetType() == F_TAG_STRING) {
				if (it->value_const().s_value.find(L'%') != yaya::string_t::npos) {
					
					it = st.cell().insert(it, CCell(F_TAG_BRACKETIN) );
					it += 2;
					
					it = st.cell().insert(it, CCell(F_TAG_BRACKETOUT) );
					it++;

					if (st.type == ST_WHEN) {
						vm.logger().Error(E_E, 46, dicfilename, st.linecount);
						errcount++;
					}
					continue;
				}
			}
			// ����
			it++;
		}
	}

	// ���ߍ��ݗv�f�����Z�������ɕ������Č��̎��̊Y���ʒu�֑}��
	if ( st.cell_size() ) { //�������p
		for(std::vector<CCell>::iterator it = st.cell().begin(); it != st.cell().end(); ) {
			if (it->value_GetType() == F_TAG_STRING) {
				if (it->value_const().s_value.find(L'%') != yaya::string_t::npos) {
					// ���Z�������֕ϊ�
					int	t_errcount = 0;
					yaya::string_t	linedata = it->value_const().s_value;
					int	res = ConvertEmbedStringToFormula(linedata, dicfilename, st.linecount);
					t_errcount += res;
					if (res) {
						it++;
						continue;
					}
					// �����̍������Ɛ��`
					std::vector<CCell>	addcells;
					res = 1 - StructFormula(linedata, addcells, dicfilename, st.linecount);
					t_errcount += res;
					if (res) {
						it++;
						continue;
					}
					// ���̎�ʂ�ݒ�
					for(std::vector<CCell>::iterator it2 = addcells.begin(); it2 != addcells.end(); it2++) {
						if (it2->value_GetType() != F_TAG_NOP)
							continue;

						t_errcount += SetCellType1(*it2, 1, dicfilename, st.linecount);
					}
					// ���̎��̊Y���ʒu�֑}��
					if (!t_errcount) {
						it = st.cell().erase(it);
						int	c_num = addcells.size();
						for(int i = c_num - 1; i >= 0; i--)
							it = st.cell().insert(it, addcells[i]);
						it += c_num;
						continue;
					}
					errcount += t_errcount;
				}
			}
			// ����
			it++;
		}
	}

	return (errcount) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CParser0::ConvertPlainString
 *  �@�\�T�v�F  �V���O���N�H�[�g�������ʏ핶����֒u�����܂�
 * -----------------------------------------------------------------------
 */
void	CParser0::ConvertPlainString(void)
{
	for(std::vector<CFunction>::iterator it = vm.function().begin(); it != vm.function().end(); it++)
		for(std::vector<CStatement>::iterator it2 = it->statement.begin(); it2 != it->statement.end(); it2++)
		    ConvertPlainString1(*it2, it->dicfilename);
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CParser0::ConvertPlainString1
 *  �@�\�T�v�F  �V���O���N�H�[�g�������ʏ핶����֒u�����܂�
 * -----------------------------------------------------------------------
 */
void	CParser0::ConvertPlainString1(CStatement& st, const yaya::string_t& /*dicfilename*/)
{
	if (st.type < ST_FORMULA)
		return;

	if ( st.cell_size() ) { //�������p
		for(std::vector<CCell>::iterator it = st.cell().begin(); it != st.cell().end(); it++)
			if (it->value_GetType() == F_TAG_STRING_PLAIN)
				it->value_SetType(F_TAG_STRING);
	}
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CParser0::ConvertEmbedStringToFormula
 *  �@�\�T�v�F  "%"�Ŗ��ߍ��܂ꂽ�v�f��������������Z�������֕ϊ����܂�
 *
 *  �Ԓl�@�@�F  1/0=�G���[/����
 * -----------------------------------------------------------------------
 */
char	CParser0::ConvertEmbedStringToFormula(yaya::string_t& str, const yaya::string_t& dicfilename, int linecount)
{
	yaya::string_t	resstr;
	int	nindex = -1;
	for(int nfirst = 0; ; nfirst++) {
		// "%"�̔���
		int	p_pers = str.find(L'%', 0);
		if (p_pers == -1) {
			vm.logger().Error(E_E, 55, dicfilename, linecount);
			return 1;
		}
		// ���Z���Z�q��ǉ�
		if (nfirst)
			resstr += L"+";
		// ��s���镶���񍀂�ǉ�
		if (p_pers > 0) {
			yaya::string_t	prestr;
			prestr.assign(str, 0, p_pers);
			AddDoubleQuote(prestr);
			resstr += prestr;
			str.erase(0, p_pers);
			resstr += L"+";
		}
		// "%"�����c��Ȃ������炻��ŏI���
		if (str.size() == 1) {
			resstr += L"\"%\"";
			break;
		}
		// "%"�̎���"("�Ȃ璷���w��t���̖��ߍ��݂Ȃ̂ł���𔲂��o��
		if (str[1] == L'(') {
			// �����o���ʒu����
			int	bdepth = 1;
			int	len = str.size();
			int     spos = 0;
			for(spos = 2; spos < len; spos++) {
				bdepth += ((str[spos] == L'(') - (str[spos] == L')'));
				if (!bdepth)
					break;
			}
			if (spos < len) {
				spos++;
			}
			// �G���[����
			if (bdepth != 0) {
				vm.logger().Error(E_E, 60, dicfilename, linecount);
				return 1;
			}
			if (spos == 2) {
				vm.logger().Error(E_E, 61, dicfilename, linecount);
				return 1;
			}
			else if (spos < 2) {
				vm.logger().Error(E_E, 62, dicfilename, linecount);
				return 1;
			}
			// ���ߍ��ݗv�f�����o���Ēǉ�
			yaya::string_t	embedstr;
			embedstr.assign(str, 1, spos - 1);
			resstr += L"TOSTR";
			resstr += embedstr;
			str.erase(0, spos);
//			nindex++;
			// ����"%"��T���Ă݂�
			p_pers = str.find(L'%', 0);
			// ������Ȃ���΂��ꂪ�Ō�̕�����萔��
			if (p_pers == -1) {
				embedstr = str;
				if (embedstr.size()) {
					AddDoubleQuote(embedstr);
					resstr += L"+";
					resstr += embedstr;
				}
				break;
			}
			continue;
		}
		// "%"�̎���"["�Ȃ猋�ʂ̍ė��p�i%[n]�j�Ȃ̂ł���𔲂��o��
		if (str[1] == L'[') {
			// �܂���s���鍀�������Ȃ�G���[
			if (nindex == -1) {
				vm.logger().Error(E_E, 81, dicfilename, linecount);
				return 1;
			}
			// �����o���ʒu����
			int	bdepth = 1;
			int	len = str.size();
			int     spos = 0;
			for(spos = 2; spos < len; spos++) {
				bdepth += ((str[spos] == L'[') - (str[spos] == L']'));
				if (!bdepth)
					break;
			}
			if (spos < len) {
				spos++;
			}
			// �G���[����
			if (bdepth != 0) {
				vm.logger().Error(E_E, 78, dicfilename, linecount);
				return 1;
			}
			if (spos == 2) {
				vm.logger().Error(E_E, 79, dicfilename, linecount);
				return 1;
			}
			else if (spos < 2) {
				vm.logger().Error(E_E, 80, dicfilename, linecount);
				return 1;
			}
			// ���ߍ��ݗv�f�����o���A"���ʂ̍ė��p�������s���֐�"�Ƃ��Ēǉ�
			resstr += CSystemFunction::HistoryFunctionName();
			resstr += L"(";
			
			resstr += yaya::ws_itoa(nindex, 10);
			resstr += L"-(";

			resstr.append(str, 2, spos - 3);
			resstr += L"))";

			str.erase(0, spos);
			// ����"%"��T���Ă݂�
			p_pers = str.find(L'%', 0);
			// ������Ȃ���΂��ꂪ�Ō�̕�����萔��
			if (p_pers == -1) {
				yaya::string_t embedstr;
				embedstr = str;
				if (embedstr.size()) {
					AddDoubleQuote(embedstr);
					resstr += L"+";
					resstr += embedstr;
				}
				break;
			}
			continue;
		}
		// �ʏ��"%"
		else {
			// ����"%"�𔭌�
			p_pers = str.find(L'%', 1);
			nindex++;
			// ������Ȃ���΂��ꂪ�Ō�̍�
			if (p_pers == -1) {
				yaya::string_t	embedstr = str;
				AddDoubleQuote(embedstr);
				resstr += embedstr;
				break;
			}
			// ���������̂Œǉ�
			yaya::string_t	embedstr;
			embedstr.assign(str, 0, p_pers);
			AddDoubleQuote(embedstr);
			resstr += embedstr;
			str.erase(0, p_pers);
		}
	}

	// �Ԃ�
	str = resstr;
	return 0;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CParser0::CheckDepthAndSerialize
 *  �@�\�T�v�F  �����̃J�b�R�����������Ă��邩�����A����ю��̉��Z���������߂܂�
 *
 *  �Ԓl�@�@�F  1/0=�G���[/����
 * -----------------------------------------------------------------------
 */
char	CParser0::CheckDepthAndSerialize(void)
{
	int	errcount = 0;

	// �����̃J�b�R����
	for(std::vector<CFunction>::iterator it = vm.function().begin(); it != vm.function().end(); it++)
		for(std::vector<CStatement>::iterator it2 = it->statement.begin(); it2 != it->statement.end(); it2++) {
			if (it2->type < ST_FORMULA)
				continue;

			errcount += CheckDepth1(*it2, it->dicfilename);
		}

	// when��if�ϊ��̍ŏI�����@���̐�����if�ŏ����\�Ȕ��莮�ɐ��`����
	errcount += MakeCompleteConvertionWhenToIf();

	// ���Z�����̌���
	for(std::vector<CFunction>::iterator it = vm.function().begin(); it != vm.function().end(); it++)
		for(std::vector<CStatement>::iterator it2 = it->statement.begin(); it2 != it->statement.end(); it2++) {
			if (it2->type < ST_FORMULA)
				continue;

			errcount += CheckDepthAndSerialize1(*it2, it->dicfilename);
		}

	return (errcount) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CParser0::MakeCompleteConvertionWhenToIf
 *  �@�\�T�v�F  when��if�ϊ��̍ŏI�������s���܂��B���̐�����if�ŏ����\�Ȕ��莮�ɐ��`���܂�
 *
 *  �Ԓl�@�@�F  1/0=�G���[/����
 * -----------------------------------------------------------------------
 */
char	CParser0::MakeCompleteConvertionWhenToIf(void)
{
	int	errcount = 0;

	for(std::vector<CFunction>::iterator it = vm.function().begin(); it != vm.function().end(); it++) {
		std::vector<yaya::string_t>	caseary;
		yaya::string_t	dmystr = L"";
		caseary.push_back(dmystr);
		std::vector<int>		whencnt;
		whencnt.push_back(0);
		int	depth = 0;
		for(std::vector<CStatement>::iterator it2 = it->statement.begin(); it2 != it->statement.end(); it2++) {
			if (depth == -1) {
				vm.logger().Error(E_E, 52, it->dicfilename, it2->linecount);
				errcount++;
				break;
			}
			// {
			if (it2->type == ST_OPEN) {
				depth++;
				yaya::string_t	dmystr = L"";
				caseary.push_back(dmystr);
				whencnt.push_back(0);
				continue;
			}
			// }
			if (it2->type == ST_CLOSE) {
				caseary[depth] = L"";
				whencnt[depth] = 0;
				depth--;
				continue;
			}
			// case
			if (it2->type == ST_FORMULA) {
				if (it2->cell_size() >= 2) {
					if (it2->cell()[0].value_GetType() == F_TAG_LOCALVARIABLE &&
						it2->cell()[1].value_GetType() == F_TAG_EQUAL) {
						if (!::wcsncmp(PREFIX_CASE_VAR,
							it2->cell()[0].name.c_str(),PREFIX_CASE_VAR_SIZE)) {
							caseary[depth] = it2->cell()[0].name;
							whencnt[depth] = 0;
							continue;
						}
					}
				}
			}
			// when
			if (it2->type == ST_WHEN) {
				int	depthm1 = depth - 1;
				if (depthm1 < 0) {
					vm.logger().Error(E_E, 64, it->dicfilename, it2->linecount);
					errcount++;
					break;
				}
				if (!caseary[depthm1].size()) {
					vm.logger().Error(E_E, 63, it->dicfilename, it2->linecount);
					errcount++;
					break;
				}
				// if/elseif�֕ϊ�
				if (!whencnt[depthm1])
					it2->type = ST_IF;
				else
					it2->type = ST_ELSEIF;
				(whencnt[depthm1])++;
				// ���̐����𔻒莮�ɏ���������
				int	i = 0;

				if ( it2->cell_size() ) { //�������p
					for(std::vector<CCell>::iterator it3 = it2->cell().begin(); it3 != it2->cell().end(); ) {
						// ���x��
						if (!i) {
							if (it3->value_GetType() != F_TAG_INT && 
								it3->value_GetType() != F_TAG_DOUBLE && 
								it3->value_GetType() != F_TAG_STRING && 
								it3->value_GetType() != F_TAG_DOUBLE) {
								vm.logger().Error(E_E, 53, it->dicfilename, it2->linecount);
								errcount++;
								break;
							}
							i = 1;
							it3++;
							// �Ō�̍��@����͕K��==����
							if (it3 == it2->cell().end()) {
								CCell	addcell1(F_TAG_LOCALVARIABLE);
								addcell1.name    = caseary[depthm1];
								addcell1.value_Delete();
								it3 = it2->cell().insert(it3, addcell1);
								CCell	addcell2(F_TAG_IFEQUAL);
								it3 = it2->cell().insert(it3, addcell2);
								break;
							}
							continue;
						}
						// or/and
						i = 0;
						if (it3->value_GetType() == F_TAG_OR) {
							// or
							CCell	addcell1(F_TAG_LOCALVARIABLE);
							addcell1.name      = caseary[depthm1];
							addcell1.value_Delete();
							it3 = it2->cell().insert(it3, addcell1);
							CCell	addcell2(F_TAG_IFEQUAL);
							it3 = it2->cell().insert(it3, addcell2);
							if (it3 == it2->cell().end())
								break;
							it3++;
							if (it3 == it2->cell().end())
								break;
							it3++;
							if (it3 == it2->cell().end())
								break;
							it3++;
						}
						else if (it3->value_GetType() == F_TAG_AND) {
							// and
							CCell	addcell1(F_TAG_LOCALVARIABLE);
							addcell1.name      = caseary[depthm1];
							addcell1.value_Delete();
							it3 = it2->cell().insert(it3, addcell1);
							CCell	addcell2(F_TAG_IFLTEQUAL);
							it3 = it2->cell().insert(it3, addcell2);
							if (it3 == it2->cell().end())
								break;
							it3++;
							if (it3 == it2->cell().end())
								break;
							it3++;
							if (it3 == it2->cell().end())
								break;
							it3++;
							if (it3 == it2->cell().end())
								break;
							CCell	addcell3(F_TAG_IFLTEQUAL);
							it3 = it2->cell().insert(it3, addcell3);
							CCell	addcell4(F_TAG_LOCALVARIABLE);
							addcell4.name      = caseary[depthm1];
							addcell4.value_Delete();
							it3 = it2->cell().insert(it3, addcell4);
							if (it3 == it2->cell().end())
								break;
							it3++;
							if (it3 == it2->cell().end())
								break;
							it3++;
							if (it3 == it2->cell().end())
								break;
							it3++;
							if (it3 == it2->cell().end())
								break;
							it3++;
						}
						else {
							vm.logger().Error(E_E, 54, it->dicfilename, it2->linecount);
							errcount++;
							break;
						}
					}
				}
			}
		}
	}

	return (errcount) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CParser0::CheckDepth1
 *  �@�\�T�v�F  �����̃J�b�R�����������Ă��邩�������܂�
 *
 *  �Ԓl�@�@�F  1/0=�G���[/����
 * -----------------------------------------------------------------------
 */
char	CParser0::CheckDepth1(CStatement& st, const yaya::string_t& dicfilename)
{
	// ()/[]�̑Ή��Â�������
	std::vector<int>	hb_depth;
	int	depth = 0;
	if ( st.cell_size() ) { //�������p
		for(std::vector<CCell>::iterator it = st.cell().begin(); it != st.cell().end(); it++) {
			if (it->value_GetType() == F_TAG_BRACKETIN)
				depth++;
			else if (it->value_GetType() == F_TAG_BRACKETOUT)
				depth--;
			else if (it->value_GetType() == F_TAG_HOOKBRACKETIN)
				hb_depth.push_back(depth);
			else if (it->value_GetType() == F_TAG_HOOKBRACKETOUT) {
				int	gb_depth_size = hb_depth.size();
				if (!gb_depth_size) {
					vm.logger().Error(E_E, 20, dicfilename, st.linecount);
					return 1;
				}
				else if (hb_depth[gb_depth_size - 1] != depth) {
					vm.logger().Error(E_E, 20, dicfilename, st.linecount);
					return 1;
				}
				hb_depth.erase(hb_depth.end() - 1);
			}
		}
	}
	if (depth) {
		vm.logger().Error(E_E, 19, dicfilename, st.linecount);
		return 1;
	}

	return 0;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CParser0::CheckDepthAndSerialize1
 *  �@�\�T�v�F  �����̉��Z���������肵�܂�
 *
 *  �Ԓl�@�@�F  1/0=�G���[/����
 * -----------------------------------------------------------------------
 */
char	CParser0::CheckDepthAndSerialize1(CStatement& st, const yaya::string_t& dicfilename)
{
	// �ēx()����q�̑Ή����������A���Z�����Z�o�p�̃t���O���쐬����
	// �t���Odepthvec�͑Ή����鍀�̏�����Ԃ������Ă���B
	// -1    �����ς������͏����Ώۂł͂Ȃ���
	// -2    �����҂��̍�
	// 0�ȏ� ���Z�q�A�������傫���قǗD��x������

	//������int�łȂ��Ƃ��߁c�ci--�̃��[�v�ŕ��i��0xFFFFFFFF�ɂȂ���unsigned���Ƌ���Ȓl�j�ɂȂ�\��������
	int sz = static_cast<int>(st.cell_size());
	int i = 0;

	std::vector<int>	depthvec;
	depthvec.reserve(sz);

	int depth = 0;
	for(i = 0; i < sz; i++) {
		// ���Z�q
		int	type = st.cell()[i].value_GetType();
		if (type >= F_TAG_ORIGIN && type < F_TAG_ORIGIN_VALUE) {
			if (type == F_TAG_BRACKETIN ||
				type == F_TAG_BRACKETOUT ||
				type == F_TAG_HOOKBRACKETIN ||
				type == F_TAG_HOOKBRACKETOUT) {
				depth += formulatag_depth[type];
				depthvec.push_back(-1);
			}
			else
				depthvec.push_back(depth + formulatag_depth[type]);
			continue;
		}
		// ���Z�q�ȊO
		depthvec.push_back(-2);
	}
	if (depth) {
		vm.logger().Error(E_E, 48, dicfilename, st.linecount);
		return 1;
	}
	if (sz != static_cast<int>(depthvec.size())) {
		vm.logger().Error(E_E, 21, dicfilename, st.linecount);
		return 1;
	}

	// ���Z�����̌���
	for( ; ; ) {
		// ���Z�Ώۂ̉��Z�q�𔭌�
		// ����[���̉��Z�q�̕���͍ŏ��̂��̂��I�΂��B�܂�AYA5�ł͉��Z�q�͏�ɍ�����E�֌��������
		// ���������� i=j=1 �� i=j; j=1 �Ɠ����ł���Bi �� 1 �͑������Ȃ��B�E���猋�������C/C++�Ƃ�
		// �����͈قȂ��Ă���B
		int	t_index = -1;
		int	t_depth = -1;
		for(i = 0; i < sz; i++)
			if (depthvec[i] > t_depth) {
				t_depth = depthvec[i];
				t_index = i;
			}
		// �Ώۂ������Ȃ����甲����
		if (t_depth == -1)
			break;

		// ��`�̊J�n�@���Z�q�̓o�^
		int	t_type = st.cell()[t_index].value_GetType();
		CSerial	addserial(t_index);
		depthvec[t_index] = -2;
		// ���ӂ̍����擾
		int	f_depth = 1;
		bool out_of_bracket = false;
		for(i = t_index - 1; i >= 0; i--) {
			// �J�b�R�[������
			if (st.cell()[i].value_GetType() == F_TAG_BRACKETIN ||
				st.cell()[i].value_GetType() == F_TAG_HOOKBRACKETIN)
				f_depth--;
			else if (st.cell()[i].value_GetType() == F_TAG_BRACKETOUT ||
				st.cell()[i].value_GetType() == F_TAG_HOOKBRACKETOUT)
				f_depth++;
			if (!f_depth) {
				out_of_bracket = true;
				break;
			}
			// �擾
			if (depthvec[i] == -2) {
				addserial.index.push_back(i);
				depthvec[i] = -1;
				break;
			}
		}
		if (out_of_bracket) {
			if (t_type == F_TAG_COMMA)
				vm.logger().Error(E_E, 23, dicfilename, st.linecount);
			else
				vm.logger().Error(E_E, 22, dicfilename, st.linecount);
			return 1;
		}
		// ���Z�q��","�̏ꍇ�A�����̂���ɍ��֌�����i�߁A�����������֐��ɓn�����߂̉��Z�q�Ɗ֐���
		// ���������ꍇ�͈������֐������ɏ���������B
		// �֐���������Ȃ��ꍇ�͒ʏ�̔z��Ƃ������ƂɂȂ�
		if (t_type == F_TAG_COMMA) {
			f_depth = 1;
			for( ; i >= 0; i--) {
				// �J�b�R�[������
				if (st.cell()[i].value_GetType() == F_TAG_BRACKETIN ||
					st.cell()[i].value_GetType() == F_TAG_HOOKBRACKETIN)
					f_depth--;
				else if (st.cell()[i].value_GetType() == F_TAG_BRACKETOUT ||
					st.cell()[i].value_GetType() == F_TAG_HOOKBRACKETOUT)
					f_depth++;
				if (!f_depth) {
					i--;
					break;
				}
			}
			if (i > 0) {
				if (st.cell()[i].value_GetType() == F_TAG_FUNCPARAM) {
					// �֐�
					depthvec[t_index] = -1;
					addserial.tindex = i;
					depthvec[i] = -2;
					i--;
					if (i < 0) {
						vm.logger().Error(E_E, 25, dicfilename, st.linecount);
						return 1;
					}
					if ((st.cell()[i].value_GetType() == F_TAG_SYSFUNC ||
						st.cell()[i].value_GetType() == F_TAG_USERFUNC) &&
						depthvec[i] == -2) {
						addserial.index.insert(addserial.index.begin(), i);
						depthvec[i] = -1;
					}
				}
			}
			//}
			//
			// �E�ӂ̍����擾�@���Z�q��","�̏ꍇ�͗񋓂��ꂽ���ׂĂ��ꊇ���Ď擾����
			//if (t_type == F_TAG_COMMA) {
			// ","
			int	gflg = 0;
			f_depth = 1;
			for(i = t_index + 1; i < sz; i++) {
				// �J�b�R�[������
				if (st.cell()[i].value_GetType() == F_TAG_BRACKETIN ||
					st.cell()[i].value_GetType() == F_TAG_HOOKBRACKETIN)
					f_depth++;
				else if (st.cell()[i].value_GetType() == F_TAG_BRACKETOUT ||
					st.cell()[i].value_GetType() == F_TAG_HOOKBRACKETOUT)
					f_depth--;
				if (!f_depth)
					break;
				// �擾
				if (depthvec[i] == -2) {
					addserial.index.push_back(i);
					depthvec[i] = -1;
					gflg = 1;
					continue;
				}
				else if (depthvec[i] == -1)
					continue;
				else if (st.cell()[i].value_GetType() == F_TAG_COMMA) {
					depthvec[i] = -1;
					continue;
				}

				break;
			}
			if (!gflg) {
				vm.logger().Error(E_E, 24, dicfilename, st.linecount);
				return 1;
			}
		}
		else {
			// ","�ȊO
			for(i = t_index + 1; i < sz; i++) {
				if (depthvec[i] == -2) {
					addserial.index.push_back(i);
					depthvec[i] = -1;
					break;
				}
			}
			if (i == sz) {
				vm.logger().Error(E_E, 22, dicfilename, st.linecount);
				return 1;
			}
		}

		// ���Z��`��o�^
		st.serial().push_back(addserial);
	}

	// ���Z���������肷��ƁA�����������ЂƂ����c�邱�ƂɂȂ�i���ꂪ���ʁj�B������m�F����
	//
	// �����������������Z�q�łȂ��ꍇ�́A�L���ȍ���1�����Ȃ����߉��Z�������������Ƃ��Ӗ����Ă���B
	// ���̂܂܂ł͌��ʂ������Ȃ��̂ŁA�u�c���������猋�ʂ𓾂�v���Ƃ��w������t���O��ǉ�����
	int	scount = 0;
	for(i = 0; i < sz; i++) {
		if (depthvec[i] == -2) {
			scount++;
			if (st.cell()[i].value_GetType() >= F_TAG_ORIGIN_VALUE) {
				CSerial	addserial(i);
				addserial.index.push_back(0);	// dmy
				addserial.index.push_back(0);	// dmy
				st.serial().push_back(addserial);
			}
		}
	}
	if (scount != 1) {
		vm.logger().Error(E_E, 86, dicfilename, st.linecount);
		return 1;
	}

	return 0;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CParser0::GetFunctionIndexFromName
 *  �@�\�T�v�F  �֐����ɑΉ�����vm.function()�z��̏������擾���܂�
 * -----------------------------------------------------------------------
 */
int	CParser0::GetFunctionIndexFromName(const yaya::string_t& str)
{
	yaya::indexmap::const_iterator it = vm.functionmap().find(str);
	if ( it != vm.functionmap().end() ) {
		return it->second;
	}
	return -1;

/*	int i = vm.function_wm().search(str, 0);
	if((i != -1) && !vm.function()[i].name.compare(str)) {
		// str�̍ŏ����֐����Ƀ}�b�`�����ꍇ��WordMatch��-1�ȊO��Ԃ��̂ŁC
		// ���S��v���ǂ����ēx�`�F�b�N���K�v�D
		return i;
	}
	return -1;*/
/*
	int	sz = vm.function().size();
	for(int i = 0; i < sz; i++)
		if (!vm.function()[i].name.compare(str))
			return i;

	return -1;
*/
}


