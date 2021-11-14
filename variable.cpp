#include "variable.h"
#include "function.h"

const CValue& CVariable::call_watcher(CValue& save) {
	if (!this)
		return emptyvalue;
	if (watcher) {
		CValue arg(F_TAG_ARRAY, 0/*dmy*/);
		arg.array().emplace_back(this->name);
		arg.array().emplace_back(value_const());
		save = watcher->Execute(arg);
		return save;
	}
	else
		return value_const();
}

void CVariable::call_destorier()
{
	if(destorier)
		destorier->Execute();
}

void CVariable::call_setter(const CValue& var_before)
{
	if(setter){
		CValue arg(F_TAG_ARRAY, 0/*dmy*/);
		arg.array().emplace_back(this->name);
		arg.array().emplace_back(value_const());
		arg.array().emplace_back(var_before);
		setter->Execute(arg);
	}
}
