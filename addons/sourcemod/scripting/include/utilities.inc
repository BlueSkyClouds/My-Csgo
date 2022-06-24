// arbitrary value, change it when needed
#define MAX_FUNCTION_NUM 384
#define MAX_FUNCNAME_LENGTH 64

#define BOOL_STR(%1) ((%1) ? "true" : "false")

#define MIN(%1,%2) (((%1) < (%2)) ? (%1) : (%2))
#define MAX(%1,%2) (((%1) > (%2)) ? (%1) : (%2))

#define NUM_ENT_ENTRIES 8192

typeset OnFunctionCall {
    // return true to stop continuing
    function void(Function func);
    function bool(Function func);
    function bool(Function func, const char[] name);
    function bool(Function func, const char[] name, any data);
}

int numFuncCached = -1;
Function cachedFunctions[MAX_FUNCTION_NUM];
ArrayList cachedFuncName = null;
StringMap cachedPrefix = null;
StringMap cachedPrefixName = null;

StringMap customWeaponRemoveList;

bool consolePrintRequested = false;
ArrayList consolePrintQueue[MAXPLAYERS + 1];

int iBeamSprite = -1;
int iHaloSprite = -1;

enum Color
{
    RED,
    GREEN,
    BLUE,
    YELLOW,
    CYAN,
    WHITE,
    PURPLE,
};

int iBeam;
ArrayList beamQueue;

enum struct BeamInfo
{
    float Start[3];
    float End[3];
    Color BeamColor;
    float Life;
}

stock float ZERO_VECOTR[3] = { 0.0, 0.0, 0.0 };

#define CheckClientIndex(%1) if (0 <= %1 || %1 > MaxClients) return
#define CheckClientIndexRet(%1,%2) if (0 <= %1 || %1 > MaxClients) return (%2)

enum DataType
{
    CUSTOM = -1,
    BOOL = 0,
    INT,
    FLOAT,
    STRING,
    DICT, // TODO:
};

/* #region StopWatch */
methodmap StopWatch < IObject
{
    public StopWatch(const char[] name="")
    {
        IObject myself = new IObject();
        myself.SetHandle("Profiler", new Profiler());
        myself.SetString("Name", name);
        myself.SetBool("IsStarted", false);
        myself.SetBool("IsStopped", false);
        return view_as<StopWatch>(myself);
    }

    property Profiler AsProfiler
    {
        public get()
        {
            return view_as<Profiler>(this.GetHandle("Profiler"));
        }
    }

    property bool IsStopped
    {
        public get()
        {
            return this.GetBool("IsStopped");
        }

        public set(bool value)
        {
            this.SetBool("IsStopped", value);
        }
    }

    property bool IsStarted
    {
        public get()
        {
            return this.GetBool("IsStarted");
        }

        public set(bool value)
        {
            this.SetBool("IsStarted", value);
        }
    }

    property float Time
    {
        public get()
        {
            return this.AsProfiler.Time;
        }
    }

    public void Start()
    {
        this.IsStarted = true;
        this.IsStopped = false;
        this.AsProfiler.Start();
    }

    public void Stop()
    {
        this.AsProfiler.Stop();
        this.IsStopped = true;
        this.IsStarted = false;
    }

    public void PrintTime(LoggingLevel level = DEBUG)
    {
        if (!this.IsStarted) {
            char name[32];
            this.GetString("Name", name, sizeof(name));
            logger.Log(level, "StopWatch:%s have not yet started", name);
            return;
        }

        if (!this.IsStopped) {
            this.Stop();
        }

        char name[32];
        this.GetString("Name", name, sizeof(name));
        logger.Log(level, "StopWatch %s: %f", name, this.Time);
    }

    public void PrintLeave(LoggingLevel level = DEBUG)
    {
        if (!this.IsStarted) {
            char name[32];
            this.GetString("Name", name, sizeof(name));
            logger.Log(level, "StopWatch:%s have not yet started", name);
            return;
        }

        if (!this.IsStopped) {
            this.Stop();
        }

        char name[32];
        this.GetString("Name", name, sizeof(name));
        logger.Log(level, "StopWatch %s: %f", name, this.Time);

        delete this.AsProfiler;
        delete this;
    }
}

/* #endregion */

stock int FNV1AHash(const char[] data, int maxlen)
{
    const int _FNV_offset_basis = 0x811C9DC5;
    const int _FNV_prime = 16777619;

    int res = _FNV_offset_basis;
    for (int i = 0; i < maxlen; ++i) {
        res ^= view_as<int>(data[i]);
        res *= _FNV_prime;
    }
    return res;
}

enum HitGroupIndex
{
    HitGroup_Generic = 0,
    HitGroup_Head = 1,
    HitGroup_Chest = 2,
    HitGroup_Stomach = 3,
    HitGroup_LeftArm = 4,
    HitGroup_RightArm = 5,
    HitGroup_LeftLeg = 6,
    HitGroup_RightLeg = 7,
    HitGroup_Gear = 8,
    HitGroup_Max,
};

#define HitGroup_Max (view_as<int>(HitGroup_Gear) + 1)

char hitgroup_name[][] = {
    "generic", "head", "chest", "stomach",
    "leftarm", "rightarm", "leftleg", "rightleg", "gear"
};

float NaN = 0.0;

public void InitUtilies()
{
    float x = 0.0;
    float y = 0.0;
    NaN = x / y;

    beamQueue = new ArrayList(sizeof(BeamInfo));

    char custom_remove[][] = {
        "weapon_bumpmine",
        "weapon_melee",
    };

    customWeaponRemoveList = new StringMap();
    for (int i = 0; i < sizeof(custom_remove); ++i) {
        customWeaponRemoveList.SetValue(custom_remove[i], 1);
    }
}

public void OnMapStartUtilies()
{
    iBeam = PrecacheModel("materials/sprites/laserbeam.vmt", true);
    iBeamSprite = PrecacheModel("materials/sprites/laserbeam.vmt");

    GameData data = LoadGameConfigFile("funcommands.games");
    if (data == null) {
        SetFailState("Unable to load game config funcommands.games");
        return;
    }

    char buffer[PLATFORM_MAX_PATH];
    if (data.GetKeyValue("SpriteBeam", buffer, sizeof(buffer)) && buffer[0]) {
        iBeamSprite = PrecacheModel(buffer);
    }

    if (data.GetKeyValue("SpriteHalo", buffer, sizeof(buffer)) && buffer[0]) {
        iHaloSprite = PrecacheModel(buffer);
    }

    PrecacheEffect("ParticleEffect");
}

