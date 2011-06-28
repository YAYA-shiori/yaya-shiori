// 
// AYA version 5
//
// AYA��1�C���X�^���X��ێ�����N���XAYAVM
// written by the Maintenance Shop/C.Ponapalt 2006
// 
// CAyaVM������������ƕ�����AYA��1�̃v���Z�X/�X���b�h/���W���[�����ő��点�邱�Ƃ��ł��܂��B
// 

#ifndef AYAVM_H
#define AYAVM_H

#include <vector>
#include <map>
#include <boost/shared_ptr.hpp>
#include "log.h"
#include "mt19937ar.h"

class CBasis;
class CFunction;
class CCallDepth;
class CSystemFunction;
class CGlobalVariable;
class CFile;
class CLib;
class CParser0;
class CParser1;

class CAyaVM
{
private:
	boost::shared_ptr<CBasis>					m_basis;

	boost::shared_ptr< std::vector<CFunction> >	m_function;
	boost::shared_ptr< yaya::indexmap > m_functionmap;

	boost::shared_ptr<CCallDepth>				m_calldepth;
	boost::shared_ptr<CSystemFunction>			m_sysfunction;
	boost::shared_ptr<CGlobalVariable>			m_variable;

	boost::shared_ptr<CFile>					m_files;
	boost::shared_ptr<CLib>						m_libs;

	boost::shared_ptr<CParser0>					m_parser0;
	boost::shared_ptr<CParser1>					m_parser1;

	CLog	m_logger;

public:
	void Load(void);
	void Unload(void);

	unsigned int genrand(void);
	int genrand_int(int n);

	// �吧��
	CBasis&					basis();

	// �֐�/�V�X�e���֐�/�O���[�o���ϐ�
	std::vector<CFunction>&	function();
	yaya::indexmap& functionmap();

	CCallDepth&				calldepth();
	CSystemFunction&		sysfunction();
	CGlobalVariable&		variable();

	// �t�@�C���ƊO�����C�u����
	CFile&					files();
	CLib&					libs();

	// ���K�[
	inline CLog& logger() {
		return m_logger;
	}

	// �p�[�T
	CParser0&				parser0();
	CParser1&				parser1();
};

#endif //AYAVM_H


