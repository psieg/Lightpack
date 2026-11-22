#ifndef PROXYFUNCJMP_H
#define PROXYFUNCJMP_H

#include"ProxyFunc.hpp"

class ProxyFuncJmp : public ProxyFunc
{
public:
	ProxyFuncJmp(void * pTarget, void * pSubstFunc, Logger *logger) :
		ProxyFunc(pTarget, pSubstFunc, logger),
		m_pTrampoline(nullptr) {}
	virtual ~ProxyFuncJmp();
	virtual bool init();
	virtual bool isHookInstalled();
	virtual bool installHook();
	virtual bool removeHook();

private:
	static const int kJmpInstructionSize = 5;
	unsigned char m_pOriginalCode[kJmpInstructionSize];
	unsigned char m_pJmpCode[kJmpInstructionSize];
	void *m_pTrampoline;
};

#endif // PROXYFUNCJMP_H