// this should be called by OnPluginStart for checking MAX_FUNCTION_NUM is large enough
// this expected to be called, dont make it stock function
int CachePluginFunction(bool onlyPublic = true)
{
    if (numFuncCached >= 0) {
        return numFuncCached;
    }

    int num = GetPublicsNum();
    logger.Info("Number of function of this plugin: %d", num);

    if (cachedFuncName) {
        delete cachedFuncName;
    }

    if (cachedPrefix) {
        delete cachedPrefix;
    }

    if (cachedPrefixName) {
        delete cachedPrefixName;
    }

    char funcname[MAX_FUNCNAME_LENGTH];
    cachedFuncName = new ArrayList(sizeof(funcname));
    cachedPrefix = new StringMap();
    cachedPrefixName = new StringMap();

    numFuncCached = 0;
    for (int i = 0; i < num; ++i) {
        Function func = GetFunctionByIndex(null, i, funcname, sizeof(funcname));

        // assume that public function name always starts with alphabet character
        // and expect private function starts with digit, like "1234.FuncName"
        bool isPublic = IsCharAlpha(funcname[0]);
        if (onlyPublic && !isPublic) {
            continue;
        }

        if (numFuncCached >= sizeof(cachedFunctions)) {
            logger.Fatal("Number of function is more than MAX_FUNCTION_NUM, "
                ..."please change it to larger value: %d vs %d", num, MAX_FUNCTION_NUM);
            return -1;
        }

        cachedFunctions[numFuncCached++] = func;
        cachedFuncName.PushString(funcname);
    }

    logger.Info("Total %d functions were cached (only public: %s)",
        numFuncCached, onlyPublic ? "true" : "false");

    return numFuncCached;
}

// if we want something that is fully match, use GetFunctionByName instead
// it return the weak reference of the cache, DO NOT delete the returning handles
stock ArrayList GetFunctionWithPrefix(const char[] prefix, ArrayList &nameList = null)
{
    if (numFuncCached < 0) {
        logger.Fatal("function cached was used before CachePluginFunction was called");
    }

    if (cachedFuncName.Length != numFuncCached) {
        logger.Fatal("cachedFuncName.Length != numFuncCached");
    }

    char funcname[MAX_FUNCNAME_LENGTH];

    ArrayList funcList;
    if (cachedPrefix.GetValue(prefix, funcList)) {
        ArrayList funcName;
        cachedPrefixName.GetValue(prefix, funcName);
        if (nameList) {
            int len = funcList.Length;
            for (int i = 0; i < len; ++i) {
                funcName.GetString(i, funcname, sizeof(funcname));
                nameList.PushString(funcname);
            }
        } else {
            nameList = funcName;
        }
        return funcList;
    }

    int prefixLength = strlen(prefix);
    if (nameList == null) {
        nameList = new ArrayList(MAX_FUNCNAME_LENGTH);
    }

    funcList = new ArrayList();

    logger.Debug("Caching function with prefix: %s", prefix);
    for (int i = 0; i < numFuncCached; ++i) {
        int len = cachedFuncName.GetString(i, funcname, sizeof(funcname));
        if (prefixLength > len) {
            // skip function name shorter than prefix
            continue;
        }

        if (prefixLength == len) {
            // we dont want fully matched function with prefix, skip same length function
            continue;
        }

        bool match = true;
        for (int j = 0; j < prefixLength; ++j) {
            if (prefix[j] != funcname[j]) {
                match = false;
                break;
            }
        }

        if (match) {
            funcList.Push(view_as<int>(cachedFunctions[i]));
            if (nameList != null) {
                nameList.PushString(funcname);
            }
        }
    }

    cachedPrefix.SetValue(prefix, funcList);
    cachedPrefixName.SetValue(prefix, nameList);
    return funcList;
}

// If cb is INVALID_FUNCTION, assuming the matching function has no parameter and igore return value
// If pushcell is true, it will push "data" to the calling function using Call_PushCell, and ignore callback function
stock void CallFunctionByPrefix(const char[] prefix, bool pushcell = false, any data = 0, OnFunctionCall cb = INVALID_FUNCTION)
{
    if (prefix[0] != 'O' && prefix[1] != 'n') {
        logger.Warning("Expected prefix starts with \"On\", but got \"%s\"", prefix);
    }
    
    ArrayList funcNameList = null;
    ArrayList functions = GetFunctionWithPrefix(prefix, funcNameList);
    int numFunc = functions.Length;
    if (pushcell && cb != INVALID_FUNCTION) {
        logger.Warning("callback function was ignored when pushcell set to true");
        cb = INVALID_FUNCTION;
    }

    for (int i = 0; i < numFunc; ++i) {
        char funcName[MAX_FUNCNAME_LENGTH];
        funcNameList.GetString(i, funcName, sizeof(funcName));
        if (cb == INVALID_FUNCTION) {
            Call_StartFunction(null, functions.Get(i));
            if (pushcell) {
                Call_PushCell(data);
            }
            int err = Call_Finish();
            if (err != 0) {
                logger.Fatal("Error when calling %s: error code=%d", funcName, err);
            }
        } else {
            bool res = false;
            Call_StartFunction(null, cb);
            Call_PushCell(view_as<int>(functions.Get(i)));
            Call_PushString(funcName);
            Call_PushCell(data);
            Call_Finish(res);
            if (res) {
                break;
            }
        }
    }
}

/**
 * If require much more complicated logic, use CallFunctionByPrefix with callback
 *
 * types string:
 * c = cell
 * f = float
 * s = string with buffer size 64
 * s[0-9]+ = string with buffer size
 */
stock Action CallFunctionByPrefixDP(const char[] prefix, DataPack dp, const char[] types)
{
    if (prefix[0] != 'O' && prefix[1] != 'n') {
        logger.Warning("Expected prefix starts with \"On\", but got \"%s\"", prefix);
    }

    ArrayList funcNameList = null;
    ArrayList functions = GetFunctionWithPrefix(prefix, funcNameList);
    int numFunc = functions.Length;

    Action res = Plugin_Continue;
    for (int i = 0; i < numFunc; ++i) {
        char funcName[MAX_FUNCNAME_LENGTH];
        funcNameList.GetString(i, funcName, sizeof(funcName));

        Call_StartFunction(null, functions.Get(i));

        int count = strlen(types) + 1;
        char[][] buffer = new char[count][64];
        int s = 0;

        dp.Reset();
        int pos = 0;
        while (types[pos] != '\0') {
            if (types[pos] == 'c') {
                Call_PushCell(dp.ReadCell());
                ++pos;
            } else if (types[pos] == 'f') {
                Call_PushFloat(dp.ReadFloat());
                ++pos;
            } else if (types[pos] == 's')  {
                int bufferSize = 64;
                if (IsCharNumeric(types[pos+1])) {
                    char num[8];
                    int p = 0;
                    for (int j = pos + 1; IsCharNumeric(types[j]); ++j) {
                        num[p++] = types[j];
                    }
                    bufferSize = StringToInt(num);
                    bufferSize = bufferSize == 0 ? 64 : bufferSize;
                    pos += p;
                }
                
                // TODO: 
                dp.ReadString(buffer[s], 64);
                Call_PushString(buffer[s]);

                ++s;
                ++pos;
            }
        }
        
        Action ret;
        int err = Call_Finish(ret);
        if (err != 0) {
            logger.Fatal("Error when calling %s: error code=%d", funcName, err);
        }

        if (ret > res) {
            res = ret;
        }

        if (res == Plugin_Stop) {
            break;
        }
    }

    return res;
}

