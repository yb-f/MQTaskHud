// MQTaskHud.cpp : Defines the entry point for the DLL application.
//

// PLUGIN_API is only to be used for callbacks.  All existing callbacks at this time
// are shown below. Remove the ones your plugin does not use.  Always use Initialize
// and Shutdown for setup and cleanup.

#include <mq/Plugin.h>
#include "mq/api/ActorAPI.h"
#include "imgui/fonts/IconsMaterialDesign.h"
#include "MQTaskHud.h"


PreSetup("MQTaskHud");
PLUGIN_VERSION(0.1);

const int FIRST_WINDOW_WIDTH = 445;
const int FIRST_WINDOW_HEIGHT = 490;

postoffice::DropboxAPI THDropbox;

bool b_showMQTaskHudWindow = false;
bool b_debugMode = false;
int i_selectedCharIndex = 0;
int i_selectedPeerIndex = 0;
int i_selectedTaskIndex = 0;

//sample data, clear before release
std::vector<char*> peerList = { "Peer1", "Peer2", "Peer3" };

const std::vector<char*> peerType = { "Group", "Zone", "All" };

std::vector<Character> fullTaskTable;

void TH_Cmd(PlayerClient* pChar, char* szLine) {
	char Arg[MAX_STRING] = { 0 };
	GetMaybeQuotedArg(Arg, MAX_STRING, szLine, 1);
	if (strlen(Arg)) {
		if (ci_equals(Arg, "help")) {
			WriteChatf(PLUGIN_MSG "\ar/th show \ag--- Show UI");
			WriteChatf(PLUGIN_MSG "\ar/th hide \ag--- Hide UI");
			WriteChatf(PLUGIN_MSG "\ar/th debug \ag-- Toggle debug mode");
			return;
		}
		if (ci_equals(Arg, "show")) {
			WriteChatf(PLUGIN_MSG "Showing UI.");
			b_showMQTaskHudWindow = true;
			return;
		}
		if (ci_equals(Arg, "hide")) {
			WriteChatf(PLUGIN_MSG "Hiding UI.");
			b_showMQTaskHudWindow = false;
			return;
		}
		if (ci_equals(Arg, "debug")) {
			WriteChatf(PLUGIN_MSG "Toggling debug mode \ar%s\ag.", b_debugMode ? "off" : "on");
			b_debugMode = !b_debugMode;
			return;
		}
		if (ci_equals(Arg, "test")) {
			//just a test arg, move along, nothing to see here.
			request_tasks();
		}
	}
}

std::vector<Task> get_tasks()
{
	std::vector<Task> myTasks;
	for (int i = 0; i <= MAX_QUEST_ENTRIES; ++i)
	{
		const auto& task = pTaskManager->QuestEntries[i];
		if (strlen(task.TaskTitle))
		{ 
			Task tmpTask(task.TaskTitle);
			auto taskStatus = pTaskManager->GetTaskStatus(pLocalPC, i, task.TaskSystem);
			for (int j = 0; j < MAX_TASK_ELEMENTS; ++j)
			{
				char szTemp[MAX_STRING] = { 0 };
				pTaskManager->GetElementDescription(&task.Elements[j], szTemp);
				if (strcmp(szTemp, "? ? ?") == 0 || strcmp(szTemp, "ERROR: String not found. (0)") == 0)
				{
					continue; // Skip this objective and move to the next
				}
				else
				{
					if (taskStatus->CurrentCounts[j] == task.Elements[j].RequiredCount)
					{
						tmpTask.addObjective(Objective(szTemp, true, 0, j));
					}
					else
					{
						tmpTask.addObjective(Objective(szTemp, false, taskStatus->CurrentCounts[j], j));
					}
				}
			}
			myTasks.push_back(tmpTask);
		}
	}
	return myTasks;
}

void send_tasks(std::vector<Task> tasks)
{
	proto::taskhud::TaskHud message;

	message.set_id(proto::taskhud::Incoming);

	proto::taskhud::TaskTable* taskTable = message.mutable_tasktable();

	for (const auto& task : tasks)
	{
		proto::taskhud::Task* protoTask = taskTable->add_tasks();
		protoTask->set_name(task.name);
		for (const auto& objective : task.objectives)
		{
			proto::taskhud::Objective* protoObjective = protoTask->add_objectives();
			protoObjective->set_objectivetext(objective.objectiveText);
			protoObjective->set_iscompleted(objective.isCompleted);
			protoObjective->set_progress(objective.progress);
			protoObjective->set_objindex(objective.index);

		}
	}
	std::string serializedData;
	message.SerializeToString(&serializedData);
	postoffice::Address address;
	address.Name = "taskhud";

	THDropbox.Post(address, message);
}

