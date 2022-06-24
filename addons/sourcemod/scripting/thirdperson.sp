#pragma semicolon 1

#define PLUGIN_NAME "Thirdperson | Mirrow Mode"
#define PLUGIN_AUTHOR "Zephyrus"
#define PLUGIN_DESCRIPTION "Thirdperson mode | Mirrow Mode"
#define PLUGIN_VERSION "1.13"
#define PLUGIN_URL ""

#include <sourcemod>
#include <sdktools>

#pragma newdecls required


int TPS_Client[MAXPLAYERS+1] = {0,...};

bool g_bThirdperson[MAXPLAYERS+1] = {false,...};
bool mirror[MAXPLAYERS+1] = {false,...};
bool b_enabled;
ConVar g_enabled;

int AllowTPCount = 0;
ConVar g_AllowTPCount;
bool g_bHidenSeekServer = false;

public Plugin myinfo = 
{
	name = PLUGIN_NAME,
	author = PLUGIN_AUTHOR,
	description = PLUGIN_DESCRIPTION,
	version = PLUGIN_VERSION,
	url = PLUGIN_URL
};

public void OnPluginStart()
{
	if(FindPluginByFile("hidenseek_final.smx") != null)
		g_bHidenSeekServer = true;

	if(CheckCanUse())
	{
		RegConsoleCmd("sm_tp", Command_TP);
		RegConsoleCmd("sm_fp", Command_FP);
		RegConsoleCmd("sm_mirror", Cmd_Mirror, "Toggle Rotational Thirdperson view");
		
	}
	
	
	
	g_enabled = CreateConVar("sm_store_thirdperson_enable", "1", "", 0, true, 0.0, true, 1.0);
	
	g_AllowTPCount = CreateConVar("sm_store_thirdperson_count", "0", "", 0, true, 0.0, true, 1.0);
	AllowTPCount = g_AllowTPCount.IntValue;
	
	HookEventEx("round_start", Event_RoundStart);
	
	b_enabled = GetConVarBool(g_enabled);
	HookConVarChange(g_enabled, HookConVar_Changed);
	HookConVarChange(g_AllowTPCount, HookConVar_Changed);
	
	AutoExecConfig();
	
}


public void Event_RoundStart(Handle event, char[] name, bool dontBroadcast)
{
	for(int i=1;i<=MaxClients;i++)
	{
		if(TPS_Client[i] > 0)
		{
			TPS_Client[i] = 0;
			g_bThirdperson[i] = false;
		}
	}
}



public void HookConVar_Changed(ConVar convar, char[] oldValue, char[] newValue)
{
	if(convar == g_enabled)
		b_enabled = view_as<bool>(StringToInt(newValue));
	else if(convar == g_AllowTPCount)
		AllowTPCount = view_as<int>(StringToInt(newValue));
	
	
}	

public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max)
{
	CreateNative("IsPlayerInTP", Native_IsPlayerInTP);
	CreateNative("TogglePlayerTP", Native_TogglePlayerTP);
	
	return APLRes_Success;
}

public int Native_IsPlayerInTP(Handle plugin, int numParams)
{
	int client = GetNativeCell(1);
	if(g_bThirdperson[client] || mirror[client])
		return true;
	
	return false;
}

public int Native_TogglePlayerTP(Handle plugin, int numParams)
{
	int client = GetNativeCell(1);
	g_bThirdperson[client] = !g_bThirdperson[client];
	ToggleThirdperson(client);
}

public void OnClientConnected(int client)
{
	TPS_Client[client] = 0;
	g_bThirdperson[client] = false;
	mirror[client] = false;
}

public Action Command_TP(int client, int args)
{
	if(!CheckCanUse())
		return Plugin_Handled;

	if(!b_enabled)
		return Plugin_Handled;
	
	if(client == 0 || !IsClientInGame(client))
		return Plugin_Handled;
	
	//Limit TP Count,限制TP使用次数
	if(AllowTPCount > 0 && !g_bThirdperson[client])
	{
		if(TPS_Client[client] >= AllowTPCount)
		{
			ClientCommand(client, "firstperson");
			PrintToChat(client,"每局最大TP = %d",AllowTPCount);
			
			if(mirror[client])
				SetMirror(client,false);
			return Plugin_Handled;
		}
		TPS_Client[client]++;
	}
	
	if(mirror[client])
		SetMirror(client,false);
	
	g_bThirdperson[client] = !g_bThirdperson[client];
	ToggleThirdperson(client);
	
	PrintToChat(client," \x04[提醒]\x01 \x05!tp 为标准第三视角模式 \x06!mirror 为镜像模式!");
	
	return Plugin_Handled;
}

