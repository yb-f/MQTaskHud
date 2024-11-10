// MQTaskHud.cpp : Defines the entry point for the DLL application.
//
//

#include <mq/Plugin.h>
#include "mq/api/ActorAPI.h"
#include "MQTaskHud.h"
#include "MessageHandler.h"

PreSetup("MQTaskHud");
PLUGIN_VERSION(0.1);

TaskTable taskTable;
postoffice::DropboxAPI THDropbox;

void dumpObjectives(const Task& task)
{
	for (const auto& objective : task.getObjectives())
	{
		const auto& values = objective.getValues();
		WriteChatf(PLUGIN_MSG "      Objective: \ag%s", values.objectiveText.c_str());
		WriteChatf(PLUGIN_MSG "        Complete: \ag%s", values.isCompleted ? "true" : "false");
		WriteChatf(PLUGIN_MSG "        Progress: \ag%d\aw/\ag%d", values.progress, values.required);
		
		if (objective.hasObjectiveDifferences())
		{
			WriteChatf(PLUGIN_MSG "        Differences:");
			for (const auto& difference : objective.getAllDifferences())
			{
				WriteChatf(PLUGIN_MSG "          \ar%s\aw Equal: \ag%s \ar%s",
					difference.getCharacterName(taskTable.anonMode).c_str(),
					difference.isEqualProgress() ? "Yes" : "No",
					difference.isObjAhead() ? "[Ahead]" : "[Behind]"
				);
			}
		}
	}
}

void dumpMissingList(const std::vector<std::string>& missingList) {
	if (!missingList.empty())
	{
		std::string buffer = "      Missing List: \ag";
		for (const std::string& missingName : missingList)
		{
			std::string displayedName;
			if (taskTable.anonMode)
			{
				displayedName = std::string(1, missingName.front()) + "***" + std::string(1, missingName.back()) + " ";
			}
			else
			{
				displayedName = missingName + " ";
			}
			buffer += displayedName;
		}
		WriteChatf(PLUGIN_MSG "%s", buffer.c_str());
	}
}

void dumpTaskTable()
{
	if (taskTable.isEmpty())
	{
		WriteChatf(PLUGIN_MSG "Table empty");
		return;
	}

	for (auto& character : taskTable.getCharacters())
	{
		WriteChatf(PLUGIN_MSG "Character: \ar%s\aw(\ag%d\aw)", character.getCharName(taskTable.anonMode).c_str(), character.getId());
		WriteChatf(PLUGIN_MSG "  Group Leader: \ag%s\aw", character.getGroupLeaderName(taskTable.anonMode).c_str());

		for (auto& task : character.getTasks())
		{
			WriteChatf(PLUGIN_MSG "    Task: \ay%s\aw(\ag%llu\aw)", task.getTaskName().c_str(), task.getUid());

			dumpObjectives(task);

			dumpMissingList(task.getMissingList());
		}
	}

}

void dumpPeers()
{
	if (taskTable.getPeerCount() == 0)
	{
		WriteChatf(PLUGIN_MSG "Peer list empty");
		return;
	}
	WriteChatf(PLUGIN_MSG "Peer List: ");
	for (const auto& peer : taskTable.getPeers())
	{
		WriteChatf(PLUGIN_MSG "  Peer: \ag%s \aw(\ar%d\aw)", peer.getName(taskTable.anonMode).c_str(), peer.getId());
		WriteChatf(PLUGIN_MSG "    Heartbeats missed: \ay%d", peer.getMissedHeartbeats());
	}
}

void requestPeers()
{
	MessageHandler::sendHeartbeatMessages(HeartbeatType::Register);
	if (taskTable.isRegistered == false)
	{
		taskTable.isRegistered = true;
	}
	
}

void requestTasks()
{
	postoffice::Address address;
	proto::TaskHud::TaskHud message;
	proto::TaskHud::RequestMessage reqMsg;
	reqMsg.set_reqchar(pLocalPlayer->DisplayedName);
	message.set_id(proto::TaskHud::Request);
	message.set_payload(reqMsg.SerializeAsString());
	THDropbox.Post(address, message);
}

