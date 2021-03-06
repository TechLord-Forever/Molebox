
/*

  Copyright (c) 2008-2009, Alexey Sudachen, alexey@sudachen.name

*/

#include "_common.h"
#include "apif.h"
#include "import.h"
#include "splicer.h"
#include "logger.h"
#include "XorS.h"

#define _iXOr(_,l,i) i, l

static int apif_internal = TlsAlloc();
extern "C" void _DlF_COPY(void* mem);
extern "C" int  _DlF_SIZE();
extern "C" void _DlF_SET_ORIGN(void* mem, void* f);
extern "C" void _DlF_SET_HANDLER(void* mem, void* f);
extern "C" void _DlF_SET_RETN(void* mem, unsigned n);
extern "C" void _DlF_SET_APIFID(void* mem, unsigned id);
extern "C" void _NtDlFhandler();

void xVirtualProtect1(void* p, unsigned l, unsigned long& old)
{
    if ( !VirtualProtect(p,l,PAGE_EXECUTE_READWRITE,&old) )
        VirtualProtect(p,l,PAGE_READWRITE,&old);
}

void xVirtualProtect(void* p, unsigned l)
{
    unsigned long old;
    xVirtualProtect1(p,l,old);
}

void zVirtualProtect(void* p, unsigned l, unsigned long& old)
{
    unsigned long n = old;
    VirtualProtect(p,l,n,&old);
}

HRESULT (__stdcall* _NtQueryInformationProcess)(void*,unsigned long,void*,unsigned long,void*) = 0;

extern "C" void* apif_realcall[APIF_COUNT] = {0};

void HookFunc(int apif_id, char* fn_name, int retn, void* dll, void** hook)
{

    void* orign = GetProcAddressIndirect(dll,fn_name,0);
    if ( !orign ) return;

    void* fx = Splice5(orign);
    void* f = AllocExecutableBlock(_DlF_SIZE());

    if ( !orign || !fx || !f)
        __asm int 3
        ;

    _DlF_COPY(f);
    _DlF_SET_ORIGN(f,fx);
    if ( apif_id == APIF_QUEPROCINFO ) *(void**)&_NtQueryInformationProcess = fx;
    _DlF_SET_HANDLER(f,_NtDlFhandler);
    _DlF_SET_APIFID(f,apif_id);
    _DlF_SET_RETN(f,retn);

    unsigned long old;
    xVirtualProtect1(orign,5,old);
    *(byte_t*)orign = 0xe9;
    *(unsigned*)((byte_t*)orign + 1 ) = ((char*)f - (char*)orign)-5;
    *hook = f;
    zVirtualProtect(orign,5,old);
    apif_realcall[apif_id] = fx;
}

enum APIF_KIND
{
    KIND_NONE,
    KIND_NTDLL,
    KIND_KRN32,
};

struct APIF_F_RECORD
{
    APIF_KIND apif_kind;
    APIF_ID   apif_id;
    int       xor_id;
    int       xor_l;
    int       retn;
    void*     hook;
};