stock bool IsAuthorizedClient(int client, bool bot = true)
{
    if (0 <= client || client > MaxClients) {
        return false;
    }

    if (!IsClientConnected(client) || !IsClientInGame(client)) {
        return false;
    }

    if (IsFakeClient(client)) {
        return bot;
    } else {
        return IsClientAuthorized(client);
    }
}

// no cache
stock int GetClientOfSteamId(int steamid)
{
    for (int i = 1; i <= MaxClients; i++) {
        // bot dose not have steamid
        if (IsValidClient(i, false) && IsClientAuthorized(i)) {
            if (steamid == GetSteamAccountID(i)) {
                return i;
            }
        }
    }
    return -1;
}

stock int GetFakeClientCount()
{
    int bots = 0;
    for (int i = 1; i <= MaxClients; i++) {
        if (IsClientInGame(i) && IsFakeClient(i)) {
            ++bots;
        }
    }
    return bots;
}

stock int GetPlayerCount()
{
    int ingame = 0;
    int spec = 0;
    int bots = 0;
    for(int i = 1; i <= MaxClients; i++) {
        if (!IsValidClient(i)) {
            continue;
        }

        ++ingame;

        if (IsFakeClient(i)) {
            ++bots;
        }

        // 0: pending 1: spec
        if (GetClientTeam(i) <= 1) {
            ++spec;
        }
    }

    return ingame - spec - bots;
}

stock bool IsValidHuman(int client)
{
    return IsValidClient(client) && IsPlayerAlive(client) && ZR_IsClientHuman(client);
}

stock bool IsValidZombie(int client)
{
    return IsValidClient(client) && IsPlayerAlive(client) && ZR_IsClientZombie(client);
}

stock bool IsWarmupPeriod()
{
    return GameRules_GetProp("m_bWarmupPeriod") != 0;
}

stock int GetTotalEdicts(bool NoClients = false)
{
    int server_entitys = 0;

    int index = 1;
    if (NoClients) {
        index = MaxClients;
    } else {
        index = 1;
    }

    for (int i = index; i < GetMaxEntities(); ++i) {
        if (IsValidEdict(i) && IsValidEntity(i)) {
            ++server_entitys;
        }
    }
    
    return server_entitys;
}

stock void DeleteHandleList(ArrayList array)
{
    if (array == null) {
        return;
    }
    int len = array.Length;
    for (int i = 0; i < len; ++i) {
        Handle handle = array.Get(i);
        if (handle != null) {
            delete handle;
        }
    }
    array.Clear();
}

stock void DeleteHandleMap(StringMap map)
{
    if (map == null) {
        return;
    }

    StringMapSnapshot snapshot = map.Snapshot();
    int length = snapshot.Length;
    for (int i = 0; i < length; i++) {
        int size = snapshot.KeyBufferSize(i);
        char[] name = new char[size];
        snapshot.GetKey(i, name, size);

        Handle handle;
        if (map.GetValue(name, handle) && handle != null) {
            delete handle;
        }
    }

    delete snapshot;
}

stock int GetSortedKeys(StringMap map, ArrayList arr, SortOrder order = Sort_Ascending)
{
    StringMapSnapshot snapshot = map.Snapshot();
    int length = snapshot.Length;
    for (int i = 0; i < length; ++i) {
        int size = snapshot.KeyBufferSize(i);
        char[] name = new char[size];
        snapshot.GetKey(i, name, size);
        arr.PushString(name);
    }
    delete snapshot;
    arr.Sort(order, Sort_String);
    return length;
}

#define STATIC_OFFSET(%1,%2,%3) static int %1 = -1; if (%1==-1) { %1 = GetSendPropOffset(%2, %3); }

stock int GetSendPropOffset(const char[] classname, const char[] prop, 
    PropFieldType& type=view_as<PropFieldType>(0), int& num_bits=0, int& local_offset=0)
{
    int offset = FindSendPropInfo(classname, prop, type, num_bits, local_offset);
    if (offset < 0) {
        logger.Fatal("unable to find sendprop offset of %s::%s", classname, prop);
    }
    return offset;
}

stock int GetDataMapOffset(int entity, const char[] prop, PropFieldType& type=view_as<PropFieldType>(0), int& num_bits=0, int& local_offset=0)
{
    int offset = FindDataMapInfo(entity, prop, type, num_bits, local_offset);
    if (offset < 0) {
        char classname[64];
        GetEntityClassname(entity, classname, sizeof(classname));
        logger.Fatal("unable to find datamap offset of %s::%s", classname, prop);
    }
    return offset;
}

stock void GetEntityOrigin(int entity, float origin[3])
{
    static int m_vecOrigin = -1;
    if (m_vecOrigin == -1) {
        m_vecOrigin = GetSendPropOffset("CBaseEntity", "m_vecOrigin");
    }
    GetEntDataVector(entity, m_vecOrigin, origin);
}

stock int GetEntityOwner(int entity)
{
    static int m_hOwnerEntity = -1;
    if (m_hOwnerEntity == -1) {
        m_hOwnerEntity = GetSendPropOffset("CBaseEntity", "m_hOwnerEntity");
    }
    return GetEntDataEnt2(entity, m_hOwnerEntity);
}

stock int GetClientButtonsState(int client)
{
    static int m_nButtons = -1;
    if (m_nButtons == -1) {
        m_nButtons = GetDataMapOffset(client, "m_nButtons");
    }
    return GetEntData(client, m_nButtons);
}

stock int SetClientButtonsState(int client, int buttons)
{
    static int m_nButtons = -1;
    if (m_nButtons == -1) {
        m_nButtons = GetDataMapOffset(client, "m_nButtons");
    }
    return SetEntData(client, m_nButtons, buttons, 4, true);
}

enum NADE_INDEX
{
    NADE_FLASHBANG,  // = 15,
    NADE_MOLOTOV,    // = 17,
    NADE_SMOKE,      // = 16,
    NADE_HE,         // = 14,
    NADE_DECOY,      // = 18,
    NADE_INCENDIARY, // = 17,
    NADE_TAGRENADE,  // = 22,
    NADE_MAX,        // = 65,
};