void thCmd(PlayerClient* pChar, const char* szLine) {
	char arg[MAX_STRING] = {};
	GetMaybeQuotedArg(arg, MAX_STRING, szLine, 1);
	if (strlen(arg)) {
		if (ci_equals(arg, "help")) {
			WriteChatf(PLUGIN_MSG "\ar/th show    \ag--- Show UI");
			WriteChatf(PLUGIN_MSG "\ar/th hide    \ag--- Hide UI");
			WriteChatf(PLUGIN_MSG "\ar/th anon    \ag--- Anonymize character names");
			WriteChatf(PLUGIN_MSG "\ar/th refresh \ag--- Refresh data");
			return;
		}
		if (ci_equals(arg, "show")) {
			WriteChatf(PLUGIN_MSG "Showing UI.");
			if (taskTable.isEmpty())
			{
				requestTasks();
			}
			taskTable.showTaskHudWindow = true;
			return;
		}
		if (ci_equals(arg, "hide")) {
			WriteChatf(PLUGIN_MSG "Hiding UI.");
			taskTable.showTaskHudWindow = false;
			return;
		}
		if (ci_equals(arg, "anon"))
		{
			WriteChatf(PLUGIN_MSG "Toggling anonymous mode \ar%s\ag.", taskTable.anonMode ? "off" : "on");
			taskTable.anonMode = !taskTable.anonMode;
			return;
		}
		if (ci_equals(arg, "refresh"))
		{
			requestTasks();
		}
		if (ci_equals(arg, "refreshpeers"))
		{
			requestPeers();
		}
		if (ci_equals(arg, "dumptable"))
		{
			dumpTaskTable();
		}
		if (ci_equals(arg, "dumppeers"))
		{
			dumpPeers();
		}
		if (ci_equals(arg, "clearpeers"))
		{
			taskTable.clearPeerList();
		}
		if (ci_equals(arg, "cleartable"))
		{
			taskTable.clearAllCharacters();
		}
		if (ci_equals(arg, "clearall"))
		{
			taskTable.clearAllCharacters();
			taskTable.clearPeerList();
		}
	}
}

std::vector<Task> getTasks()
{
	//this is a vector of our Task class.
	std::vector<Task> myTasks;
	for (int i = 0; i < MAX_QUEST_ENTRIES; ++i)
	{
		//this is not of the Task class, this is the eqlib::CTaskEntry
		const auto& task = pTaskManager->QuestEntries[i];
		if (strlen(task.TaskTitle))
		{
			//this is a Task class object
			Task tmpTask(task.TaskTitle);
			auto taskStatus = pTaskManager->GetTaskStatus(pLocalPC, i, task.TaskSystem);
			for (int j = 0; j < MAX_TASK_ELEMENTS; ++j)
			{
				char szTemp[MAX_STRING] = {};
				pTaskManager->GetElementDescription(&task.Elements[j], szTemp);
				if (strcmp(szTemp, "? ? ?") == 0)
				{
					tmpTask.addObjective(Objective(szTemp, false, -1, -1, j));
				}
				else if ((strcmp(szTemp, "ERROR: String not found. (0)") == 0) || (strcmp(szTemp, "ERROR: String not found. (2146435071)") == 0))
				{
					continue;
				}
				else
				{
					if (taskStatus->CurrentCounts[j] >= task.Elements[j].RequiredCount)
					{
						tmpTask.addObjective(Objective(szTemp, true, 0, 0, j));
					}
					else
					{
						tmpTask.addObjective(Objective(szTemp, false, taskStatus->CurrentCounts[j], task.Elements[j].RequiredCount, j));
					}
				}
			}
			myTasks.push_back(tmpTask);
		}
	}
	return myTasks;
}