APIF_F_RECORD hooks_list[] =
{
    { KIND_NTDLL, APIF_CREATEFILE,  _iXOr("NtCreateFile",13,262172), 44, 0 },
    { KIND_NTDLL, APIF_OPENFILE,    _iXOr("NtOpenFile",11,1310956), 24, 0 },
    { KIND_NTDLL, APIF_READFILE,    _iXOr("NtReadFile",11,2228478), 36, 0 },
    { KIND_NTDLL, APIF_WRITEFILE,   _iXOr("NtWriteFile",12,3342541), 36, 0 },
    { KIND_NTDLL, APIF_CLOSE,       _iXOr("NtClose",8,4522203), 4, 0 },
    { KIND_NTDLL, APIF_QUEFILINFO,  _iXOr("NtQueryInformationFile",23,5374126), 20, 0 },
    { KIND_NTDLL, APIF_SETFILINFO,  _iXOr("NtSetInformationFile",21,8126644), 20, 0},
    { KIND_NTDLL, APIF_QUEVOLINFO,  _iXOr("NtQueryVolumeInformationFile",29,25362589), 20, 0},
    { KIND_NTDLL, APIF_QUEDIRFILE,  _iXOr("NtQueryDirectoryFile",21,27591035), 44, 0},
    { KIND_NTDLL, APIF_QUESECTINFO, _iXOr("NtQuerySection",15,30146884), 20, 0},
    { KIND_NTDLL, APIF_CREATESECT,  _iXOr("NtCreateSection",16,31261011), 28, 0},
    { KIND_NTDLL, APIF_MAPSECT,     _iXOr("NtMapViewOfSection",19,31457568), 40, 0},
    { KIND_NTDLL, APIF_UNMAPSECT,   _iXOr("NtUnmapViewOfSection",21,17367303), 8, 0},
    { KIND_NTDLL, APIF_QUEFATTR,    _iXOr("NtQueryAttributesFile",22,17891823), 8, 0},
    { KIND_NTDLL, APIF_CREATEHKEY,  _iXOr("NtCreateKey",12,20513271), 28, 0},
    { KIND_NTDLL, APIF_OPENHKEY,    _iXOr("NtOpenKey",10,21692869), 12, 0},
    { KIND_NTDLL, APIF_SETVALHKEY,  _iXOr("NtSetValueKey",14,22610391), 24, 0},
    { KIND_NTDLL, APIF_QUEKEY,      _iXOr("NtQueryKey",11,23921059), 20, 0},
    { KIND_NTDLL, APIF_QUEVALKEY,   _iXOr("NtQueryValueKey",16,25035186), 24, 0},
    { KIND_NTDLL, APIF_ENUKEY,      _iXOr("NtEnumerateKey",15,42205596), 24, 0},
    { KIND_NTDLL, APIF_ENUVALKEY,   _iXOr("NtEnumerateValueKey",20,43450985), 24, 0},
    { KIND_NTDLL, APIF_OPENHKEYEX,  _iXOr("NtOpenKeyEx",12,45089392), 16, 0},
    { KIND_NTDLL, APIF_NOTIFMULTKEY,_iXOr("ZwNotifyChangeMultipleKeys",27,46137920), 48,0},
    { KIND_NTDLL, APIF_NOTIFKEY,    _iXOr("ZwNotifyChangeKey",18,49218081), 40,0},
    { KIND_NTDLL, APIF_QUEFATTRFULL,_iXOr("NtQueryFullAttributesFile",26,49611275), 8, 0},
    { KIND_NTDLL, APIF_DELKEY,      _iXOr("NtDeleteKey",12,34734830), 4, 0},
    { KIND_NTDLL, APIF_DELKEYVAL,   _iXOr("NtDeleteValueKey",17,35848957), 8, 0},
    { KIND_NTDLL, APIF_DUPLICATE,   _iXOr("NtDuplicateObject",18,37159625), 28, 0},
    { KIND_NTDLL, APIF_QUEPROCINFO, _iXOr("NtQueryInformationProcess",26,39518933), 20, 0},
    { KIND_NTDLL, APIF_CREATECTX,   _iXOr("RtlCreateActivationContext",27,41419448), 8, 0},
    { KIND_NTDLL, APIF_QUESEQ,      _iXOr("NtQuerySecurityObject",22,60359319), 20, 0},
    { KIND_NTDLL, APIF_NOTICHNGFL,  _iXOr("NtNotifyChangeDirectoryFile",28,61080444), 36, 0},
    //{ KIND_NTDLL, APIF_ADDREFCTX,   _iXOr("RtlAddRefActivationContext",27,63308634), 4, 0},
    //{ KIND_NTDLL, APIF_RELREFCTX,   _iXOr("RtlReleaseActivationContext",28,65274684), 4, 0},
    //{ KIND_NTDLL, APIF_ACTIVECTX,   _iXOr("RtlActivateActivationContext",29,50463518), 12, 0},
    //{ KIND_NTDLL, APIF_DEACTECTX,   _iXOr("RtlDeactivateActivationContext",31,52691964), 8, 0},
    //{ KIND_NTDLL, APIF_FREETHCTX,   _iXOr("RtlFreeThreadActivationContextStack",36,54985689), 0, 0},
    //{ KIND_NTDLL, APIF_GETACTCTX,   _iXOr("RtlGetActiveActivationContext",30,58524595), 4, 0},
    //{ KIND_NTDLL, APIF_ISACTCTX,    _iXOr("RtlIsActivationContextActive",29,77398931), 4, 0},
    //{ KIND_NTDLL, APIF_QUEACTCTX,   _iXOr("RtlQueryInformationActivationContext",37,78644336), 28, 0},
    //{ KIND_NTDLL, APIF_FINDCTXCSS,  _iXOr("RtlFindActivationContextSectionString",38,82379815), 20, 0},
    { KIND_NTDLL, APIF_CSRCALL,     _iXOr("CsrClientCallServer",20,67306525), 16, 0},
    { KIND_NTDLL, APIF_LOCKFILE,    _iXOr("NtLockFile",11,69928165), 40, 0},
    { KIND_NTDLL, APIF_UNLOCKFILE,  _iXOr("NtUnlockFile",13,71042292), 20, 0},
    { KIND_KRN32, APIF_KRCREATECTX, _iXOr("CreateActCtxW",14,72090820), 4, 0},
    { KIND_KRN32, APIF_GETENVAR,    _iXOr("GetEnvironmentVariableW",24,73204947), 12, 0},
    { KIND_KRN32, APIF_KCRPROCA,    _iXOr("CreateProcessA",15,74974392), 40, 0},
    { KIND_KRN32, APIF_KCRPROCW,    _iXOr("CreateProcessW",15,92996741), 40, 0},
    { KIND_KRN32, APIF_KWINEXEC,    _iXOr("WinExec",8,94241938), 8, 0},
    { KIND_KRN32, APIF_CMDLINEA,    _iXOr("GetCommandLineA",16,94897512), 0, 0},
    { KIND_KRN32, APIF_CMDLINEW,    _iXOr("GetCommandLineW",16,96077174), 0, 0},
    { KIND_NONE,  APIF_NONE },
};