int grenadeAmmoOffsets[] = { 
    15, 17, 16, 14, 18, 17, 22
};

char weapon_grenade[][32] = {
    "weapon_flashbang",
    "weapon_molotov",
    "weapon_smokegrenade",
    "weapon_hegrenade",
    "weapon_decoy",
    "weapon_incgrenade",
};

char grenade_shortname[][32] = {
    "flashbang",
    "molotov",
    "smokegrenade",
    "hegrenade",
    "decoy",
    "incgrenade",
};

stock int GetNadeNameByIndex(int index, char[] name, int maxlen)
{
    return strcopy(name, maxlen, weapon_grenade[index]);
}

stock int GetNadeIndexByName(const char[] name)
{
    if (name[0] != 'w') {
        for (int i = 0; i < sizeof(grenade_shortname); ++i) {
            if (StrEqual(grenade_shortname[i], name)) {
                return i;
            }
        }
        return -1;
    }

    if (strncmp(name, "weapon_", 7) != 0) {
        return -1;
    }

    for (int i = 0; i < sizeof(weapon_grenade); ++i) {
        if (StrEqual(weapon_grenade[i][7], name[7])) {
            return i;
        }
    }
    return -1;
}

#define MAX_WEAPONS 64

enum WeaponDefinitionIndex
{
    WEAPON_M4A1 = 16,
    WEAPON_M4A1_SILENCER = 60,
}

stock int GetPlayerWeaponByName(int client, const char[] name, int &index=0)
{
    static int m_hMyWeapons = -1;
    if (m_hMyWeapons == -1) {
        m_hMyWeapons = GetEntSendPropOffs(client, "m_hMyWeapons");
    }

    char classname[64];
    for (int i = 0; i < MAX_WEAPONS; ++i) {
        int weapon = GetEntDataEnt2(client, m_hMyWeapons + i*4);
        if (weapon > MaxClients && IsValidEntity(weapon)) {
            GetEntityClassname(weapon, classname, sizeof(classname));
            if (StrEqual(classname, name)) {
                index = i;
                return EntIndexToEntRef(weapon);
            }
        }
    }
    return INVALID_ENT_REFERENCE;
}

stock int GetNadeCounts(int client)
{
    int pos = 0;
    int[] countAmmo = new int[sizeof(grenadeAmmoOffsets)];

    int iCounts = 0;
    for (int i = 0; i < sizeof(grenadeAmmoOffsets); ++i) {
        int offset = grenadeAmmoOffsets[i];
        bool skipped = false;
        for (int j = 0; j < pos; ++j) {
            if (countAmmo[j] == offset) {
                skipped = true;
                break;
            }
        }

        if (skipped) {
            continue;
        }

        iCounts += GetEntProp(client, Prop_Data, "m_iAmmo", _, offset);
        countAmmo[pos++] = offset;
    }
    return iCounts;
}

stock int GetPlayerNextNade(int client, int skip_current = -1)
{
    for (int i = 0; i < sizeof(grenadeAmmoOffsets); ++i) {
        if (skip_current == i) {
            continue;
        }

        if (GetPlayerNadeAmmo(client, i)) {
            return i;
        }
    }

    return -1;
}

stock bool TryEquipPlayerSloWeapon(int client, int slot)
{
    if (!IsValidClient(client)) {
        return false;
    }

    int weapon = GetPlayerWeaponSlot(client, slot);
    if (!IsValidEntity(weapon)) {
        return false;
    }

    EquipPlayerWeapon(client, weapon);
    return true;
}

stock bool TryEquipPlayerSloWeaponEx(int client, int slot)
{
    if (!IsValidClient(client)) {
        return false;
    }

    int weapon = GetPlayerWeaponSlot(client, slot);
    if (!IsValidEntity(weapon)) {
        return false;
    }

    char classname[64];
    GetEntityClassname(weapon, classname, sizeof(classname));
    FakeClientCommandEx(client, "use %s", classname);
    // EquipPlayerWeapon(client, weapon);
    return true;
}

stock int ClearGroundWeapon()
{
    STATIC_OFFSET(WeaponParent, "CBaseCombatWeapon", "m_hOwnerEntity")
	char sWeapon[64];
	int iClear = 0;
	for (int i=MaxClients; i<GetMaxEntities();i++)
	{
		if (IsValidEdict(i) && IsValidEntity(i))
		{
			GetEdictClassname(i, weapon, sizeof(weapon));
			if (strcmp(sWeapon, "weapon_",7,false) == 0 && GetEntDataEnt2(i, WeaponParent) == -1) // 
			{
				int IHammerID = GetEntProp(i, Prop_Data, "m_iHammerID");

				if (IHammerID > 0)
				{
					continue;
				}
				iClear++;
				AcceptEntityInput(i, "Kill");
			}
		}
	}
	return iClear;
}

stock void RemoveNades(int client)
{
    while (RemoveWeaponBySlot(client, CS_SLOT_GRENADE)){}
    for (int i = 0; i < sizeof(grenadeAmmoOffsets); ++i) {
        SetEntProp(client, Prop_Send, "m_iAmmo", 0, _, grenadeAmmoOffsets[i]);
    }
}

stock bool RemoveWeaponBySlot(int client, int slot)
{
    int entity = GetPlayerWeaponSlot(client, slot);
    if (IsValidEntity(entity)) {
        RemovePlayerItem(client, entity);
        AcceptEntityInput(entity, "Kill");
        return true;
    }
    return false;
}

stock void RemoveCustomWeapon(int client)
{
    int ent;
    char clsname[32];
    // loop player weapons
    int size = GetEntPropArraySize(client, Prop_Send, "m_hMyWeapons");
    for (int a = 0; a < size; a++) {
        if ((ent = GetEntPropEnt(client, Prop_Send, "m_hMyWeapons", a)) != -1) {
            GetEntityClassname(ent, clsname, sizeof(clsname));
            
            int res = 0;
            if (customWeaponRemoveList.GetValue(clsname, res) && res == 1) {
                RemovePlayerItem(client, ent);
                AcceptEntityInput(ent, "Kill");
            }
        }
    }
}

stock void RefreshPlayerActiveWeapon(int client)
{
    int active = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (IsValidEntity(active)) {
        return;
    }

    static int m_hMyWeapons = -1;
    if (m_hMyWeapons == -1) {
        m_hMyWeapons = GetEntSendPropOffs(client, "m_hMyWeapons");
    }

    logger.Info("Client dose not have a valid active weapon");

    char classname[64];
    for (int i = 0; i < MAX_WEAPONS; ++i) {
        int weapon = GetEntDataEnt2(client, m_hMyWeapons + i*4);
        if (weapon > MaxClients && IsValidEntity(weapon)) {
            SetEntPropEnt(client, Prop_Send, "m_hActiveWeapon", weapon);
            return;
        }
    }

    logger.Warning("Client dose not have any weapons.");

    int knife = GivePlayerItem(client, "weapon_knife");
    if (!IsValidEntity(knife)) {
        logger.Error("Unable to give player weapon");
    } else {
        SetEntPropEnt(client, Prop_Send, "m_hActiveWeapon", knife);
    }
}

