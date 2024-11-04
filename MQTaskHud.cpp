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
bool b_anonMode = false;
int i_selectedCharIndex = 0;
int i_selectedPeerIndex = 0;
int i_selectedTaskIndex = 0;

std::vector<std::string> peerList = { };

const std::vector<std::string> peerType = { "Group", "Zone", "All" };

std::vector<Character> fullTaskTable;

std::string AnonymizeName(const std::string& name)
{
	if (name.empty()) return name;
	return std::string(1, name.front()) + "***" + std::string(1, name.back());
}

void PrintTaskTable()
{
	if (fullTaskTable.empty())
	{
		WriteChatf(PLUGIN_MSG "Table empty");
	}
	for (const auto& character : fullTaskTable) {
		std::string displayedName = b_anonMode ? AnonymizeName(character.name) : character.name;
		WriteChatf("Character: \ar%s\aw", displayedName.c_str());

		for (const auto& task : character.tasks) {
			WriteChatf("  Task: \ay%s\aw", task.name.c_str());

			for (const auto& objective : task.objectives) {
				WriteChatf("    Objective: \ag%s\aw - Complete: \ag%s\aw - Progress: \ag%d\aw - Required: \ag%d\aw Index: \ag%d\aw",
					objective.objectiveText.c_str(),
					objective.isCompleted ? "true" : "false",
					objective.progress,
					objective.required,
					objective.index
				);
				if (!objective.objectiveDifferences.empty())
				{
					WriteChatf("      Objective Differences:");
					for (const auto& diffPair : objective.objectiveDifferences)
					{
						const std::vector<ObjectiveDifference>& differences = diffPair.second;

						for (const auto& diff : differences)
						{
							std::string displayedName = b_anonMode ? AnonymizeName(diff.characterName) : diff.characterName;
							WriteChatf("        Character: \ar%s\aw - Is Ahead: \ag%s\aw",
								displayedName.c_str(),
								diff.isAhead ? "true" : "false"
							);
						}
					}
				}
			}

			if (!task.missingList.empty())
			{
				WriteChatf("    Missing List:");
				for (const auto& missing : task.missingList) {
					std::string displayedMissingName = b_anonMode ? AnonymizeName(missing) : missing;
					WriteChatf("      Character: \ar%s\aw", displayedMissingName.c_str());
				}
			}
		}
	}
}

void RequestTasks()
{
	WriteChatf(PLUGIN_MSG "Request task function");
	postoffice::Address address;
	address.Mailbox = "taskhud";
	proto::taskhud::TaskHud message;
	proto::taskhud::RequestMessage reqMsg;
	reqMsg.set_reqchar(pLocalPlayer->DisplayedName);
	message.set_id(proto::taskhud::Request);
	message.set_payload(reqMsg.SerializeAsString());
	THDropbox.Post(address, message);
}

void TH_Cmd(PlayerClient* pChar, char* szLine) {
	char Arg[MAX_STRING] = { 0 };
	GetMaybeQuotedArg(Arg, MAX_STRING, szLine, 1);
	if (strlen(Arg)) {
		if (ci_equals(Arg, "help")) {
			WriteChatf(PLUGIN_MSG "\ar/th show    \ag--- Show UI");
			WriteChatf(PLUGIN_MSG "\ar/th hide    \ag--- Hide UI");
			WriteChatf(PLUGIN_MSG "\ar/th anon    \ag--- Anonymize character names");
			WriteChatf(PLUGIN_MSG "\ar/th refresh \ag--- Refresh data");
			return;
		}
		if (ci_equals(Arg, "show")) {
			WriteChatf(PLUGIN_MSG "Showing UI.");
			if (fullTaskTable.empty())
			{
				RequestTasks();
			}
			b_showMQTaskHudWindow = true;
			return;
		}
		if (ci_equals(Arg, "hide")) {
			WriteChatf(PLUGIN_MSG "Hiding UI.");
			b_showMQTaskHudWindow = false;
			return;
		}
		if (ci_equals(Arg, "anon"))
		{
			WriteChatf(PLUGIN_MSG "Toggling anonymous mode \ar%s\ag.", b_anonMode ? "off" : "on");
			b_anonMode = !b_anonMode;
			return;
		}
		if (ci_equals(Arg, "refresh"))
		{
			RequestTasks();
		}
		if (ci_equals(Arg, "dumptable"))
		{
			PrintTaskTable();
		}
	}
}

