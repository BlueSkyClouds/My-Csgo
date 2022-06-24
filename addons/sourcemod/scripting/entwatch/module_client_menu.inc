#define EW_MODULE_CLIENTMENU


stock void EWM_Client_MenuOnPluginStart()
{
    RegConsoleCmd("sm_hud", EWM_Client_Menu_MainMenu,"set entwatch hud client prefs");
    RegConsoleCmd("sm_entwatchmenu", EWM_Client_Menu_MainMenu,"set entwatch hud client prefs");
}

public Action EWM_Client_Menu_MainMenu(int client,int args)
{
    if((IsClientConnected(client) && IsClientInGame(client)))
    {

        Menu MenuMain = new Menu(Handle_MenuMain);

        MenuMain.SetTitle("[EntWatch]设置\n--------------");

        char sFormat[80];
        Format(sFormat,sizeof(sFormat),"显示神器列表: %s",g_CSettings_Hud[client].Display ? "开":"关");

        MenuMain.AddItem("HUD_Toggle", sFormat);
        MenuMain.AddItem("HUD_Pos", "HUD位置设置");
        MenuMain.AddItem("HUD_Color", "HUD颜色设置");

        Format(sFormat,sizeof(sFormat),"神器按钮优先使用: %s",g_bUsePriorityClient[client] ? "启用":"禁用");
        MenuMain.AddItem("HUD_Use", sFormat);

        MenuMain.ExitButton = true;
        MenuMain.Display(client, 0);
    }
    return Plugin_Handled;
}

static void EWM_Client_Menu_PosMenu(int client, int iPos = -1)
{
    Menu PosMneu = new Menu(Handle_PosMenu);

    PosMneu.SetTitle("[EntWatch]位置设置\n--------------");

    PosMneu.AddItem("up", "上移");
    PosMneu.AddItem("up2", "上移(微调)");
    PosMneu.AddItem("down", "下移");
    PosMneu.AddItem("down2", "下移(微调)");
    PosMneu.AddItem("left", "左移");
    PosMneu.AddItem("left2", "左移(微调)");
    PosMneu.AddItem("right", "右移");
    PosMneu.AddItem("right2", "右移(微调)");
    PosMneu.AddItem("reset", "恢复默认设置");


    PosMneu.ExitBackButton = true;
    if(iPos != -1)
        PosMneu.DisplayAt(client,iPos, 60);
    else PosMneu.Display(client, 60);
}
static void EWM_Client_Menu_ColorMenu(int client, int iPos = -1)
{
    Menu ColorMenu = new Menu(Handle_ColorMenu);

    ColorMenu.SetTitle("[EntWatch]颜色设置\n--------------");

    ColorMenu.AddItem("white", "白色(默认)");
    ColorMenu.AddItem("red", "红色");
    ColorMenu.AddItem("yellow", "黄色");
    ColorMenu.AddItem("green", "绿色");
    ColorMenu.AddItem("cyan", "青色");
    ColorMenu.AddItem("blue", "蓝色");
    ColorMenu.AddItem("purple", "紫色");

    ColorMenu.ExitBackButton = true;
    if(iPos != -1)
        ColorMenu.DisplayAt(client,iPos, 60);
    else ColorMenu.Display(client, 60);
}

public int Handle_MenuMain(Menu MenuMain, MenuAction action, int iClient, int param2)
{
    if (action == MenuAction_Select)
    {
        char info[32];
        GetMenuItem(MenuMain, param2, info, sizeof(info));
        if(StrEqual(info,"HUD_Pos",false))
        {
            EWM_Client_Menu_PosMenu(iClient);
        }
        if(StrEqual(info,"HUD_Color",false))
        {
            EWM_Client_Menu_ColorMenu(iClient);
        }
        if(StrEqual(info,"HUD_Toggle",false))
        {
            if (g_CSettings_Hud[iClient].Display)
            {
                CPrintToChat(iClient, "%s%t %s%t %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Hud", g_SchemeConfig.Color_Disabled, "Disabled");
                g_CSettings_Hud[iClient].Display = false;
                SetClientCookie(iClient, g_hCookie_Display, "0");
            }
            else
            {
                CPrintToChat(iClient, "%s%t %s%t %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Hud", g_SchemeConfig.Color_Enabled, "Enabled");
                g_CSettings_Hud[iClient].Display = true;
                SetClientCookie(iClient, g_hCookie_Display, "1");
            }
            EWM_Client_Menu_MainMenu(iClient,GetMenuSelectionPosition());
        }
        if(StrEqual(info,"HUD_Use",false))
        {
            if(g_bUsePriorityClient[iClient])
            {
                EWM_Use_Priority_Command(iClient,0);
            }
            else
            {
                EWM_Use_Priority_Command(iClient,0);
            }
            EWM_Client_Menu_MainMenu(iClient,GetMenuSelectionPosition());
        }

    }else if (action == MenuAction_End)
    {
        delete MenuMain;
    }
}