void handleMessage(const std::shared_ptr<postoffice::Message>& message)
{
	if (message->Payload)
	{
		proto::TaskHud::TaskHud taskHudMessage;
		taskHudMessage.ParseFromString(*message->Payload);

		switch (taskHudMessage.id())
		{
		case proto::TaskHud::Request:
		{
			MessageHandler::processRequestMessage();
			break;
		}
		case proto::TaskHud::Incoming:
		{
			proto::TaskHud::TaskTable protoTaskTable;
			protoTaskTable.ParseFromString(taskHudMessage.payload());
			MessageHandler::processIncomingMessage(protoTaskTable);
			break;
		}
		case proto::TaskHud::Register:
		{
			proto::TaskHud::HeartbeatMessages heartbeat;
			heartbeat.ParseFromString(taskHudMessage.payload());
			MessageHandler::processRegisterMessage(heartbeat);
			break;
		}
		case proto::TaskHud::RegisterResponse:
		{
			proto::TaskHud::HeartbeatMessages heartbeat;
			heartbeat.ParseFromString(taskHudMessage.payload());
			MessageHandler::processRegisterResponseMessage(heartbeat);
			break;
		}
		case proto::TaskHud::PauseHeartbeat:
		{
			proto::TaskHud::HeartbeatMessages heartbeat;
			heartbeat.ParseFromString(taskHudMessage.payload());
			MessageHandler::processPauseHeartbeatMessage(heartbeat);
			break;
		}
		case proto::TaskHud::ResumeHeartbeat:
		{
			proto::TaskHud::HeartbeatMessages heartbeat;
			heartbeat.ParseFromString(taskHudMessage.payload());
			MessageHandler::processResumeHeartbeatMessage(heartbeat);
			break;
		}
		case proto::TaskHud::Heartbeat:
		{
			proto::TaskHud::HeartbeatMessages heartbeat;
			heartbeat.ParseFromString(taskHudMessage.payload());
			MessageHandler::processHeartbeatMessage(heartbeat);
			break;
		}
		default:
		{
			break;
		}
		}
	}
}

std::pair<bool, bool> compareObjectives(const Objective& objA, const Objective& objB)
{
	bool equal = false;
	bool isAheadA = false;
	auto valA = objA.getValues();
	auto valB = objB.getValues();
	if (valA.isCompleted && valB.isCompleted) {
		equal = true;
	}
	else if (valA.isCompleted && !valB.isCompleted) {
		isAheadA = true;
	}
	else if (!valA.isCompleted && valB.isCompleted) {
		equal = false;
	}
	else {
		equal = valA.progress == valB.progress;
		isAheadA = valA.progress > valB.progress;
	}
	return { equal, isAheadA };
}

void compareCharacterTasks(Character& characterA, Character& characterB)
{
	auto& tasksA = characterA.getTasks();
	auto& tasksB = characterB.getTasks();
	for (auto& taskA : tasksA)
	{
		bool hasTask = false;
		for (auto& taskB : tasksB)
		{
			if (taskA.getUid() == taskB.getUid())
			{
				hasTask = true;

				//Compare objectives in the task
				auto& objectivesA = taskA.getObjectives();
				auto& objectivesB = taskB.getObjectives();

				for (auto& objA : objectivesA)
				{
					std::vector<ObjectiveDifference> objDiffs;
					for (auto& objB :objectivesB)
					{
						if (objA.getIndex() == objB.getIndex())
						{
							auto [equal, isAheadA] = compareObjectives(objA, objB);
							if (equal) {
								// tasks are equal, set equal to true for both characters
								objA.addObjectiveDifference(ObjectiveDifference(objA.getIndex(), true, false, characterB.getCharName()));
							}
							else if (isAheadA) {
								// A is ahead, so set isAhead to true in A, false in B
								objA.addObjectiveDifference(ObjectiveDifference(objA.getIndex(), false, true, characterB.getCharName()));
							}
							else {
								// B is ahead, so false in A, true in B
								objA.addObjectiveDifference(ObjectiveDifference(objA.getIndex(), false, false, characterB.getCharName()));
							}
						}
					}
				}
			}
		}
		if (!hasTask)
		{
			taskA.addToMissingList(characterB.getCharName());
		}
	}
}

void compareTasks()
{
	auto& characters = taskTable.getCharacters();

	for (auto& characterA : characters)
	{
		for (auto& characterB : characters)
		{
			if (&characterA != &characterB)
			{
				compareCharacterTasks(characterA, characterB);
			}
		}
	}

}

