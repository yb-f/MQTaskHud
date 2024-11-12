#pragma once
#include "MQTaskHud.h"
#include <mq/Plugin.h>

class TaskHudImGui {
public:
	static void drawComboBoxes(const std::string& myGroupLeader, const std::string& myZone, const bool& amIGrouped);
	static void drawCharactersMissingTask(std::string_view myGroupLeader, std::string_view myZone, bool amIGrouped, const Task& selectedTask);
	static void drawCharsMissingObj(std::string_view myGroupLeader, std::string_view myZone, const bool& amIGrouped, const Objective& objective);
	static void drawTaskDetails(std::string_view myGroupLeader, std::string_view myZone, const bool& amIGrouped, const Task& selectedTask);
	static void drawTaskHud(const Task& selectedTask);
	static void drawTaskHudLoading();

	static constexpr int FIRST_WINDOW_WIDTH = 445;
	static constexpr int FIRST_WINDOW_HEIGHT = 490;
	static inline const std::vector<std::string> peerType = { "Group", "Zone", "All" };
};