void request_tasks()
{
	postoffice::Address address;
	address.Mailbox = "taskhud";
	proto::taskhud::TaskHud message;
	message.set_id(proto::taskhud::Request);
	THDropbox.Post(address, message);
	
}

void HandleMessage(const std::shared_ptr<postoffice::Message>& message)
{
	if (message->Payload)
	{
		proto::taskhud::TaskHud taskHudMessage;
		taskHudMessage.ParseFromString(*message->Payload);
		
		switch (taskHudMessage.id())
		{
		case proto::taskhud::Request:
		{
			WriteChatf(PLUGIN_MSG "Received request message");
			fullTaskTable.clear();
			std::vector<Task> my_tasks = get_tasks();
			send_tasks(my_tasks);
			break;
		}
		case proto::taskhud::Incoming:
		{
			WriteChatf(PLUGIN_MSG "Received incoming message");
			Character character;
			for (char* name : peerList) {
				delete[] name;
			}
			if (taskHudMessage.has_tasktable())
			{
				const proto::taskhud::TaskTable& payload = taskHudMessage.tasktable();
				std::string s_tmpCharName = payload.character();
				character.name = s_tmpCharName;
				char* copiedName = new char[s_tmpCharName.size() + 1];
				std::strcpy(copiedName, s_tmpCharName.c_str());

				peerList.push_back(copiedName);
				for (const auto& protoTask : payload.tasks())
				{
					Task tmpTask;
					strncpy_s(tmpTask.name, protoTask.name().c_str(), MAX_STRING); 

					for (const auto& protoObjective : protoTask.objectives())
					{
						Objective tmpObjective;
						strncpy_s(tmpObjective.objectiveText, protoObjective.objectivetext().c_str(), MAX_STRING);
						tmpObjective.isCompleted = protoObjective.iscompleted();
						tmpObjective.progress = protoObjective.progress();
						tmpObjective.index = protoObjective.objindex();
						tmpTask.addObjective(tmpObjective);
					}
					character.addTask(tmpTask);
				}
			}
			fullTaskTable.push_back(character);
			break;
		}
		default:
		{
			break;
		}
		}
	}
}


/**
 * @fn InitializePlugin
 *
 * This is called once on plugin initialization and can be considered the startup
 * routine for the plugin.
 */
PLUGIN_API void InitializePlugin()
{
	DebugSpewAlways("MQTaskHud::Initializing version %f", MQ2Version);
	WriteChatf(PLUGIN_MSG "About to create dropbox");
	THDropbox = postoffice::AddActor("taskhud", HandleMessage);
	WriteChatf(PLUGIN_MSG "created dropbox");
	AddCommand("/th", TH_Cmd, false, true, true);
	// AddXMLFile("MQUI_MyXMLFile.xml");
	// AddMQ2Data("mytlo", MyTLOData);
}

/**
 * @fn ShutdownPlugin
 *
 * This is called once when the plugin has been asked to shutdown.  The plugin has
 * not actually shut down until this completes.
 */
PLUGIN_API void ShutdownPlugin()
{
	DebugSpewAlways("MQTaskHud::Shutting down");
	RemoveCommand("/th");
	THDropbox.Remove();
	// RemoveXMLFile("MQUI_MyXMLFile.xml");
	// RemoveMQ2Data("mytlo");
}

/**
 * @fn SetGameState
 *
 * This is called when the GameState changes.  It is also called once after the
 * plugin is initialized.
 *
 * For a list of known GameState values, see the constants that begin with
 * GAMESTATE_.  The most commonly used of these is GAMESTATE_INGAME.
 *
 * When zoning, this is called once after @ref OnBeginZone @ref OnRemoveSpawn
 * and @ref OnRemoveGroundItem are all done and then called once again after
 * @ref OnEndZone and @ref OnAddSpawn are done but prior to @ref OnAddGroundItem
 * and @ref OnZoned
 *
 * @param GameState int - The value of GameState at the time of the call
 */
PLUGIN_API void SetGameState(int GameState)
{
	//DebugSpewAlways("MQTaskHud::SetGameState(%d)", GameState);
}


/**
 * @fn OnPulse
 *
 * This is called each time MQ2 goes through its heartbeat (pulse) function.
 *
 * Because this happens very frequently, it is recommended to have a timer or
 * counter at the start of this call to limit the amount of times the code in
 * this section is executed.
 */