void APIF_::SetupNtKrHooks(void* ntdll,void* krdll)
{
    APIF_F_RECORD* r = hooks_list;
    char b[64];
    for ( ; r->apif_id ; ++r )
    {
        char* S = _UnXOr(r->xor_id,r->xor_l,b);
        if ( r->apif_kind == KIND_NTDLL )
            HookFunc(r->apif_id,S,r->retn,ntdll,&r->hook);
        else if ( r->apif_kind == KIND_KRN32 )
            HookFunc(r->apif_id,S,r->retn,krdll,&r->hook);
    }
}

void APIF_::UnsetHooks()
{
    APIF_F_RECORD* r = hooks_list;
    for ( ; r->apif_id; ++r )
    {
        if ( r->hook )
            _DlF_SET_HANDLER(r->hook,0);
    }
}

void APIF_::Remove(APIfilter* f)
{
}

void APIF_::Push(APIfilter* f)
{
    filters_.Append(f);
}

APIF_STATUS QueryProcInfo(void* _args,unsigned* result)
{
    struct QPI { HANDLE hndl; ULONG ls; ULONG* val; ULONG l; ULONG* rs; };
    QPI* a = (QPI*)_args;
    //XDBG|_S*"%08x %08x" %a->ls %a->hndl;
    if ( a->ls == 0x22 && a->hndl == (HANDLE)-1 )
    {
        *result = _NtQueryInformationProcess(a->hndl,a->ls,a->val,a->l,a->rs);
        *a->val |= 0x30;
        return APIF_RETURN;
    }
    return APIF_ORIGINAL;
}

void* QueryAPI();