public Action Command_FP(int client, int args)
{
	if(!b_enabled)
		return Plugin_Handled;

	if(client == 0 || !IsClientInGame(client))
		return Plugin_Handled;

	g_bThirdperson[client] = false;
	SetThirdperson(client, false);
	SetMirror(client,false);
	mirror[client] = false;
	
	return Plugin_Handled;
}



stock void ToggleThirdperson(int client)
{
	if(g_bThirdperson[client])
	{
		SetThirdperson(client, true);
	}
	else
	SetThirdperson(client, false);
}

stock void SetThirdperson(int client, bool tp)
{
	static Handle m_hAllowTP = INVALID_HANDLE;
	if(m_hAllowTP == INVALID_HANDLE)
		m_hAllowTP = FindConVar("sv_allow_thirdperson");
	
	SetConVarInt(m_hAllowTP, 1);
	
	if(tp)
	{
		//makeTP
		ClientCommand(client, "cam_collision 1");
		//tp_reset
		ClientCommand(client, "cam_idealyaw 0");
		ClientCommand(client, "cam_idealdist 150");
		ClientCommand(client, "cam_idealpitch 0");
		ClientCommand(client, "c_thirdpersonshoulder 0");
		//tp
		ClientCommand(client, "thirdperson");
	}
	else
	{
	ClientCommand(client, "firstperson");
	//tp_reset
	ClientCommand(client, "cam_idealyaw 0");
	ClientCommand(client, "cam_idealdist 150");
	ClientCommand(client, "cam_idealpitch 0");
	ClientCommand(client, "c_thirdpersonshoulder 0");
	}

}

//!Mirror

public Action Cmd_Mirror(int client, int args)
{
	if(!CheckCanUse())
		return Plugin_Handled;
	if(client == 0 || !IsClientInGame(client))
		return Plugin_Handled;

	//Limit TP Count
	if(AllowTPCount > 0 && !g_bThirdperson[client])
	{
		if(TPS_Client[client] >= AllowTPCount)
		{
			SetMirror(client,false);
			PrintToChat(client,"每局最大TP = %d",AllowTPCount);
			return Plugin_Handled;
		}
		TPS_Client[client]++;
	}
	
	
	
	if (!IsPlayerAlive(client))
	{
		ReplyToCommand(client, "[SM] 你必须在存活时使用.");
		return Plugin_Handled;
	}
	
	if(mirror[client])
		SetMirror(client,false);
	else SetMirror(client,true);
	
	return Plugin_Handled;
}


stock void SetMirror(int client, bool b_Mirror)
{
	if (!IsPlayerAlive(client))
	{
		ReplyToCommand(client, "[SM] 你必须在存活时使用.");
		return;
	}
	
	if(g_bThirdperson[client])
	{
		g_bThirdperson[client] = false;
		SetThirdperson(client, false);
	}
	
	if (b_Mirror)
	{
		//tp_reset
		ClientCommand(client, "cam_idealyaw -180");
		ClientCommand(client, "cam_idealdist 150");
		ClientCommand(client, "cam_idealpitch 0");
		ClientCommand(client, "c_thirdpersonshoulder 0");
		//tp
		ClientCommand(client, "thirdperson");
		
		// SetEntPropEnt(client, Prop_Send, "m_hObserverTarget", 0); 
		// SetEntProp(client, Prop_Send, "m_iObserverMode", 1);
		// SetEntProp(client, Prop_Send, "m_bDrawViewmodel", 0);
		// SetEntProp(client, Prop_Send, "m_iFOV", 90);
		// SendConVarValue(client, mp_forcecamera, "1");
		
		mirror[client] = true;
		ReplyToCommand(client, "[SM] 启用镜像模式.");
	}
	else
	{
		//tp_reset
		ClientCommand(client, "cam_idealyaw 0");
		ClientCommand(client, "cam_idealdist 150");
		ClientCommand(client, "cam_idealpitch 0");
		ClientCommand(client, "c_thirdpersonshoulder 0");
		
		//tp
		ClientCommand(client, "firstperson");
		
		mirror[client] = false;
		ReplyToCommand(client, "[SM] 禁用镜像模式.");
	}
	
}



//TTT
public void TTT_OnClientGetRole(int client, int role)
{
	if(IsClientInGame(client))
	{	
		TPS_Client[client] = AllowTPCount;
		
		if(g_bThirdperson[client])
		{
			ClientCommand(client, "firstperson");
		}
		
		if(mirror[client])
			SetMirror(client,false);
	}
}


bool CheckCanUse()
{
	if(g_bHidenSeekServer)
		return false;
	
	return true;
}