/*
 * File author: sfwidde (Kelvin)
 * Created: 2025-01-28
 */

#pragma once

#include <windows.h>
#include <string.h>

// Add padding to struct
#define PAD(x, n) \
	private: \
		const BYTE pad ## x[n]; \
	public:
#define NAKED __declspec(naked) void // Without prologue or epilogue
#define SAFE_CALL __stdcall // The callee itself cleans the stack

// -----------------------------------------------------------------------------

template<typename T>
class Patch
{
public:
	T& m_data;
private:
	DWORD m_oldProt;

public:
	Patch(DWORD address);
	~Patch();
};

template<typename T>
Patch<T>::Patch(DWORD address) : m_data(*(T*)address)
{
	VirtualProtect(&m_data, sizeof(T), PAGE_EXECUTE_READWRITE, &m_oldProt);
}

template<typename T>
Patch<T>::~Patch()
{
	DWORD myProt;
	VirtualProtect(&m_data, sizeof(T), m_oldProt, &myProt);
}

// -----------------------------------------------------------------------------

template<SIZE_T N>
void InstallHook(DWORD address, LPCVOID functionHook)
{
	// jmp (1) + address (4) = 5 [+ remaining bytes]
	static_assert(N >= 5);

	Patch<BYTE[N]> theHook(address);
	*theHook.m_data = 0xE9; // jmp
	*(DWORD*)(theHook.m_data + 1) = ((DWORD)functionHook - (address + 5)); // Redirect address
	memset(theHook.m_data + 5, 0x90, N - 5); // nop remaining bytes
}

// -----------------------------------------------------------------------------
