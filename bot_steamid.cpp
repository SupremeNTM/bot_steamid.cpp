#include <extdll.h>
#include <meta_api.h>
#include <rehlds_api.h>
#include <cstdio>
#include <cstring>
#include <string>

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

std::string ReplaceBotInStatusLine(const char* msg) {
    std::string line(msg);
    if (!line.empty() && line.front() == '#') {
        size_t pos = line.find(" BOT ");
        if (pos != std::string::npos) {
            size_t q1 = line.find('"');
            size_t q2 = (q1 != std::string::npos) ? line.find('"', q1 + 1) : std::string::npos;
            std::string botName = "YaPB";
            if (q1 != std::string::npos && q2 != std::string::npos && q2 > q1 + 1) {
                botName = line.substr(q1 + 1, q2 - q1 - 1);
            }
            unsigned int id = HashBotName(botName.c_str());
            std::string fakeId = " STEAM_0:0:" + std::to_string(id) + " ";
            line.replace(pos, 5, fakeId);
        }
    }
    return line;
}

void Hook_Con_Printf(IRehldsHook_Con_Printf* chain, const char* str) {
    if (str && strstr(str, " BOT ")) {
        std::string modified = ReplaceBotInStatusLine(str);
        chain->callNext(modified.c_str());
        return;
    }
    chain->callNext(str);
}

void Hook_SV_ClientPrintf(IRehldsHook_SV_ClientPrintf* chain, const char* str) {
    if (str && strstr(str, " BOT ")) {
        std::string modified = ReplaceBotInStatusLine(str);
        chain->callNext(modified.c_str());
        return;
    }
    chain->callNext(str);
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
    "1.1",
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

    g_RehldsHookchains->Con_Printf()->registerHook(&Hook_Con_Printf);
    g_RehldsHookchains->SV_ClientPrintf()->registerHook(&Hook_SV_ClientPrintf);
    return TRUE;
}

C_DLLEXPORT int Meta_Detach(PLUG_LOADTIME now, PL_UNLOAD_REASON reason) {
    if (g_RehldsHookchains) {
        g_RehldsHookchains->Con_Printf()->unregisterHook(&Hook_Con_Printf);
        g_RehldsHookchains->SV_ClientPrintf()->unregisterHook(&Hook_SV_ClientPrintf);
    }
    return TRUE;
}