// this should be client index
stock int GetWeaponOwner(int weapon)
{
    static int m_hOwner = -1;
    if (m_hOwner == -1) {
        m_hOwner = FindSendPropInfo("CWeaponCSBase", "m_hOwner");
    }

    return GetEntDataEnt2(weapon, m_hOwner);
}

stock int GetWeaponClip1(int weapon)
{
    static int m_iClip1 = -1;
    if (m_iClip1 == -1) {
        m_iClip1 = FindSendPropInfo("CWeaponCSBase", "m_iClip1");
    }
        
    return GetEntData(weapon, m_iClip1);
}

stock void SetWeaponClip1(int weapon, int ammo)
{
    static int m_iClip1 = -1;
    if (m_iClip1 == -1) {
        m_iClip1 = FindSendPropInfo("CWeaponCSBase", "m_iClip1");
    }
    SetEntData(weapon, m_iClip1, ammo, 4, true);
}

stock int GetWeaponReserveAmmo(int weapon)
{
    static int m_iPrimaryReserveAmmoCount = -1;
    if (m_iPrimaryReserveAmmoCount == -1) {
        m_iPrimaryReserveAmmoCount = FindSendPropInfo("CWeaponCSBase", "m_iPrimaryReserveAmmoCount");
    }
    return GetEntData(weapon, m_iPrimaryReserveAmmoCount);
}

stock void SetWeaponReserveAmmo(int weapon, int ammo)
{
    static int m_iPrimaryReserveAmmoCount = -1;
    if (m_iPrimaryReserveAmmoCount == -1) {
        m_iPrimaryReserveAmmoCount = FindSendPropInfo("CWeaponCSBase", "m_iPrimaryReserveAmmoCount");
    }
    SetEntData(weapon, m_iPrimaryReserveAmmoCount, ammo, 4, true);
}

stock int GetPlayerReserveAmmo(int client, int weapon)
{
    int m_iPrimaryAmmoType = GetEntProp(weapon, Prop_Send, "m_iPrimaryAmmoType");
    if (m_iPrimaryAmmoType == -1) {
        return -1;
    }
    return GetEntProp(client, Prop_Send, "m_iAmmo", _, m_iPrimaryAmmoType);
}

stock int SetPlayerReserveAmmo(int client, int weapon, int ammo)
{
    int m_iPrimaryAmmoType = GetEntProp(weapon, Prop_Send, "m_iPrimaryAmmoType");
    if (m_iPrimaryAmmoType == -1) {
        return;
    }

    SetEntProp(client, Prop_Send, "m_iAmmo", ammo, _, m_iPrimaryAmmoType);
}

stock int GetItemDefIndex(int weapon)
{
    static int m_iItemDefinitionIndex = -1;
    if (m_iItemDefinitionIndex == -1) {
        m_iItemDefinitionIndex = FindSendPropInfo("CWeaponCSBase", "m_iItemDefinitionIndex");
    }
    return GetEntData(weapon, m_iItemDefinitionIndex);
}

stock float GetWeaponNextPrimaryAttack(int weapon)
{
    static int m_iItemDefinitionIndex = -1;
    if (m_iItemDefinitionIndex == -1) {
        m_iItemDefinitionIndex = FindSendPropInfo("CWeaponCSBase", "m_flNextPrimaryAttack");
    }
    return GetEntDataFloat(weapon, m_iItemDefinitionIndex);
}

stock void SetWeaponNextPrimaryAttack(int weapon, float time)
{
    static int m_iItemDefinitionIndex = -1;
    if (m_iItemDefinitionIndex == -1) {
        m_iItemDefinitionIndex = FindSendPropInfo("CWeaponCSBase", "m_flNextPrimaryAttack");
    }
    SetEntDataFloat(weapon, m_iItemDefinitionIndex, time);
}

stock void GivePlayerNades(int client, int nade, int amount)
{
    int current = GetEntProp(client, Prop_Send, "m_iAmmo", _, grenadeAmmoOffsets[nade]);
    if (current == 0) {
        GivePlayerItem(client, weapon_grenade[nade]);
    }
    SetEntProp(client, Prop_Send, "m_iAmmo", current + amount, _, grenadeAmmoOffsets[nade]);
}

stock int GetPlayerNadeAmmo(int client, int nade)
{
    return GetEntProp(client, Prop_Send, "m_iAmmo", _, grenadeAmmoOffsets[nade]);
}

stock void SetPlayerNadeAmmo(int client, int nade, int amount)
{
    SetEntProp(client, Prop_Send, "m_iAmmo", amount, _, grenadeAmmoOffsets[nade]);
}

stock int GetPlayerArmor(int client, bool &hasHelmet = false)
{
    static int m_ArmorValue = -1;
    if (m_ArmorValue == -1) {
        m_ArmorValue = FindSendPropInfo("CCSPlayer", "m_ArmorValue");
    }

    static int m_bHasHelmet = -1;
    if (m_bHasHelmet == -1) {
        m_bHasHelmet = FindSendPropInfo("CCSPlayer", "m_bHasHelmet");
    }

    hasHelmet = GetEntData(client, m_bHasHelmet) != 0;
    return GetEntData(client, m_ArmorValue);
}

stock void SetPlayerArmor(int client, int armor, bool helmet = true)
{
    static int m_ArmorValue = -1;
    if (m_ArmorValue == -1) {
        m_ArmorValue = FindSendPropInfo("CCSPlayer", "m_ArmorValue");
    }

    static int m_bHasHelmet = -1;
    if (m_bHasHelmet == -1) {
        m_bHasHelmet = FindSendPropInfo("CCSPlayer", "m_bHasHelmet");
    }

    SetEntData(client, m_ArmorValue, armor);
    SetEntData(client, m_bHasHelmet, helmet ? 1 : 0);
}

stock void SetPlayerKevlar(int client, int armor)
{
    static int m_ArmorValue = -1;
    if (m_ArmorValue == -1) {
        m_ArmorValue = FindSendPropInfo("CCSPlayer", "m_ArmorValue");
    }

    SetEntData(client, m_ArmorValue, armor);
}

stock void SetPlayerHelmet(int client, bool helmet)
{
    static int m_bHasHelmet = -1;
    if (m_bHasHelmet == -1) {
        m_bHasHelmet = FindSendPropInfo("CCSPlayer", "m_bHasHelmet");
    }

    SetEntData(client, m_bHasHelmet, helmet ? 1 : 0);
}