int FindCharacterIndexByName(const std::vector<Character>& characters, const std::string& name)
{
	auto it = std::find_if(characters.begin(), characters.end(), [&name](const Character& character) { return character.name == name; });
	if (it != characters.end()) {
		return std::distance(characters.begin(), it);
	}
	else
	{
		return -1;
	}
}

std::vector<Task> GetTasks()
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
				if (strcmp(szTemp, "? ? ?") == 0)
				{
					tmpTask.addObjective(Objective(szTemp, false, -1, -1, j));
				}
				else if (strcmp(szTemp, "ERROR: String not found. (0)") == 0)
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

void SendTasks(const std::vector<Task>& tasks)
{
	if (GetGameState() != GAMESTATE_INGAME)
	{
		return;
	}
	
	proto::taskhud::TaskTable message;
	WriteChatf(PLUGIN_MSG "Sending tasks now");
	message.set_character(pLocalPlayer->DisplayedName);

	for (const auto& task : tasks)
	{
		proto::taskhud::Task* protoTask = message.add_tasks();
		protoTask->set_name(task.name);
		for (const auto& objective : task.objectives)
		{
			proto::taskhud::Objective* protoObjective = protoTask->add_objectives();
			protoObjective->set_objectivetext(objective.objectiveText);
			protoObjective->set_iscompleted(objective.isCompleted);
			protoObjective->set_progress(objective.progress);
			protoObjective->set_required(objective.required);
			protoObjective->set_index(objective.index);

		}
	}

	postoffice::Address address;
	address.Mailbox = "taskhud";
	proto::taskhud::TaskHud payload;
	payload.set_id(proto::taskhud::Incoming);
	payload.set_payload(message.SerializeAsString());
	THDropbox.Post(address, payload);
}



void CompareTasks()
{
	for (auto& currentCharacter : fullTaskTable)
	{
		for (auto& currentTask : currentCharacter.tasks)
		{
			currentTask.clearMissingList();

			for (const auto& otherCharacter : fullTaskTable)
			{
				if (currentCharacter.name == otherCharacter.name)
				{
					continue;
				}

				bool taskFound = false;
				for (const auto& otherTask : otherCharacter.tasks)
				{
					if (otherTask.name == currentTask.name)
					{
						taskFound = true;
						for (auto& currentObjective : currentTask.objectives)
						{
							bool objectiveFound = false;
							for (const auto& otherObjective : otherTask.objectives)
							{
								if (currentObjective.index == otherObjective.index)
								{
									objectiveFound = true;
									currentObjective.compareWith(otherObjective, otherCharacter.name);
									break;
								}
							}
						}
						break;
					}
				}
				if (!taskFound)
				{
					currentTask.missingList.push_back(otherCharacter.name);
				}
			}
		}
	}
}

void ProcessIncomingMessage(proto::taskhud::TaskTable& taskTable)
{
	if (std::find(peerList.begin(), peerList.end(), taskTable.character()) != peerList.end())
	{
		return;
	}

	Character newCharacter(taskTable.character());
	peerList.push_back(newCharacter.name);
	
	for (const auto& task : taskTable.tasks())
	{
		Task newTask(task.name());
		for (const auto& objective : task.objectives())
		{
			Objective newObjective(
				objective.objectivetext(),
				objective.iscompleted(),
				objective.progress(),
				objective.required(),
				objective.index()
			);
			newTask.addObjective(newObjective);
		}
		newCharacter.addTask(newTask);
	}
	fullTaskTable.push_back(newCharacter);
	std::sort(fullTaskTable.begin(), fullTaskTable.end(), [](const Character& a, const Character& b) {
		return a.name < b.name;
		});
	CompareTasks();
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
			fullTaskTable.clear();
			peerList.clear();
			std::vector<Task> my_tasks = GetTasks();
			SendTasks(my_tasks);
			break;
		}
		case proto::taskhud::Incoming:
		{
			proto::taskhud::TaskTable taskTable;
			taskTable.ParseFromString(taskHudMessage.payload());
			ProcessIncomingMessage(taskTable);
			break;
		}
		default:
		{
			break;
		}
		}
	}
}