PLUGIN_API void OnPulse()
{
/*
	static std::chrono::steady_clock::time_point PulseTimer = std::chrono::steady_clock::now();
	// Run only after timer is up
	if (std::chrono::steady_clock::now() > PulseTimer)
	{
		// Wait 5 seconds before running again
		PulseTimer = std::chrono::steady_clock::now() + std::chrono::seconds(5);
		DebugSpewAlways("MQTaskHud::OnPulse()");
	}
*/
}

/**
 * @fn OnWriteChatColor
 *
 * This is called each time WriteChatColor is called (whether by MQ2Main or by any
 * plugin).  This can be considered the "when outputting text from MQ" callback.
 *
 * This ignores filters on display, so if they are needed either implement them in
 * this section or see @ref OnIncomingChat where filters are already handled.
 *
 * If CEverQuest::dsp_chat is not called, and events are required, they'll need to
 * be implemented here as well.  Otherwise, see @ref OnIncomingChat where that is
 * already handled.
 *
 * For a list of Color values, see the constants for USERCOLOR_.  The default is
 * USERCOLOR_DEFAULT.
 *
 * @param Line const char* - The line that was passed to WriteChatColor
 * @param Color int - The type of chat text this is to be sent as
 * @param Filter int - (default 0)
 */
PLUGIN_API void OnWriteChatColor(const char* Line, int Color, int Filter)
{
	// DebugSpewAlways("MQTaskHud::OnWriteChatColor(%s, %d, %d)", Line, Color, Filter);
}

/**
 * @fn OnIncomingChat
 *
 * This is called each time a line of chat is shown.  It occurs after MQ filters
 * and chat events have been handled.  If you need to know when MQ2 has sent chat,
 * consider using @ref OnWriteChatColor instead.
 *
 * For a list of Color values, see the constants for USERCOLOR_. The default is
 * USERCOLOR_DEFAULT.
 *
 * @param Line const char* - The line of text that was shown
 * @param Color int - The type of chat text this was sent as
 *
 * @return bool - Whether to filter this chat from display
 */
PLUGIN_API bool OnIncomingChat(const char* Line, DWORD Color)
{
	// DebugSpewAlways("MQTaskHud::OnIncomingChat(%s, %d)", Line, Color);
	return false;
}

/**
 * @fn OnAddSpawn
 *
 * This is called each time a spawn is added to a zone (ie, something spawns). It is
 * also called for each existing spawn when a plugin first initializes.
 *
 * When zoning, this is called for all spawns in the zone after @ref OnEndZone is
 * called and before @ref OnZoned is called.
 *
 * @param pNewSpawn PSPAWNINFO - The spawn that was added
 */
PLUGIN_API void OnAddSpawn(PSPAWNINFO pNewSpawn)
{
	// DebugSpewAlways("MQTaskHud::OnAddSpawn(%s)", pNewSpawn->Name);
}

/**
 * @fn OnRemoveSpawn
 *
 * This is called each time a spawn is removed from a zone (ie, something despawns
 * or is killed).  It is NOT called when a plugin shuts down.
 *
 * When zoning, this is called for all spawns in the zone after @ref OnBeginZone is
 * called.
 *
 * @param pSpawn PSPAWNINFO - The spawn that was removed
 */
PLUGIN_API void OnRemoveSpawn(PSPAWNINFO pSpawn)
{
	// DebugSpewAlways("MQTaskHud::OnRemoveSpawn(%s)", pSpawn->Name);
}

/**
 * @fn OnAddGroundItem
 *
 * This is called each time a ground item is added to a zone (ie, something spawns).
 * It is also called for each existing ground item when a plugin first initializes.
 *
 * When zoning, this is called for all ground items in the zone after @ref OnEndZone
 * is called and before @ref OnZoned is called.
 *
 * @param pNewGroundItem PGROUNDITEM - The ground item that was added
 */
PLUGIN_API void OnAddGroundItem(PGROUNDITEM pNewGroundItem)
{
	// DebugSpewAlways("MQTaskHud::OnAddGroundItem(%d)", pNewGroundItem->DropID);
}

/**
 * @fn OnRemoveGroundItem
 *
 * This is called each time a ground item is removed from a zone (ie, something
 * despawns or is picked up).  It is NOT called when a plugin shuts down.
 *
 * When zoning, this is called for all ground items in the zone after
 * @ref OnBeginZone is called.
 *
 * @param pGroundItem PGROUNDITEM - The ground item that was removed
 */
PLUGIN_API void OnRemoveGroundItem(PGROUNDITEM pGroundItem)
{
	// DebugSpewAlways("MQTaskHud::OnRemoveGroundItem(%d)", pGroundItem->DropID);
}