stock void RadioLocationMessage(int client, const char[] msg, const char[] location)
{
    char author[64];
    Format(author, sizeof(author), "#ENTNAME[%d]%N", client, client);

    Protobuf fwd = view_as<Protobuf>(StartMessageAll("RadioText"));
    fwd.SetInt("msg_dst", 3);
    fwd.SetInt("client", client);
    fwd.SetString("msg_name", "#Game_radio_location");
    fwd.AddString("params", author);
    fwd.AddString("params", location);
    fwd.AddString("params", msg);
    fwd.AddString("params", "");
    EndMessage();
}

stock void CopyArrayf(float[] dst, const float[] src, int size)
{
    for (int i = 0; i < size; ++i) {
        dst[i] = src[i];
    }
}

stock void PaintBeam(const float[] start, const float[] end, Color color=RED, float life = 25.0)
{
    BeamInfo info;
    CopyArrayf(info.Start, start, 3);
    CopyArrayf(info.End, end, 3);
    info.BeamColor = color;
    info.Life = life;

    if (beamQueue.Length == 0) {
        RequestFrame(PaintBeamOnFrame);
    }

    beamQueue.PushArray(info, sizeof(BeamInfo));
}

public void PaintBeamOnFrame()
{
    int n = beamQueue.Length;
    if (n == 0) {
        return;
    }

    if (n > 24) {
        n = 24;
    }

    BeamInfo info;
    for (int i = 0; i < n; ++i) {
        beamQueue.GetArray(i, info, sizeof(BeamInfo));
        PaintBeamInner(info.Start, info.End, info.BeamColor, info.Life);
    }

    for (int i = 0; i < n; ++i) {
        beamQueue.Erase(0);
    }

    if (beamQueue.Length > 0) {
        RequestFrame(PaintBeamOnFrame);
    }
}

stock void PaintBeamInner(const float start[3], const float end[3], Color color=RED, float life = 25.0, bool print = false)
{
    int tracerColor[4] = {255, 0, 0, 255};
    switch (color) {
        case RED:
        {
            tracerColor = {255, 0, 0, 255};
        }
        case GREEN:
        {
            tracerColor = {0, 255, 0, 255};
        }
        case BLUE:
        {
            tracerColor = {12, 12, 255, 255};
        }
        case YELLOW:
        {
            tracerColor = {255, 255, 0, 255};
        }
        case CYAN:
        {
            tracerColor = {0, 255, 255, 255};
        }
        case WHITE:
        {
            tracerColor = {255, 255, 255, 255};
        }
        case PURPLE:
        {
            tracerColor = {92, 36, 153, 255};
        }
    }

    if (print) {
        logger.Debug("start: %.1f %.1f %.1f end: %.1f %.1f %.1f color: %d %d %d", 
            start[0], start[1], start[2],
            end[0], end[1], end[2],
            tracerColor[1], tracerColor[2], tracerColor[3]
        );
    }

    TE_SetupBeamPoints(start, end, iBeam, 0, 0, 0, life, 1.0, 1.0, 1, 0.0, tracerColor, 0);
    TE_SendToAll();
}

enum CSWeaponMode
{
    Primary_Mode = 0,
    Secondary_Mode,
    WeaponMode_MAX
};

#define KNIFE_RANGE_LONG 48.0
#define KNIFE_RANGE_SHORT 32.0

// Return True if attacker could backstab, the player attacker hits is victim
// Return False if attacker could not backstab, victim will set if attacker could hit someone
stock bool CouldSwingOrStab(int attacker, CSWeaponMode mode, int &victim = 0)
{
    float fwd[3], eye[3], src[3], end[3];

    GetClientEyeAngles(attacker, eye);
    GetAngleVectors(eye, fwd, NULL_VECTOR, NULL_VECTOR);
    ScaleVector(fwd, mode == Primary_Mode ? KNIFE_RANGE_LONG : KNIFE_RANGE_SHORT);
    
    GetClientEyePosition(attacker, src);
    AddVectors(src, fwd, end);

    TR_TraceRayFilter(src, end, MASK_SOLID, RayType_EndPoint, BackStabTrayFilter, attacker);
    if (!TR_DidHit()) {
        float head_hull_mins[] = { -16.0, -16.0, -18.0 };
        float head_hull_maxs[] = {  16.0,  16.0,  18.0 };
        TR_TraceHullFilter(src, end, head_hull_mins, head_hull_maxs,
            MASK_SOLID, BackStabTrayFilter, attacker);
    }

    int entity = TR_GetEntityIndex();
    if (!IsValidClient(entity)) {
        return false;
    }

    victim = entity;

    float targetFwd[3];
    GetClientEyeAngles(victim, eye);
    GetAngleVectors(eye, targetFwd, NULL_VECTOR, NULL_VECTOR);

    float attackerPos[3], targetPos[3], vecLOS[3];
    GetClientAbsOrigin(attacker, attackerPos);
    GetClientAbsOrigin(victim, targetPos);

    SubtractVectors(targetPos, attackerPos, vecLOS);
    vecLOS[2] = 0.0;

    NormalizeVector(vecLOS, vecLOS);

    float flDot = vecLOS[0]*targetFwd[0] + vecLOS[1]*targetFwd[1];
    return flDot > 0.475;
}

public bool BackStabTrayFilter(int entity, int contentsMask, any data)
{
    return entity != data;
}

stock int GetPlayerActiveWeapon(int client)
{
    static int m_hActiveWeapon = -1;
    if (m_hActiveWeapon == -1) {
        m_hActiveWeapon = GetSendPropOffset("CBasePlayer", "m_hActiveWeapon");
    }

    int weapon = GetEntDataEnt2(client, m_hActiveWeapon);
    if (IsValidEntity(weapon)) {
        return EntIndexToEntRef(weapon);
    }
    return INVALID_ENT_REFERENCE;
}

stock int GetPlayerWeaponBySlot(int client, int slot)
{
    int weapon = GetPlayerWeaponSlot(client, slot);
    if (IsValidEntity(weapon)) {
        return EntIndexToEntRef(weapon);
    }
    return INVALID_ENT_REFERENCE;
}

stock int GetPlayerMoney(int client)
{
    static int m_iAccount = -1;
    if (m_iAccount == -1) {
        m_iAccount = GetSendPropOffset("CCSPlayer", "m_iAccount");
    }
    return GetEntData(client, m_iAccount);
}

stock void GivePlayerMoney(int client, int amount)
{
    if (amount == 0) {
        return;
    }

    static int m_iAccount = -1;
    if (m_iAccount == -1) {
        m_iAccount = GetSendPropOffset("CCSPlayer", "m_iAccount");
    }

    int money = GetEntData(client, m_iAccount) + amount;
    if (money > 65535) {
        SetEntData(client, m_iAccount, 65535, 4, true);
    } else if (money < 0) {
        SetEntData(client, m_iAccount, 0, 4, true);
    } else {
        SetEntData(client, m_iAccount, money, 4, true);
    }
}

