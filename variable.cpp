#include "variable.h"
#include "function.h"
#include "messages.h"
#include "ayavm.h"

const CValue& CVariable::call_watcher(CAyaVM&vm,CValue& save) {
	if (watcher.size()) {
		ptrdiff_t index = vm.function_exec().GetFunctionIndexFromName(watcher);

		if (index < 0) {
			vm.logger().Error(E_W, 12, watcher);
			save = watcher;
			//SetError(12);
			return save;
		}

		CFunction* it = &vm.function_exec().func[size_t(index)];

		CValue arg(F_TAG_ARRAY, 0/*dmy*/);
		arg.array().emplace_back(this->name);
		arg.array().emplace_back(value_const());

		save = it->Execute(arg);
		return save;
	}
	else
		return value_const();
}

void CVariable::call_destorier(CAyaVM& vm)
{
	if(destorier.size()){
		ptrdiff_t index = vm.function_exec().GetFunctionIndexFromName(destorier);

		if (index < 0) {
			vm.logger().Error(E_W, 12, destorier);
			//SetError(12);
			return;
		}

		CFunction* it = &vm.function_exec().func[size_t(index)];

		CValue arg(F_TAG_ARRAY, 0/*dmy*/);
		arg.array().emplace_back(this->name);
		arg.array().emplace_back(value_const());
		it->Execute(arg);
	}
}

/*
 * setterを実行し、変数に入れる値をresultに返します
 * setterの中でグローバル変数が増えるとthisが無効になるので、実行後はthisに触れません
 * 呼び出し元が変数を引き直してresultを代入してください
 * 返値　　：  true=setterを実行した false=setterなし
 */
bool CVariable::call_setter(CAyaVM& vm, const CValue& var_before, CValue& result)
{
	if(setter.size()){
		ptrdiff_t index = vm.function_exec().GetFunctionIndexFromName(setter);

		if (index < 0) {
			vm.logger().Error(E_W, 12, setter);
			//SetError(12);
			return false;
		}

		CFunction* it = &vm.function_exec().func[size_t(index)];

		CValue arg(F_TAG_ARRAY, 0/*dmy*/);
		arg.array().emplace_back(this->name);
		arg.array().emplace_back(value_const());
		arg.array().emplace_back(var_before);
		result = it->Execute(arg);
		return true;
	}
	return false;
}
