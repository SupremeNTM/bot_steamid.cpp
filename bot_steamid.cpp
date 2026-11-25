#include <extdll.h>
#include <meta_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned int HashBotName(const char* name) {
    unsigned int hash = 5381;
    while (*name) {
        hash = ((hash << 5) + hash) + (unsigned char)(*name++);
    }
    return (hash % 899999999) + 100000000;
}

void Hook_ClientPrintf(edict_t* pEdict, PRINT_TYPE ptype, const char* szMsg) {
    if (szMsg && *szMsg == '#') {
        const char* botPos = strstr(szMsg, " BOT ");
        if (botPos) {
            static char* outBuf = nullptr;
            static char* botName = nullptr;
            static char* midBuf = nullptr;
            if (!outBuf) outBuf = (char*)malloc(512);
            if (!botName) botName = (char*)malloc(64);
            if (!midBuf) midBuf = (char*)malloc(64);

            strcpy(botName, "YaPB");
            const char* q1 = strchr(szMsg, '"');
            if (q1) {
                const char* q2 = strchr(q1 + 1, '"');
                if (q2 && q2 > q1 + 1) {
                    size_t len = (size_t)(q2 - (q1 + 1));
                    if (len > 63) len = 63;
                    memcpy(botName, q1 + 1, len);
                    *(botName + len) = '\0';
                }
            }

            unsigned int id = HashBotName(botName);
            size_t prefixLen = (size_t)(botPos - szMsg);
            if (prefixLen > 200) prefixLen = 200;

            memcpy(outBuf, szMsg, prefixLen);
            *(outBuf + prefixLen) = '\0';

            snprintf(midBuf, 64, " STEAM_0:0:%u ", id);
            strcat(outBuf, midBuf);
            strncat(outBuf, botPos + 5, 511 - strlen(outBuf));

            (*g_engfuncs.pfnClientPrintf)(pEdict, ptype, outBuf);
            RETURN_META(MRES_SUPERCEDE);
        }
    }
    RETURN_META(MRES_IGNORED);
}

plugin_info_t Plugin_info = {
    META_INTERFACE_VERSION,
    "FakeBot Status",
    "1.3",
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