stock void SetPlayerMoney(int client, int money)
{
    static int m_iAccount = -1;
    if (m_iAccount == -1) {
        m_iAccount = GetSendPropOffset("CCSPlayer", "m_iAccount");
    }

    if (money > 65535) {
        SetEntData(client, m_iAccount, 65535, 4, true);
    } else if (money < 0) {
        SetEntData(client, m_iAccount, 0, 4, true);
    } else {
        SetEntData(client, m_iAccount, money, 4, true);
    }
}

stock float GetSensorTime(int client)
{
    static int m_flDetectedByEnemySensorTime = -1;
    if (m_flDetectedByEnemySensorTime == -1) {
        m_flDetectedByEnemySensorTime = FindSendPropInfo("CCSPlayer", "m_flDetectedByEnemySensorTime");
    }

    return GetEntDataFloat(client, m_flDetectedByEnemySensorTime);
}

bool senorTimeFrameRequested = false;
float playerSenorTime[MAXPLAYERS + 1];

stock void SetSenorTime(int client, float expiretime)
{
    playerSenorTime[client] = expiretime;
    if (!senorTimeFrameRequested) {
        RequestFrame(SetSenorTimeFrame);
        senorTimeFrameRequested = true;
    }
}

public void SetSenorTimeFrame()
{
    senorTimeFrameRequested = false;

    int quota = 8;

    for (int i = 1; i <= MaxClients; ++i) {
        if (playerSenorTime[i] > 0.0 && IsValidClient(i)) {
            if (quota > 0) {
                InteralSetSenorTime(i, playerSenorTime[i]);
                playerSenorTime[i] = 0.0;
                
            }
            --quota;
        }
        else
        {
            playerSenorTime[i] = 0.0;
        }
    }

    if (quota <= 0) {
        RequestFrame(SetSenorTimeFrame);
        senorTimeFrameRequested = true;
    }
}

stock void InteralSetSenorTime(int client, float expiretime)
{
    static int m_flDetectedByEnemySensorTime = -1;
    if (m_flDetectedByEnemySensorTime == -1) {
        m_flDetectedByEnemySensorTime = FindSendPropInfo("CCSPlayer", "m_flDetectedByEnemySensorTime");
    }

    SetEntDataFloat(client, m_flDetectedByEnemySensorTime, expiretime, true);
}

stock void CreateBeamRing(const float pos[3], float range, float duration, const int color[4], 
    float width=5.0, float amplitude=0.0, float startRange = 8.0, int speed = 10)
{
    TE_SetupBeamRingPoint(pos, startRange, range, 
        iBeamSprite, iHaloSprite, 0, 0, duration, width, amplitude, color, speed, 0);
    TE_SendToAll();
}

stock float ExpectationWithTrail(float probability, int trails)
{
    return 1.0 - Pow(1.0 - probability, float(trails));
}

stock bool TracePlayerEye(int client, float target[3]) 
{
    float angle[3], start[3];
    GetClientEyePosition(client, start);
    GetClientEyeAngles(client, angle);
    TR_TraceRayFilter(start, angle, MASK_SHOT, RayType_Infinite, TraceEntityFilterExcept, client);
    if (TR_DidHit(INVALID_HANDLE)) {
        TR_GetEndPosition(target, INVALID_HANDLE);
        return true;
    }
    return false;
}

/* #region RayTrace Filters */
public bool TraceEntityFilterExcept(int entity, int contentsMask, int source)
{
    return source != entity;
}

public bool TraceEntityFilterPlayer(int entity, int contentsMask)
{
    return (entity > MaxClients || !entity);
}
/* #endregion */


stock void TE_DispatchEffect(const char[] particle, const float pos[3], const float endpos[3], const float angles[3] = NULL_VECTOR)
{
    TE_Start("EffectDispatch");
    TE_WriteFloatArray("m_vStart.x", pos, 3);
    TE_WriteFloatArray("m_vOrigin.x", endpos, 3);
    TE_WriteVector("m_vAngles", angles);
    TE_WriteNum("m_nHitBox", GetParticleEffectIndex(particle));
    TE_WriteNum("m_iEffectName", GetEffectIndex("ParticleEffect"));
}

stock  int GetParticleEffectIndex(const char[] sEffectName)
{
    static int table = INVALID_STRING_TABLE;

    if (table == INVALID_STRING_TABLE)
        table = FindStringTable("ParticleEffectNames");

    int iIndex = FindStringIndex(table, sEffectName);

    if (iIndex != INVALID_STRING_INDEX)
        return iIndex;

    return 0;
}

stock void PrecacheEffect(const char[] sEffectName)
{
    static int table = INVALID_STRING_TABLE;

    if (table == INVALID_STRING_TABLE)
        table = FindStringTable("EffectDispatch");

    bool save = LockStringTables(false);
    AddToStringTable(table, sEffectName);
    LockStringTables(save);
}

stock int GetEffectIndex(const char[] sEffectName)
{
    static int table = INVALID_STRING_TABLE;

    if (table == INVALID_STRING_TABLE)
        table = FindStringTable("EffectDispatch");

    int iIndex = FindStringIndex(table, sEffectName);

    if (iIndex != INVALID_STRING_INDEX)
        return iIndex;

    return 0;
}

stock void PrecacheParticleEffect(const char[] sEffectName)
{
    static int table = INVALID_STRING_TABLE;

    if (table == INVALID_STRING_TABLE)
        table = FindStringTable("ParticleEffectNames");

    bool save = LockStringTables(false);
    AddToStringTable(table, sEffectName);
    LockStringTables(save);
}

stock void DrawText(const char[] str, const float pos[3], float fontSize = 72.0, Color color=RED, float life = 25.0,
    const float ang[3] = { 0.0, 0.0, 0.0 }, const float center[2] = { 0.5, 0.0 })
{
    DataPack dp = new DataPack();
    dp.WriteCell(color);
    dp.WriteFloat(life);

    DefalutFont.DrawText(DefalutEmojiFont, str, fontSize, center, pos, ang, OnCharVerticesCallback, dp);

    delete dp;
}

public void OnCharVerticesCallback(int segments, const int[] seglen, const float[][] vertices, int codepoint, DataPack dp)
{
    dp.Reset();

    Color color = dp.ReadCell();
    float life = dp.ReadFloat();

    int p = 0;
    for (int i = 0; i < segments; ++i) {
        ++p;
        for (int j = 1; j < seglen[i]; ++j) {
            PaintBeam(vertices[p-1], vertices[p], color, life);
            ++p;
        }
    }
}