/**
 * @fn OnBeginZone
 *
 * This is called just after entering a zone line and as the loading screen appears.
 */
PLUGIN_API void OnBeginZone()
{
	// DebugSpewAlways("MQTaskHud::OnBeginZone()");
}

/**
 * @fn OnEndZone
 *
 * This is called just after the loading screen, but prior to the zone being fully
 * loaded.
 *
 * This should occur before @ref OnAddSpawn and @ref OnAddGroundItem are called. It
 * always occurs before @ref OnZoned is called.
 */
PLUGIN_API void OnEndZone()
{
	// DebugSpewAlways("MQTaskHud::OnEndZone()");
}

/**
 * @fn OnZoned
 *
 * This is called after entering a new zone and the zone is considered "loaded."
 *
 * It occurs after @ref OnEndZone @ref OnAddSpawn and @ref OnAddGroundItem have
 * been called.
 */
PLUGIN_API void OnZoned()
{
	// DebugSpewAlways("MQTaskHud::OnZoned()");
}


void ShowTaskDetails()
{

}

/**
 * @fn OnUpdateImGui
 *
 * This is called each time that the ImGui Overlay is rendered. Use this to render
 * and update plugin specific widgets.
 *
 * Because this happens extremely frequently, it is recommended to move any actual
 * work to a separate call and use this only for updating the display.
 */
PLUGIN_API void OnUpdateImGui()
{

	if (GetGameState() == GAMESTATE_INGAME)
	{
		if (b_showMQTaskHudWindow)
		{
			ImGui::SetNextWindowSize(ImVec2(FIRST_WINDOW_WIDTH, FIRST_WINDOW_HEIGHT), ImGuiCond_FirstUseEver);
			if (ImGui::Begin("Task HUD", &b_showMQTaskHudWindow, ImGuiWindowFlags_None)) 
			{
				ImGui::SetNextItemWidth(100);
				ImGui::Combo("##Character Select", &i_selectedCharIndex, peerList.data(), peerList.size());
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("Selected character");
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(100);
				ImGui::Combo("##Peer type", &i_selectedPeerIndex, peerType.data(), peerType.size());
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("Peer set");
				}
				ImGui::SameLine();
				if (ImGui::SmallButton(ICON_MD_REFRESH)) {
					//triggers.do_refresh = true;
				}
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("Refresh tasks");
				}
				// Display tasks and objectives
				ShowTaskDetails();
			}
			ImGui::End();
		}
	}

}

/**
 * @fn OnMacroStart
 *
 * This is called each time a macro starts (ex: /mac somemacro.mac), prior to
 * launching the macro.
 *
 * @param Name const char* - The name of the macro that was launched
 */
PLUGIN_API void OnMacroStart(const char* Name)
{
	// DebugSpewAlways("MQTaskHud::OnMacroStart(%s)", Name);
}

/**
 * @fn OnMacroStop
 *
 * This is called each time a macro stops (ex: /endmac), after the macro has ended.
 *
 * @param Name const char* - The name of the macro that was stopped.
 */
PLUGIN_API void OnMacroStop(const char* Name)
{
	// DebugSpewAlways("MQTaskHud::OnMacroStop(%s)", Name);
}

/**
 * @fn OnLoadPlugin
 *
 * This is called each time a plugin is loaded (ex: /plugin someplugin), after the
 * plugin has been loaded and any associated -AutoExec.cfg file has been launched.
 * This means it will be executed after the plugin's @ref InitializePlugin callback.
 *
 * This is also called when THIS plugin is loaded, but initialization tasks should
 * still be done in @ref InitializePlugin.
 *
 * @param Name const char* - The name of the plugin that was loaded
 */
PLUGIN_API void OnLoadPlugin(const char* Name)
{
	// DebugSpewAlways("MQTaskHud::OnLoadPlugin(%s)", Name);
}

/**
 * @fn OnUnloadPlugin
 *
 * This is called each time a plugin is unloaded (ex: /plugin someplugin unload),
 * just prior to the plugin unloading.  This means it will be executed prior to that
 * plugin's @ref ShutdownPlugin callback.
 *
 * This is also called when THIS plugin is unloaded, but shutdown tasks should still
 * be done in @ref ShutdownPlugin.
 *
 * @param Name const char* - The name of the plugin that is to be unloaded
 */
PLUGIN_API void OnUnloadPlugin(const char* Name)
{
	// DebugSpewAlways("MQTaskHud::OnUnloadPlugin(%s)", Name);
}