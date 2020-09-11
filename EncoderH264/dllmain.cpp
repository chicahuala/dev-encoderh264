// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

#include <atlbase.h>
#include <atlcom.h>
#include <atlctl.h>

#include <oleauto.h>

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        EXTERN_C __declspec(selectany) const type name = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}

MIDL_DEFINE_GUID(IID, LIBID_EncoderH264ATLLib, 0x489ae2e2, 0xaae2, 0x4ab9, 0xa2, 0xa2, 0xc1, 0xd9, 0x97, 0x1b, 0x05, 0x19);

#define IDR_ENCODERH264ATL      101

class CEncoderH264ATLModule : public ATL::CAtlDllModuleT< CEncoderH264ATLModule >
    {
    public:
        DECLARE_LIBID(LIBID_EncoderH264ATLLib)
        DECLARE_REGISTRY_APPID_RESOURCEID(IDR_ENCODERH264ATL, "{489ae2e2-aae2-4ab9-a2a2-c1d9971b0519}")
    };

extern class CEncoderH264ATLModule _AtlModule;

CEncoderH264ATLModule _AtlModule;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    OLECHAR och;
    BSTR bs = SysAllocString(&och);

    // hInstance;
    return _AtlModule.DllMain(ul_reason_for_call, lpReserved);

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