void DrawTaskCombo(int i_chrIdx)
{
	Character character = fullTaskTable[i_chrIdx];
	if (i_selectedTaskIndex + 1 > fullTaskTable[i_chrIdx].tasks.size())
	{
		i_selectedTaskIndex = 0;
	}
	if (ImGui::BeginCombo("##Task", character.tasks[i_selectedTaskIndex].name.c_str()))
	{
		for (int i = 0; i < character.tasks.size(); ++i)
		{
			bool isSelected = (i_selectedTaskIndex == i);
			if (ImGui::Selectable(character.tasks[i].name.c_str(), isSelected))
			{
				i_selectedTaskIndex = i;
			}
			if (isSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
}

std::vector<std::string> FindCharacterMissingTask(int i_chrIdx, const std::string& selectedTaskName)
{
	std::vector<std::string> charactersWithoutTask;

	for (size_t i = 0; i < fullTaskTable.size(); ++i)
	{
		if (i == i_chrIdx || fullTaskTable.empty())
		{
			continue;
		}
		const auto& character = fullTaskTable[i];
		bool b_taskFound = false;

		for (const auto& task : character.tasks)
		{
			if (task.name == selectedTaskName) {
				b_taskFound = true;
				break;
			}
		}

		if (!b_taskFound)
		{
			charactersWithoutTask.push_back(character.name);
		}
	}
	return charactersWithoutTask;
}

void DrawMissingCharacters(int i_chrIdx)
{
	const std::string selectedTaskName = fullTaskTable[i_chrIdx].tasks[i_selectedTaskIndex].name;
	std::vector<std::string> charactersWithoutTask = FindCharacterMissingTask(i_chrIdx, selectedTaskName);
	if (!charactersWithoutTask.empty())
	{
		ImGui::SeparatorText("Missing this task");
		for (size_t i = 0; i < charactersWithoutTask.size(); ++i)
		{
			std::string displayName = b_anonMode ? AnonymizeName(charactersWithoutTask[i]) : charactersWithoutTask[i];
			ImGui::Text("%s", displayName.c_str());
			if (i < charactersWithoutTask.size() - 1)
			{
				ImGui::SameLine();
				ImGui::Text(ICON_MD_REMOVE);
				ImGui::SameLine();
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
	THDropbox = postoffice::AddActor("taskhud", HandleMessage);
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

void DrawComboBoxes()
{
	ImGui::SetNextItemWidth(100);
	std::string displayedName = b_anonMode ? AnonymizeName(fullTaskTable[i_selectedCharIndex].name) : fullTaskTable[i_selectedCharIndex].name;
	if (ImGui::BeginCombo("CharacterSelect", displayedName.c_str()))
	{
		for (size_t i = 0; i < fullTaskTable.size(); ++i)
		{
			const bool is_selected = (i == i_selectedCharIndex);
			std::string displayedName = b_anonMode ? AnonymizeName(fullTaskTable[i].name.c_str()) : fullTaskTable[i].name.c_str();
			if (ImGui::Selectable(displayedName.c_str(), is_selected))
			{
				i_selectedCharIndex = i;
			}
			if (is_selected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	ImGui::SameLine();
	ImGui::SetNextItemWidth(100);
	if (ImGui::BeginCombo("##Peer Group Select", peerType[i_selectedPeerIndex].c_str()))
	{
		for (size_t i = 0; i < peerType.size(); ++i)
		{
			bool isGroupSelected = (i_selectedPeerIndex == i);
			if (ImGui::Selectable(peerType[i].c_str(), isGroupSelected))
			{
				i_selectedPeerIndex = i;
			}
			if (isGroupSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Peer group selection");
		}
	}
	
	ImGui::SameLine();
	if (ImGui::SmallButton(ICON_MD_REFRESH))
	{
		RequestTasks();
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Refresh tasks");
	}

	if (!fullTaskTable.empty() && i_selectedCharIndex < fullTaskTable.size())
	{
		const Character& selectedCharacter = fullTaskTable[i_selectedCharIndex];
		if (i_selectedTaskIndex > selectedCharacter.tasks.size())
		{
			i_selectedTaskIndex = 1;
		}
		if (ImGui::BeginCombo("##Task Select", selectedCharacter.tasks[i_selectedTaskIndex].name.c_str()))
		{
			for (size_t i = 0; i < selectedCharacter.tasks.size(); ++i)
			{
				bool isTaskSelected = (i_selectedTaskIndex == i);
				if (ImGui::Selectable(selectedCharacter.tasks[i].name.c_str(), isTaskSelected))
				{
					i_selectedTaskIndex = i;
				}
			}
			ImGui::EndCombo();
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Task selection");
			}
		}
	}

}

void DrawCharactersMissingTask(const Task& selectedTask)
{
	if (!selectedTask.missingList.empty())
	{
		ImGui::SeparatorText("Characters missing this task");
		for (size_t i = 0; i < selectedTask.missingList.size(); ++i)
		{
			std::string displayedName = b_anonMode ? AnonymizeName(selectedTask.missingList[i].c_str()) : selectedTask.missingList[i].c_str();
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", displayedName.c_str());
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Bring %s to foreground", displayedName.c_str());
				if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
				{
					char command[256];
					snprintf(command, sizeof(command), "/dex %s /foreground", selectedTask.missingList[i].c_str());
					DoCommand(command);
				}
			}
			if (i < selectedTask.missingList.size() - 1)
			{
				ImGui::SameLine();
				ImGui::Text(ICON_MD_REMOVE);
				ImGui::SameLine();
			}
		}
		
	}
}

void DrawCharsMissingObj(const Objective& objective)
{
	if (!objective.objectiveDifferences.empty())
	{
		for (const auto& diffPair : objective.objectiveDifferences)
		{
			const std::vector<ObjectiveDifference>& diffs = diffPair.second;

			for (size_t i = 0; i < diffs.size(); ++i) {
				const auto& diff = diffs[i];
				std::string displayedName = b_anonMode ? AnonymizeName(diff.characterName) : diff.characterName;

				ImVec4 color = diff.isAhead ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f);

				ImGui::TextColored(color, "%s", displayedName.c_str());
				
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("Bring %s to foreground", displayedName.c_str());
					if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
					{
						char command[256];
						snprintf(command, sizeof(command), "/dex %s /foreground", diff.characterName.c_str());
						DoCommand(command);
					}
				}

				if (i != diffs.size() - 1) {
					ImGui::SameLine();
					ImGui::Text(ICON_MD_REMOVE);
					ImGui::SameLine();
				}
			}
		}
	}
}

void DrawTaskDetails(const Task& selectedTask)
{
	ImGui::Separator();
	if (ImGui::BeginTable("##Objective Table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
	{
		for (const Objective& objective : selectedTask.objectives)
		{
			ImGui::TableNextColumn();
			ImGui::Text(objective.objectiveText.c_str());
			ImGui::TableNextColumn();
			if (objective.isCompleted)
			{
				ImGui::TextColored(ImVec4(0, 255, 0, 255), "Done");
			}
			else
			{
				if (objective.progress != -1 && objective.required != -1)
				{
					ImGui::Text("%d/%d", objective.progress, objective.required);
				}
			}
			ImGui::TableNextColumn();
			DrawCharsMissingObj(objective);
			ImGui::TableNextRow();
		}
		ImGui::EndTable();
	}
}

void DrawTaskHud(const Task& selectedTask)
{
	ImGui::SetNextWindowSize(ImVec2(FIRST_WINDOW_WIDTH, FIRST_WINDOW_HEIGHT), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Task HUD", &b_showMQTaskHudWindow, ImGuiWindowFlags_None))
	{
		DrawComboBoxes();
		DrawCharactersMissingTask(selectedTask);
		DrawTaskDetails(selectedTask);
		ImGui::EndTabItem();
	}

	ImGui::End();
}

PLUGIN_API void OnUpdateImGui()
{
	if (GetGameState() == GAMESTATE_INGAME && b_showMQTaskHudWindow)
	{
		if (!fullTaskTable.empty())
		{
			if (i_selectedCharIndex >= fullTaskTable.size())
			{
				ImGui::SetNextWindowSize(ImVec2(FIRST_WINDOW_WIDTH, FIRST_WINDOW_HEIGHT), ImGuiCond_FirstUseEver);
				if (ImGui::Begin("Task HUD", &b_showMQTaskHudWindow, ImGuiWindowFlags_None))
				{
					ImGui::Text("Updating data.");
					ImGui::End();
					return;
				}

				ImGui::End();
			}
			Character& selectedCharacter = fullTaskTable[i_selectedCharIndex];
			if (i_selectedTaskIndex < selectedCharacter.tasks.size())
			{
				Task& selectedTask = selectedCharacter.tasks[i_selectedTaskIndex];
				DrawTaskHud(selectedTask);
			}
		}
	}	
}

PLUGIN_API bool OnIncomingChat(const char* Line, DWORD Color)
{
	if (strstr(Line, "Your task") && strstr(Line, "has been updated"))
	{
		RequestTasks();
	}
	else if (strstr(Line, "You have been assigned the task"))
	{
		RequestTasks();
	}

	// DebugSpewAlways("MQtest::OnIncomingChat(%s, %d)", Line, Color);
	return false;
}

