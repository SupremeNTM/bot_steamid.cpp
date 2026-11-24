#include <extdll.h>
#include <meta_api.h>
#include <rehlds_api.h>
#include <cstdio>
#include <cstring>

IRehldsApi* g_RehldsApi = nullptr;
const RehldsFuncs_t* g_RehldsFuncs = nullptr;
IRehldsHookchains* g_RehldsHookchains = nullptr;
IRehldsServerStatic* g_RehldsSvs = nullptr;

unsigned int HashBotName(const char* name) {
    unsigned int hash = 5381;
    while (*name) {
        hash = ((hash << 5) + hash) + (unsigned char)(*name++);
    }
    return (hash % 899999999) + 100000000;
}

char* Hook_SV_GetIDString(IRehldsHook_SV_GetIDString* chain, USERID_t* id_p) {
    if (id_p && (int)id_p->idtype == 4) {
        static char fakeSteamId;
        const char* botName = "YaPB";
        if (g_RehldsSvs) {
            int maxClients = g_RehldsSvs->GetMaxClients();
            for (int i = 0; i < maxClients; i++) {
                IGameClient* cl = g_RehldsSvs->GetClient(i);
                if (cl && cl->IsConnected() && cl->GetNetworkUserID() == id_p) {
                    botName = cl->GetName();
                    break;
                }
            }
        }
        unsigned int id = HashBotName(botName);
        snprintf(fakeSteamId, 32, "STEAM_0:0:%u", id);
        return fakeSteamId;
    }
    return chain->callNext(id_p);
}

bool InitRehldsApi() {
    CSysModule* engineModule = Sys_LoadModule("engine_i486.so");
    if (!engineModule) return false;

    CreateInterfaceFn ifaceFactory = Sys_GetFactory(engineModule);
    if (!ifaceFactory) return false;

    int retCode = 0;
    g_RehldsApi = (IRehldsApi*)ifaceFactory(VREHLDS_HLDS_API_VERSION, &retCode);
    if (!g_RehldsApi) return false;

    if (g_RehldsApi->GetMajorVersion() != REHLDS_API_VERSION_MAJOR) return false;
    if (g_RehldsApi->GetMinorVersion() < REHLDS_API_VERSION_MINOR) return false;

    g_RehldsFuncs = g_RehldsApi->GetFuncs();
    g_RehldsHookchains = g_RehldsApi->GetHookchains();
    g_RehldsSvs = g_RehldsApi->GetServerStatic();
    return true;
}

plugin_info_t Plugin_info = {
    META_INTERFACE_VERSION,
    "FakeBot Status",
    "1.0",
    "2026",
    "Custom",
    "",
    "FBSTATUS",
    PT_ANYTIME,
    PT_ANYTIME,
};

meta_globals_t* gpMetaGlobals;
gamedll_funcs_t* gpGamedllFuncs;
mutil_funcs_t* gpMetaUtilFuncs;
enginefuncs_t g_engfuncs;
globalvars_t* gpGlobals;

C_DLLEXPORT void WINAPI GiveFnptrsToDll(enginefuncs_t* pengfuncsFromEngine, globalvars_t* pGlobals) {
    memcpy(&g_engfuncs, pengfuncsFromEngine, sizeof(enginefuncs_t));
    gpGlobals = pGlobals;
}

C_DLLEXPORT int Meta_Query(char* ifvers, plugin_info_t** pPlugInfo, mutil_funcs_t* pMetaUtilFuncs) {
    *pPlugInfo = &Plugin_info;
    gpMetaUtilFuncs = pMetaUtilFuncs;
    return TRUE;
}

C_DLLEXPORT int Meta_Attach(PLUG_LOADTIME now, META_FUNCTIONS* pFunctionTable, meta_globals_t* pMGlobals, gamedll_funcs_t* pGamedllFuncs) {
    gpMetaGlobals = pMGlobals;
    gpGamedllFuncs = pGamedllFuncs;
    memset(pFunctionTable, 0, sizeof(META_FUNCTIONS));

    if (!InitRehldsApi()) {
        LOG_CONSOLE(PLID, "[FBSTATUS] Eroare: Nu s-a putut incarca ReHLDS API!");
        return FALSE;
    }

    g_RehldsHookchains->SV_GetIDString()->registerHook(&Hook_SV_GetIDString);
    return TRUE;
}

C_DLLEXPORT int Meta_Detach(PLUG_LOADTIME now, PL_UNLOAD_REASON reason) {
    if (g_RehldsHookchains) {
        g_RehldsHookchains->SV_GetIDString()->unregisterHook(&Hook_SV_GetIDString);
    }
    return TRUE;
}