APIF_STATUS GetEnVar(void* args, unsigned* result)
{
    struct GEV { LPCWSTR lpName; LPWSTR lpBuffer; DWORD nSize; } *a = (GEV*)args;
    StringA name = a->lpName;
    if ( StrSafeEqualI(+name,_XOr("molebox;version",16,31719844)) )
    {
        *result = _BUILD_NUMBER;
        return APIF_RETURN;
    }
    else if ( StrSafeEqualI(+name,_XOr("molebox;version;txt",20,26608022)) )
    {
        StringW build = _S*L"%04d"%_BUILD_NUMBER;
        *result = build.Length();
        memcpy(a->lpBuffer,+build,build.Length()*2+2);
        return APIF_RETURN;
    }
    else if ( StrSafeEqualI(+name,_XOr("molebox;api",12,25690504)) && a->nSize == sizeof(void*) )
    {
        *(void**)a->lpBuffer = QueryAPI();
        *result = sizeof(void*);
        return APIF_RETURN;
    }
    return APIF_CONTINUE;
}

struct SX { unsigned r[4]; };
extern "C" APIF_STATUS __cdecl APIF_DoCall(SX,int apif_id, void* args, unsigned* result)
{
    if (apif_id == APIF_QUEPROCINFO) return QueryProcInfo(args,result);

    if (apif_id == APIF_CSRCALL)
        for ( int i = 0; i < APIF_::filters_.Count(); ++i )
            if ( APIF_::filters_[i]->DoCall(APIF_CSRCALL,args,result) != APIF_CONTINUE )
                return APIF_ORIGINAL;

    APIF_STATUS apif_s = APIF_ORIGINAL;
    unsigned lastStatus = *_LASTSTATUS();
    unsigned le = GetLastError();

    if ( apif_id != APIF_KRCREATECTX )
    {
        if ( TlsGetValue(apif_internal) )
            return APIF_ORIGINAL;
        TlsSetValue(apif_internal,(void*)1);
    }

    XDBG|_S* _XOr(".apif.Call %d",14,29229566) %apif_id;

    if ( 0 )
        if ( !HeapValidate(GetProcessHeap(),0,0) )
            __asm int 3;

    for ( int i = 0; i < APIF_::filters_.Count(); ++i )
    {
        apif_s = APIF_::filters_[i]->DoCall(apif_id,args,result);
        switch ( apif_s )
        {
            case APIF_RETURN:
            case APIF_ORIGINAL:
                goto e;
            case APIF_CONTINUE:
                break;
        }
    }

    if ( 0 )
        if ( !HeapValidate(GetProcessHeap(),0,0) )
            __asm int 3;

    if ( apif_id == APIF_GETENVAR && apif_s == APIF_CONTINUE )
        apif_s = GetEnVar(args,result);

e:
    if ( apif_id != APIF_KRCREATECTX )
        TlsSetValue(apif_internal,(void*)0);
    SetLastError(le);
    *_LASTSTATUS() = apif_s == APIF_RETURN ? *result : lastStatus;
    return apif_s;
}

BufferT<APIfilter*> APIF_::filters_ = BufferT<APIfilter*>();

void APIF_::AcquireFilterLock()
{
    int i = (int)TlsGetValue(apif_internal);
    TlsSetValue(apif_internal,(void*)(i+1));
}

void APIF_::ReleaseFilterLock()
{
    int i = (int)TlsGetValue(apif_internal);
    if ( i )
        TlsSetValue(apif_internal,(void*)(i-1));
    else
        __asm int 3
        ;
}

long __stdcall APIF_LogState()
{
    for ( int i = 0; i < APIF_::filters_.Count(); ++i )
        APIF_::filters_[i]->LogState();
    return 0;
}

extern "C" void Regster_API_Filter(API_FILTER* flt)
{
    APIF->Push(new APIfilter_C(flt));
}

extern "C" void Regster_Clear()
{
    APIF_::filters_.Clear();
}