public int Handle_PosMenu(Menu hmenu, MenuAction action, int client, int param)
{
    if (action == MenuAction_Select)
    {
        char info[32];
        char sBuffer_cookie[32];
        char Explode_sParam[2][32];

        GetClientCookie(client, g_hCookie_HudPos, sBuffer_cookie, sizeof(sBuffer_cookie));
        ExplodeString(sBuffer_cookie, "/", Explode_sParam, 2, 32);

        float Pos_validate_X = StringToFloat(Explode_sParam[0]);
        float Pos_validate_Y = StringToFloat(Explode_sParam[1]);

        hmenu.GetItem(param, info, sizeof(info));
        if(Pos_validate_X >= 0.0 && Pos_validate_Y >= 0.0 && Pos_validate_X <= 1.0 && Pos_validate_Y <= 1.0)
        {
            if (StrEqual(info, "up")) {

                Pos_validate_Y -= 0.1;
                g_CSettings_Hud[client].Pos_X = Pos_validate_X;
                g_CSettings_Hud[client].Pos_Y = Pos_validate_Y;
                FormatEx(sBuffer_cookie, sizeof(sBuffer_cookie), "%f/%f", Pos_validate_X, Pos_validate_Y);

            }else if (StrEqual(info, "up2")) {

                Pos_validate_Y -= 0.01;
                g_CSettings_Hud[client].Pos_X = Pos_validate_X;
                g_CSettings_Hud[client].Pos_Y = Pos_validate_Y;
                FormatEx(sBuffer_cookie, sizeof(sBuffer_cookie), "%f/%f", Pos_validate_X, Pos_validate_Y);

            }else if (StrEqual(info, "down")) {

                Pos_validate_Y += 0.1;
                g_CSettings_Hud[client].Pos_X = Pos_validate_X;
                g_CSettings_Hud[client].Pos_Y = Pos_validate_Y;
                FormatEx(sBuffer_cookie, sizeof(sBuffer_cookie), "%f/%f", Pos_validate_X, Pos_validate_Y);

            }else if (StrEqual(info, "down2")) {

                Pos_validate_Y += 0.01;
                g_CSettings_Hud[client].Pos_X = Pos_validate_X;
                g_CSettings_Hud[client].Pos_Y = Pos_validate_Y;
                FormatEx(sBuffer_cookie, sizeof(sBuffer_cookie), "%f/%f", Pos_validate_X, Pos_validate_Y);

            }else if (StrEqual(info, "left")) {

                Pos_validate_X -= 0.1;
                g_CSettings_Hud[client].Pos_X = Pos_validate_X;
                g_CSettings_Hud[client].Pos_Y = Pos_validate_Y;
                FormatEx(sBuffer_cookie, sizeof(sBuffer_cookie), "%f/%f", Pos_validate_X, Pos_validate_Y);

            }else if (StrEqual(info, "left2")) {

                Pos_validate_X -= 0.01;
                g_CSettings_Hud[client].Pos_X = Pos_validate_X;
                g_CSettings_Hud[client].Pos_Y = Pos_validate_Y;
                FormatEx(sBuffer_cookie, sizeof(sBuffer_cookie), "%f/%f", Pos_validate_X, Pos_validate_Y);

            }else if (StrEqual(info, "right")) {

                Pos_validate_X += 0.1;
                g_CSettings_Hud[client].Pos_X = Pos_validate_X;
                g_CSettings_Hud[client].Pos_Y = Pos_validate_Y;
                FormatEx(sBuffer_cookie, sizeof(sBuffer_cookie), "%f/%f", Pos_validate_X, Pos_validate_Y);

            }else if (StrEqual(info, "right2")) {

                Pos_validate_X += 0.01;
                g_CSettings_Hud[client].Pos_X = Pos_validate_X;
                g_CSettings_Hud[client].Pos_Y = Pos_validate_Y;
                FormatEx(sBuffer_cookie, sizeof(sBuffer_cookie), "%f/%f", Pos_validate_X, Pos_validate_Y);

            }else if (StrEqual(info, "reset")) {

                g_CSettings_Hud[client].Pos_X = g_SchemeConfig.Pos_HUD_X;
                g_CSettings_Hud[client].Pos_Y = g_SchemeConfig.Pos_HUD_Y;
                FormatEx(sBuffer_cookie, sizeof(sBuffer_cookie), "%f/%f", g_SchemeConfig.Pos_HUD_X, g_SchemeConfig.Pos_HUD_Y);

            }
            SetClientCookie(client, g_hCookie_HudPos, sBuffer_cookie);
            EWM_Client_Menu_PosMenu(client,GetMenuSelectionPosition());
        }else{
            g_CSettings_Hud[client].Pos_X = g_SchemeConfig.Pos_HUD_X;
            g_CSettings_Hud[client].Pos_Y = g_SchemeConfig.Pos_HUD_Y;
            FormatEx(sBuffer_cookie, sizeof(sBuffer_cookie), "%f/%f", g_SchemeConfig.Pos_HUD_X, g_SchemeConfig.Pos_HUD_Y);
            SetClientCookie(client, g_hCookie_HudPos, sBuffer_cookie);
            EWM_Client_Menu_PosMenu(client,GetMenuSelectionPosition());
            PrintToChat(client, " \x04[EntWatch]\x07范围超出最大值已恢复默认设置");
        }


    }else if (param == MenuCancel_ExitBack){
        EWM_Client_Menu_MainMenu(client, 0);
    }else if(action == MenuAction_End){
        delete hmenu;
    }
}

