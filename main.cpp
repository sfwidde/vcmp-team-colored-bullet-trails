/*
 * Per-Team Colored Bullet Trails for Vice City: Multiplayer (VC:MP) 0.4
 * Author: sfwidde (Kelvin)
 * 2026-08-23
 */

#include "patch.hpp"
#include <windows.h>

#define MAX_PLAYERS 100
#define MAX_BULLET_TRACES 16

#pragma pack(push, 1)
class Player
{
public:
	PAD(0, 8); // 0-8
	char m_name[24]; // 8-32
	PAD(1, 8); // 32-40
	int m_id; // 40-44
	PAD(2, 8); // 44-52
	DWORD m_color; // 52-56
	PAD(3, 12); // 56-68
	void** m_ped; // 68-72
	// ...
};
#pragma pack(pop)

static DWORD vcmpBaseAddress;
static const void* shootingEntity; // CEntity*
static const Player* shootingPlayer;
static DWORD bulletTraceColors[MAX_BULLET_TRACES];
static int bulletTraceId;

static void ClearBulletTraceColors()
{
	for (int i = 0; i < MAX_BULLET_TRACES; ++i)
	{
		bulletTraceColors[i] = 0xFFFFFF; // @0x573112
	}
}

static Player* /*SAFE_CALL*/ FindPlayerFromPed(const void* ped)
{
	DWORD x = *(DWORD*)(vcmpBaseAddress + 0x42BDB0); // An object
	// Can be NULL, e.g.: when /disconnect'ed
	if (!x)
	{
		ClearBulletTraceColors();
		return nullptr;
	}

	// Hopefully is valid at all times else we're fucked -
	// could have been solved with a NULL check, dickhead
	x = *(DWORD*)(x + 4); // +0x4 = players array

	Player* player;
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		player = (Player*)(*(DWORD*)(x + i * 4 + 4)); // See 0x138315
		if (player && player->m_ped && *player->m_ped == ped)
		{
			return player;
		}
	}
	return nullptr;
}

// -----------------------------------------------------------------------------

// 0x573D40 was the right one for this task but VC:MP forced me
// to hook this one instead because it wasn't being called for shotguns :<
static NAKED CWeapon_FireHook()
{
	__asm
	{
		// Grab the shooting entity
		mov eax, [esp+4]
		mov shootingEntity, eax

		push ebx
		mov ebx, esp
		sub esp, 8

		mov eax, vcmpBaseAddress
		add eax, 0xE5086
		jmp eax
	}
}

static NAKED CBulletTraces_AddTraceHook()
{
	__asm mov [ebp+0x8118F0], ebx

	__asm mov bulletTraceId, ebp
	__asm pushad
	shootingPlayer = FindPlayerFromPed(shootingEntity);
	if (shootingPlayer)
	{
		bulletTraceColors[bulletTraceId / 0x2C] = shootingPlayer->m_color;
	}
	__asm popad

	__asm mov ecx, 0x573A2B
	__asm jmp ecx
}

static NAKED CBulletTraces_RenderHook()
{
	__asm
	{
		// Get these ready for idiv
		push ebx
		push eax
		push edx

		// Get the actual bullet ID (eax)
		mov eax, ebp
		mov ebx, 0x2C
		cdq
		idiv ebx
		pop edx

		mov ebx, eax // ebx = eax because we need eax restored
		pop eax
		or eax, [bulletTraceColors+ebx*4] // This is our magic
		pop ebx

		mov esi, 0x573117
		jmp esi
	}
}

// -----------------------------------------------------------------------------

static DWORD GetVCMPBaseAddress()
{
	HMODULE x = GetModuleHandleW(L"vcmp-game.dll");
	if (!x) { x = GetModuleHandleW(L"vcmp-steam.dll"); }
	return (DWORD)x;
}

static BOOL Init()
{
	if (!(vcmpBaseAddress = GetVCMPBaseAddress())) { return FALSE; }

	ClearBulletTraceColors();
	InstallHook<6>(vcmpBaseAddress + 0xE5080, CWeapon_FireHook);
	InstallHook<6>(0x573A25, CBulletTraces_AddTraceHook);
	InstallHook<5>(0x573112, CBulletTraces_RenderHook);
	return TRUE;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hinstDLL);
		return Init();
	default:
		return TRUE;
	}
}
