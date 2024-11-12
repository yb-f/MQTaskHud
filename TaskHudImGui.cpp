#include "TaskHudImGui.h"

#include <mq/Plugin.h>
#include "imgui/fonts/IconsMaterialDesign.h"
#include "MQTaskHud.h"
#include "TaskHudImGui.h"
#include "MessageHandler.h"

void TaskHudImGui::drawComboBoxes(const std::string& myGroupLeader, const std::string& myZone, const bool& amIGrouped)
{
	ImGui::SetNextItemWidth(100);
	if (ImGui::BeginCombo("##Character Select", taskTable.getPeerAtIndex(taskTable.selectedCharIndex).getName(taskTable.anonMode).data()))
	{
		for (int i = 0; i < taskTable.getPeerCount(); ++i)
		{
			const auto& tmpPeer = taskTable.getPeerAtIndex(i);
			bool is_selected = (taskTable.selectedCharIndex == i);
			if (ImGui::Selectable(tmpPeer.getName(taskTable.anonMode).data(), is_selected))
			{
				taskTable.selectedCharIndex = i;
			}
			if (is_selected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Character selection");
	}

	ImGui::SameLine();
	ImGui::SetNextItemWidth(100);
	if (ImGui::BeginCombo("##Peer Group Select", peerType[taskTable.selectedPeerIndex].c_str()))
	{
		for (int i = 0; i < peerType.size(); ++i)
		{
			bool isGroupSelected = (taskTable.selectedPeerIndex == i);
			if (ImGui::Selectable(peerType[i].c_str(), isGroupSelected))
			{
				taskTable.selectedPeerIndex = i;
			}
			if (isGroupSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Peer group selection");
	}

	ImGui::SameLine();
	if (ImGui::SmallButton(ICON_MD_REFRESH))
	{
		MessageHandler::requestTasks();
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Refresh tasks");
	}

	if (!taskTable.isEmpty() && taskTable.selectedCharIndex < taskTable.getPeerCount())
	{
		int selectedCharID = taskTable.getPeerAtIndex(taskTable.selectedCharIndex).getId();
		if (auto selectedCharOpt = taskTable.getCharacterById(selectedCharID))
		{
			Character selectedCharacter = *selectedCharOpt;
			if (!selectedCharacter.getTasks().empty())
			{
				if (const auto& selectedTaskOpt = selectedCharacter.getTaskByIndex(taskTable.selectedTaskIndex))
				{
					const auto& selectedTask = *selectedTaskOpt;
					if (ImGui::BeginCombo("##Task Select", selectedTask.getTaskName().data()))
					{
						for (int i = 0; i < selectedCharacter.getTasksCount(); ++i)
						{
							if (const auto& taskOpt = selectedCharacter.getTaskByIndex(i))
							{
								Task tmpTask = *taskOpt;
								bool isTaskSelected = (taskTable.selectedTaskIndex == i);
								if (ImGui::Selectable(tmpTask.getTaskName().data(), isTaskSelected))
								{
									taskTable.selectedTaskIndex = i;
								}
							}
						}
						ImGui::EndCombo();
					}
					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip("Task selection");
					}
				}
			}
		}
		else
		{
			WriteChatf(PLUGIN_MSG "Selected character not found in peer list.");
			return;
		}
		
	}
}

void TaskHudImGui::drawCharactersMissingTask(std::string_view myGroupLeader, std::string_view myZone, bool amIGrouped, const Task& selectedTask)
{
	if (!selectedTask.getMissingList().empty())
	{
		ImGui::SeparatorText("Characters missing this task");
		bool isFirst = true;
		for (int i = 0; i < selectedTask.getMissingList().size(); ++i)
		{
			const std::string_view characterName = selectedTask.getMissingNameByIndex(i);
			if (auto characterOpt = taskTable.getCharacterByName(characterName))
			{
				bool shouldDisplay = false;
				Character& character = *characterOpt;
				if (taskTable.selectedPeerIndex == 0 && amIGrouped)
				{
					shouldDisplay = character.getGroupLeaderName() == myGroupLeader;
				}
				else if (taskTable.selectedPeerIndex == 1)
				{
					shouldDisplay = character.getZoneShortName() == myZone;
				}
				else if (taskTable.selectedPeerIndex == 2)
				{
					shouldDisplay = true;
				}

				if (shouldDisplay)
				{
					if (!isFirst)
					{
						ImGui::SameLine();
						ImGui::Text(ICON_MD_REMOVE);
						ImGui::SameLine();
					}
					ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", character.getCharacterName(taskTable.anonMode).data());
					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip("Bring %s to foreground", character.getCharacterName(taskTable.anonMode).data());
						if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
						{
							DoCommandf("/dex %s foreground", character.getCharacterName().data());
						}
					}
					isFirst = false;
				}

			}
		}
	}
}

void TaskHudImGui::drawCharsMissingObj(std::string_view myGroupLeader, std::string_view myZone, const bool& amIGrouped, const Objective& objective)
{
	if (!objective.getAllDifferences().empty())
	{
		bool isThisFirst = true;
		for (const auto& objDiff : objective.getAllDifferences())
		{
			if (auto characterOpt = taskTable.getCharacterByName(objDiff.getCharacterName()))
			{
				const auto& character = *characterOpt;
				const bool isValidCharacter = (amIGrouped && taskTable.selectedPeerIndex == 0 && character.getGroupLeaderName() == myGroupLeader) ||
					(taskTable.selectedPeerIndex == 1 && character.getZoneShortName() == myZone) || taskTable.selectedPeerIndex == 2;
				if (isValidCharacter)
				{
					if (!objDiff.isEqualProgress())
					{
						if (!isThisFirst)
						{
							ImGui::SameLine();
							ImGui::Text(ICON_MD_REMOVE);
							ImGui::SameLine();
						}
						const ImVec4 color = objDiff.isObjAhead() ? ImVec4(1.0f, 0.0f, 0.0f, 1.0f) : ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
						ImGui::TextColored(color, "%s", objDiff.getCharacterName(taskTable.anonMode).data());
						if (ImGui::IsItemHovered())
						{
							ImGui::SetTooltip("Bring %s to foreground", objDiff.getCharacterName(taskTable.anonMode).data());
							if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
							{
								DoCommandf("/dex %s foreground", objDiff.getCharacterName().data());
							}
						}
						isThisFirst = false;
					}
				}
			}
		}
	}
}

void TaskHudImGui::drawTaskDetails(std::string_view myGroupLeader, std::string_view myZone, const bool& amIGrouped, const Task& selectedTask)
{
	ImGui::Separator();
	if (ImGui::BeginTable("##Objective Table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
	{
		for (const Objective& objective : selectedTask.getObjectives())
		{
			auto objValues = objective.getValues();
			ImGui::TableNextColumn();
			ImGui::Text(objValues.objectiveText.c_str());
			ImGui::TableNextColumn();


			if (objValues.isCompleted)
			{
				ImGui::TextColored(ImVec4(0, 255, 0, 255), "Done");
			}
			else
			{
				if (objValues.progress != -1 && objValues.required != -1)
				{
					ImGui::Text("%d/%d", objValues.progress, objValues.required);
				}
			}
			ImGui::TableNextColumn();
			drawCharsMissingObj(myGroupLeader, myZone, amIGrouped, objective);
			ImGui::TableNextRow();
		}
		ImGui::EndTable();
	}
}

void TaskHudImGui::drawTaskHud(const Task& selectedTask)
{
	std::string myGroupLeader;
	std::string myZone = pZoneInfo->ShortName;
	bool amIGrouped = false;

	if (!pLocalPC->Group)
	{
		amIGrouped = false;
		myGroupLeader = "none";
	}
	else
	{
		amIGrouped = true;
		myGroupLeader = pLocalPC->Group->GetGroupLeader()->GetName();
	}
	ImGui::SetNextWindowSize(ImVec2(TaskHudImGui::FIRST_WINDOW_WIDTH, TaskHudImGui::FIRST_WINDOW_HEIGHT), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Task HUD", &taskTable.showTaskHudWindow, ImGuiWindowFlags_None))
	{
		drawComboBoxes(myGroupLeader, myZone, amIGrouped);
		drawCharactersMissingTask(myGroupLeader, myZone,amIGrouped, selectedTask);
		drawTaskDetails(myGroupLeader, myZone, amIGrouped, selectedTask);
	}

	ImGui::End();
}

void TaskHudImGui::drawTaskHudLoading()
{
	ImGui::SetNextWindowSize(ImVec2(TaskHudImGui::FIRST_WINDOW_WIDTH, TaskHudImGui::FIRST_WINDOW_HEIGHT), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Task HUD", &taskTable.showTaskHudWindow, ImGuiWindowFlags_None))
	{
		ImGui::Text("Updating data.");
		ImGui::End();
		return;
	}

	ImGui::End();
}
