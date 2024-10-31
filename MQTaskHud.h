#pragma once

#define PLUGIN_MSG "\ay[\agTaskHud\ay]\aw "

#include "Taskhud.pb.h"


#ifdef _DEBUG
#pragma comment(lib, "libprotobufd")
#else
#pragma comment(lib, "libprotobuf")
#endif

struct Objective {
	char objectiveText[MAX_STRING];
	bool isCompleted;
	int progress;
	int index;

	Objective() {}

	Objective(const char text[MAX_STRING], bool completed, int prog, int idx) : isCompleted(completed), progress(prog), index(idx) {
		strncpy_s(objectiveText, text, MAX_STRING);
	}
};

struct Task {
	char name[MAX_STRING];
	std::vector<Objective> objectives;

	Task() {}

	explicit Task(const char* taskName) { strncpy_s(name, taskName, MAX_STRING); }

	Task(const char* taskName, const std::vector<Objective>& objs) : objectives(objs) { strncpy_s(name, taskName, MAX_STRING); }

	void addObjective(const Objective& obj) {
		objectives.push_back(obj);
	}
};

struct Character {
	std::string name;
	std::vector<Task> tasks;

	Character() {}

	explicit Character(const std::string& characterName) : name(characterName) {}

	Character(const std::string& characterName, const std::vector<Task>& characterTasks) : name(characterName), tasks(characterTasks) {}

	void addTask(const Task& task) {
		tasks.push_back(task);
	}
};

void TH_Cmd(PlayerClient* pChar, char* szLine);
std::vector<Task> get_tasks();
void send_tasks(std::vector<Task>);
void request_tasks();
void HandleMessage(const std::shared_ptr<postoffice::Message>& message);