void doHeartbeat()
{
	MessageHandler::sendHeartbeatMessages(HeartbeatType::Heartbeat);

	for (auto& peer : taskTable.getPeers())
	{
		peer.incrementHeartbeats();
		if (peer.getMissedHeartbeats() >= 6)
		{
			taskTable.removePeerById(peer.getId());
		}
	}
}

PLUGIN_API void InitializePlugin()
{
	DebugSpewAlways("MQTaskHud::Initializing version %f", MQ2Version);
	AddCommand("/th", thCmd, false, true, true);
	THDropbox = postoffice::AddActor(handleMessage);
	if (GetGameState() == GAMESTATE_INGAME)
	{
		MessageHandler::sendHeartbeatMessages(HeartbeatType::Register);
	}
	// AddXMLFile("MQUI_MyXMLFile.xml");
	// AddMQ2Data("mytlo", MyTLOData);
}

PLUGIN_API void ShutdownPlugin()
{
	DebugSpewAlways("MQTaskHud::Shutting down");
	RemoveCommand("/th");
	THDropbox.Remove();
	// RemoveXMLFile("MQUI_MyXMLFile.xml");
	// RemoveMQ2Data("mytlo");
}

PLUGIN_API void OnUpdateImGui()
{
	if (GetGameState() == GAMESTATE_INGAME && taskTable.showTaskHudWindow)
	{
		if (!taskTable.isEmpty())
		{
			if (taskTable.selectedCharIndex >= taskTable.getPeerCount() || taskTable.getPeerCount() == 0)
			{
				drawTaskHudLoading();
				return;
			}
			
			int selectedCharID = taskTable.getPeerAtIndex(taskTable.selectedCharIndex).getId();
			if (auto selectedCharOpt = taskTable.getCharacterById(selectedCharID))
			{
				Character selectedCharacter = *selectedCharOpt;
				if (selectedCharacter.getTasksCount() == 0) {
					//character found but no tasks, draw loading
					drawTaskHudLoading();
					return;
				}

				if (taskTable.selectedTaskIndex >= selectedCharacter.getTasksCount())
				{
					taskTable.selectedTaskIndex = selectedCharacter.getTasksCount() - 1;
				}
				auto selectedTaskOpt = selectedCharacter.getTaskByIndex(taskTable.selectedTaskIndex);
				if (!selectedTaskOpt) {
					drawTaskHudLoading();
					return;
				}
				const Task selectedTask = *selectedTaskOpt;
				drawTaskHud(selectedTask);
			}
			else 
			{
				//character not found draw loading
				drawTaskHudLoading();
				return;
			}
		}
	}
}	

PLUGIN_API bool OnIncomingChat(const char* Line, DWORD Color)
{
	if (strstr(Line, "Your task") && strstr(Line, "has been updated"))
	{
		requestTasks();
	}
	else if (strstr(Line, "You have been assigned the task"))
	{
		requestTasks();
	}

	// DebugSpewAlways("MQtest::OnIncomingChat(%s, %d)", Line, Color);
	return false;
}

PLUGIN_API void OnPulse()
{
	static std::chrono::steady_clock::time_point PulseTimer = std::chrono::steady_clock::now() + std::chrono::seconds(1);
		// Run only after timer is up
		if (std::chrono::steady_clock::now() >= PulseTimer)
		{
			PulseTimer += std::chrono::seconds(1);
			//DebugSpewAlways("MQtest::OnPulse()");
			doHeartbeat();
		}
}

PLUGIN_API void OnBeginZone()
{
	MessageHandler::sendHeartbeatMessages(HeartbeatType::PauseHeartbeat);
}

PLUGIN_API void OnZoned()
{
	if (!taskTable.isRegistered && GetGameState() == GAMESTATE_INGAME)
	{
		MessageHandler::sendHeartbeatMessages(HeartbeatType::Register);
		taskTable.isRegistered = true;
		return;
	}
	MessageHandler::sendHeartbeatMessages(HeartbeatType::ResumeHeartbeat);
}