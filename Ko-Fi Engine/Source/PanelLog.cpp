#include "PanelLog.h"
#include "ImGuiAppLog.h"

PanelLog::PanelLog()
{
	panelName = "Log";
}

PanelLog::~PanelLog()
{
}

bool PanelLog::Update()
{
	OPTICK_EVENT();

	appLog->Draw("Log");

	return true;
}