public int Handle_ColorMenu(Menu hmenu, MenuAction action, int client, int param)
{
    if (action == MenuAction_Select)
    {
        char info[32];
        char sBuffer_cookie[32];

        hmenu.GetItem(param, info, sizeof(info));

        if (StrEqual(info, "white")) {
            
            g_CSettings_Hud[client].Color[0] = 255;
            g_CSettings_Hud[client].Color[1] = 255;
            g_CSettings_Hud[client].Color[2] = 255;
            g_CSettings_Hud[client].Color[3] = 255;
            Format(sBuffer_cookie, sizeof(sBuffer_cookie), "%i/%i/%i/%i", g_CSettings_Hud[client].Color[0], g_CSettings_Hud[client].Color[1], g_CSettings_Hud[client].Color[2], g_CSettings_Hud[client].Color[3]);

        } else if (StrEqual(info, "red")) {

            g_CSettings_Hud[client].Color[0] = 255;
            g_CSettings_Hud[client].Color[1] = 30;
            g_CSettings_Hud[client].Color[2] = 30;
            g_CSettings_Hud[client].Color[3] = 255;
            Format(sBuffer_cookie, sizeof(sBuffer_cookie), "%i/%i/%i/%i", g_CSettings_Hud[client].Color[0], g_CSettings_Hud[client].Color[1], g_CSettings_Hud[client].Color[2], g_CSettings_Hud[client].Color[3]);
        
        } else if (StrEqual(info, "yellow")) {
            
            g_CSettings_Hud[client].Color[0] = 255;
            g_CSettings_Hud[client].Color[1] = 255;
            g_CSettings_Hud[client].Color[2] = 0;
            g_CSettings_Hud[client].Color[3] = 255;
            Format(sBuffer_cookie, sizeof(sBuffer_cookie), "%i/%i/%i/%i", g_CSettings_Hud[client].Color[0], g_CSettings_Hud[client].Color[1], g_CSettings_Hud[client].Color[2], g_CSettings_Hud[client].Color[3]);

        } else if (StrEqual(info, "green")) {

            g_CSettings_Hud[client].Color[0] = 0;
            g_CSettings_Hud[client].Color[1] = 255;
            g_CSettings_Hud[client].Color[2] = 0;
            g_CSettings_Hud[client].Color[3] = 255;
            Format(sBuffer_cookie, sizeof(sBuffer_cookie), "%i/%i/%i/%i", g_CSettings_Hud[client].Color[0], g_CSettings_Hud[client].Color[1], g_CSettings_Hud[client].Color[2], g_CSettings_Hud[client].Color[3]);
        
        } else if (StrEqual(info, "cyan")) {

            g_CSettings_Hud[client].Color[0] = 0;
            g_CSettings_Hud[client].Color[1] = 255;
            g_CSettings_Hud[client].Color[2] = 255;
            g_CSettings_Hud[client].Color[3] = 255;
            Format(sBuffer_cookie, sizeof(sBuffer_cookie), "%i/%i/%i/%i", g_CSettings_Hud[client].Color[0], g_CSettings_Hud[client].Color[1], g_CSettings_Hud[client].Color[2], g_CSettings_Hud[client].Color[3]);
        
        } else if (StrEqual(info, "blue")) {

            g_CSettings_Hud[client].Color[0] = 0;
            g_CSettings_Hud[client].Color[1] = 0;
            g_CSettings_Hud[client].Color[2] = 255;
            g_CSettings_Hud[client].Color[3] = 255;
            Format(sBuffer_cookie, sizeof(sBuffer_cookie), "%i/%i/%i/%i", g_CSettings_Hud[client].Color[0], g_CSettings_Hud[client].Color[1], g_CSettings_Hud[client].Color[2], g_CSettings_Hud[client].Color[3]);
        
        } else if (StrEqual(info, "purple")) {

            g_CSettings_Hud[client].Color[0] = 164;
            g_CSettings_Hud[client].Color[1] = 32;
            g_CSettings_Hud[client].Color[2] = 240;
            g_CSettings_Hud[client].Color[3] = 255;
            Format(sBuffer_cookie, sizeof(sBuffer_cookie), "%i/%i/%i/%i", g_CSettings_Hud[client].Color[0], g_CSettings_Hud[client].Color[1], g_CSettings_Hud[client].Color[2], g_CSettings_Hud[client].Color[3]);

        }

        SetClientCookie(client, g_hCookie_HudColor, sBuffer_cookie);
        EWM_Client_Menu_ColorMenu(client,GetMenuSelectionPosition());

    }else if (param == MenuCancel_ExitBack){
        EWM_Client_Menu_MainMenu(client, 0);
    }else if(action == MenuAction_End){
        delete hmenu;
    }
}
