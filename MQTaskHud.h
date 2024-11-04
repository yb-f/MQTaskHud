#pragma once

#define PLUGIN_MSG "\ay[\agTaskHud\ay]\aw "

#include "Taskhud.pb.h"


#ifdef _DEBUG
#pragma comment(lib, "libprotobufd")
#else
#pragma comment(lib, "libprotobuf")
#endif

struct ObjectiveDifference {
	int objectiveIndex;
	bool isAhead;
	std::string characterName;

	ObjectiveDifference() : objectiveIndex(-1), isAhead(false), characterName("") {}

	ObjectiveDifference(int index, bool ahead, const std::string& name)
		: objectiveIndex(index), isAhead(ahead), characterName(name) {}
};

struct Objective {
	std::string objectiveText;
	bool isCompleted;
	int progress;
	int required;
	int index;
	std::map<int, std::vector<ObjectiveDifference>> objectiveDifferences;

	Objective() : objectiveText(""), isCompleted(false), progress(0), required(0), index(-1) {}

	Objective(const std::string& text, bool completed, int prog, int req, int idx)
		: objectiveText(text), isCompleted(completed), progress(prog), required(req), index(idx) {}

	void addObjectiveDifference(const ObjectiveDifference& diff)
	{
		auto& diffs = objectiveDifferences[diff.objectiveIndex];
		for (const auto& existingDiff : diffs) {
			if (existingDiff.characterName == diff.characterName) {
				return;
			}
		}
		diffs.push_back(diff);
	}

	void compareWith(const Objective& other, const std::string& otherCharacterName)
	{
		if (this->index == other.index) {
			if (this->progress != other.progress || this->isCompleted != other.isCompleted) {
				bool isAhead = false;
				if (!this->isCompleted && other.isCompleted)
				{
					isAhead = true;
				}
				else
				{
					isAhead = this->progress < other.progress;
				}
				ObjectiveDifference diff(this->index, isAhead, otherCharacterName);
				addObjectiveDifference(diff);
			}
		}
	}
};

struct Task {
	std::string name;
	std::vector<Objective> objectives;
	std::vector<std::string> missingList;

	Task() {}

	explicit Task(const std::string& taskName) : name(taskName) {}

	Task(const std::string& taskName, const std::vector<Objective>& objs) : name(taskName), objectives(objs) {}

	void addObjective(const Objective& obj) {
		objectives.push_back(obj);
	}

	void clearMissingList() {
		missingList.clear();
	}
};

struct Character {
	std::string name;
	std::vector<Task> tasks;

	Character() : name("") {}

	explicit Character(const std::string& characterName) : name(characterName) {}

	Character(const std::string& characterName, const std::vector<Task>& characterTasks)
		: name(characterName), tasks(characterTasks) {}

	void addTask(const Task& task) {
		tasks.push_back(task);
	}

	void clearTasks() {
		tasks.clear();
	}
};