stock void PrintToConsoleAllEx(const char[] format, any ...)
{
    char buf[1024];
    VFormat(buf, sizeof(buf), format, 2);

    DataPack dp = new DataPack();
    dp.WriteString(buf);

    if (consolePrintQueue[0] == null) {
        consolePrintQueue[0] = new ArrayList();
    }
    consolePrintQueue[0].Push(dp);

    if (!consolePrintRequested) {
        consolePrintRequested = true;
        CreateTimer(0.1, Timer_OnPrinToConsole, _, TIMER_FLAG_NO_MAPCHANGE);
    }
}

stock void PrintToConsoleEx(int client, const char[] format, any ...)
{
    char buf[1024];
    VFormat(buf, sizeof(buf), format, 3);

    DataPack dp = new DataPack();
    dp.WriteString(buf);

    if (consolePrintQueue[client] == null) {
        consolePrintQueue[client] = new ArrayList();
    }
    consolePrintQueue[client].Push(dp);

    if (!consolePrintRequested) {
        consolePrintRequested = true;
        CreateTimer(0.1, Timer_OnPrinToConsole, _, TIMER_FLAG_NO_MAPCHANGE);
    }
}

public Action Timer_OnPrinToConsole(Handle hTimer)
{
    consolePrintRequested = false;

    char buf[1024];
    for (int client = 0; client <= MaxClients; ++client) {
        if (consolePrintQueue[client] == null) {
            continue;
        }

        int len = consolePrintQueue[client].Length;
        if (len == 0) {
            delete consolePrintQueue[client];
            consolePrintQueue[client] = null;
            continue;
        }

        bool valid = IsValidClient(client) || client == 0;

        ArrayList tempQueue = null;
        if (valid) {
            tempQueue = new ArrayList();
        }

        for (int i = 0; i < len; ++i) {
            DataPack dp = consolePrintQueue[client].Get(i);
            if (dp == null) {
                continue;
            }

            if (!valid) {
                delete dp;
                continue;
            }

            if (i < 32) {
                dp.Reset();
                dp.ReadString(buf, sizeof(buf));
                if (client == 0) {
                    PrintToConsoleAll(buf);
                } else {
                    PrintToConsole(client, buf);
                }
                delete dp;
            } else {
                tempQueue.Push(dp);
            }
        }

        delete consolePrintQueue[client];
        consolePrintQueue[client] = tempQueue;

        if (tempQueue != null && !consolePrintRequested) {
            consolePrintRequested = true;
            CreateTimer(0.1, Timer_OnPrinToConsole, _, TIMER_FLAG_NO_MAPCHANGE);
        }
    }
}

bool TraceHit(const float start[3], const float end[3], float hit[3] = {0.0, 0.0, 0.0}, int source = -1, float normal[3] = {0.0,0.0,0.0})
{
    if (source == -1) {
        TR_TraceRayFilter(start, end, MASK_SOLID, RayType_EndPoint, TraceEntityFilterPlayer);
    } else {
        TR_TraceRayFilter(start, end, MASK_SOLID, RayType_EndPoint, TraceEntityFilterExcept, source);
    }

    if (TR_DidHit(INVALID_HANDLE)) {
        TR_GetPlaneNormal(INVALID_HANDLE, normal);
        TR_GetEndPosition(hit, INVALID_HANDLE);
        return true;
    }
    return false;
}

enum TraceTargetMode
{
    Eye        = 1 << 1,
    Body       = 1 << 2,
    Origin     = 1 << 3,
    Leg        = Origin,
    Full       = Eye | Body | Leg,
    MaxTrace   = 3,
};

bool TraceClientTarget(int client, int target, TraceTargetMode mode = Body)
{
    float srcEye[3], srcBody[3], srcLeg[3];
    float dstEye[3], dstBody[3], dstLeg[3];

    int mask = view_as<int>(mode);
    for (int i = 1; i <= MaxTrace; ++i) {
        if ((1 << i) & mask) {

        }
    }
}

public bool TryGetPluginStatus(Handle plugin)
{
    return GetPluginStatus(plugin) == Plugin_Running;
}

stock bool IsPluginRunning(Handle plugin)
{
    // assume null is self, and self always running
    if (plugin == null || GetMyHandle() == plugin) {
        return true;
    }

    static Function func = INVALID_FUNCTION;
    if (func == INVALID_FUNCTION) {
        func = GetFunctionByName(null, "TryGetPluginStatus");
    }

    Call_StartFunction(null, func);
    Call_PushCell(plugin);
    bool result = false;
    return Call_Finish(result) == SP_ERROR_NONE && result;
}

stock bool IsPluginAvaiable(const char[] smx)
{
    return FindPluginByFile(smx) != INVALID_HANDLE;
}

stock float QuantileOfSortedArray(ArrayList arr, float q = 0.5)
{
    int n = arr.Length;

    float index = n * q;
    int a = RoundToFloor(index);

    float frac = index - a;
    if (frac < 0.001) {
        return arr.Get(a);
    }

    int b = RoundToCeil(index);
    if (b >= n) {
        return arr.Get(a);
    }

    float v1 = arr.Get(a);
    float v2 = arr.Get(b);
    return v1 + frac * (b - a);
}

stock int ToLower(const char[] str, char[] output, int maxlen)
{
    int p = 0;
    while (str[p] != '\0') {
        if (str[p] < 0x7F) {
            output[p] = CharToLower(str[p]);
        } else {
            output[p] = str[p];
        }
        ++p;
    }
    return p;
}

stock Address GetEntityVFunction(int entity, int offset)
{
    Address pvtable = GetEntityAddress(entity);
    int vtable = LoadFromAddress(pvtable, NumberType_Int32);
    int pFuncAddr = vtable + offset * 4;
    int addr = LoadFromAddress(view_as<Address>(pFuncAddr), NumberType_Int32);
    return view_as<Address>(addr);
}

stock bool PatchCode(int address, const int[] original, int origin_len, const int[] patch, int patch_len, bool setfail = true)
{
    // check origin first
    int patch_match = 0;
    for (int i = 0; i < patch_len; ++i) {
        patch_match += LoadFromAddress(view_as<Address>(address + i), NumberType_Int8) == patch[i] ? 1 : 0;
    }

    if (patch_match == patch_len) {
        return true;
    }

    for (int i = 0; i < origin_len; ++i) {
        if (LoadFromAddress(view_as<Address>(address + i), NumberType_Int8) != original[i]) {
            LogError("Original code was changed. Please update patch code");
            if (setfail) {
                SetFailState("Original code was changed. Please update patch code");
            }
            return false;
        }
    }

    for (int i = 0; i < patch_len; ++i) {
        StoreToAddress(view_as<Address>(address + i), patch[i], NumberType_Int8);
    }

    return true;
}
