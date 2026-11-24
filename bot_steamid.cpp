#include <extdll.h>
#include <meta_api.h>
#include <cstdio>
#include <cstring>
#include <string>

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

void Hook_ClientPrintf(edict_t* pEdict, PRINT_TYPE ptype, const char* szMsg) {
    if (szMsg && strstr(szMsg, " BOT ")) {
        std::string modified = ReplaceBotInStatusLine(szMsg);
        (*g_engfuncs.pfnClientPrintf)(pEdict, ptype, modified.c_str());
        RETURN_META(MRES_SUPERCEDE);
    }
    RETURN_META(MRES_IGNORED);
}

plugin_info_t Plugin_info = {
    META_INTERFACE_VERSION,
    "FakeBot Status",
    "1.2",
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

enginefuncs_t g_HookedEngineFuncs;

C_DLLEXPORT int GetEngineFunctions(enginefuncs_t* pengfuncsFromEngine, int* interfaceVersion) {
    if (!pengfuncsFromEngine) return FALSE;
    if (*interfaceVersion != ENGINE_INTERFACE_VERSION) {
        *interfaceVersion = ENGINE_INTERFACE_VERSION;
        return FALSE;
    }
    memset(&g_HookedEngineFuncs, 0, sizeof(enginefuncs_t));
    g_HookedEngineFuncs.pfnClientPrintf = Hook_ClientPrintf;
    memcpy(pengfuncsFromEngine, &g_HookedEngineFuncs, sizeof(enginefuncs_t));
    return TRUE;
}

C_DLLEXPORT int Meta_Attach(PLUG_LOADTIME now, META_FUNCTIONS* pFunctionTable, meta_globals_t* pMGlobals, gamedll_funcs_t* pGamedllFuncs) {
    gpMetaGlobals = pMGlobals;
    gpGamedllFuncs = pGamedllFuncs;
    memset(pFunctionTable, 0, sizeof(META_FUNCTIONS));
    pFunctionTable->pfnGetEngineFunctions = GetEngineFunctions;
    return TRUE;
}

C_DLLEXPORT int Meta_Detach(PLUG_LOADTIME now, PL_UNLOAD_REASON reason) {
    return TRUE;
}
