#define EW_MODULE_ELOGS

stock void EWM_ELogs_Use(class_ItemList ItemTest, int iClient)
{
	LogAction(iClient, -1, "\"%L\" used item \"%s\"", iClient, ItemTest.Name);
}

stock void EWM_ELogs_PickUp(class_ItemList ItemTest, int iClient)
{
	LogAction(iClient, -1, "\"%L\" has picked up item \"%s\"", iClient, ItemTest.Name);
}

stock void EWM_ELogs_Drop(class_ItemList ItemTest, int iClient)
{
	LogAction(iClient, -1, "\"%L\" has dropped item \"%s\"", iClient, ItemTest.Name);
}

stock void EWM_ELogs_Disconnect(class_ItemList ItemTest, int iClient)
{
	LogAction(iClient, -1, "\"%L\" disconnected with item \"%s\"", iClient, ItemTest.Name);
}

stock void EWM_ELogs_PlayerDeath(class_ItemList ItemTest, int iClient)
{
	LogAction(iClient, -1, "\"%L\" has died with item \"%s\"", iClient, ItemTest.Name);
}