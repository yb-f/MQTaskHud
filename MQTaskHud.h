#pragma once

#define PLUGIN_MSG "\ay[\agTaskHud\ay]\aw "

#include "Taskhud.pb.h"


#ifdef _DEBUG
#pragma comment(lib, "libprotobufd")
#else
#pragma comment(lib, "libprotobuf")
#endif

class ObjectiveDifference {
public:
	struct DiffValues {
		int objectiveIndex;
		bool equalProgress;
		bool isAhead;
		std::string characterName;
	};
	//constructors
	ObjectiveDifference()
		: objectiveIndex(-1), equalProgress(false), isAhead(false), characterName("") {}

	ObjectiveDifference(int index, bool equal, bool ahead, const std::string& name)
		: objectiveIndex(index), equalProgress(equal), isAhead(ahead), characterName(name) {}

	//setter
	void setValues(int index, bool equal, bool ahead, const std::string& name)
	{
		objectiveIndex = index;
		equalProgress = equal;
		isAhead = ahead;
		characterName = name;
	}

	//getters
	DiffValues getValues() const
	{
		return { objectiveIndex, equalProgress, isAhead, characterName };
	}

	bool isEqualProgress() const {
		return equalProgress;
	}

	bool isObjAhead() const {
		return isAhead;
	}

	const std::string getCharacterName(bool anonMode) const { 
		if (!anonMode) {
			return characterName;
		}
		return std::string(1, characterName.front()) + "***" + std::string(1, characterName.back());
	}

	const std::string getCharacterName() const {
		return getCharacterName(false);
	}

private:
	int objectiveIndex;
	bool equalProgress;
	bool isAhead;
	std::string characterName;
};

class Objective {
public:
	struct ObjValues {
		std::string objectiveText;
		bool isCompleted;
		int progress;
		int required;
		int index;
		std::vector<ObjectiveDifference> objectiveDifferences;
	};

	Objective() : objectiveText(""), isCompleted(false), progress(0), required(0), index(-1) {}

	Objective(const std::string& text, bool completed, int prog, int req, int idx)
		: objectiveText(text), isCompleted(completed), progress(prog), required(req), index(idx) {}

	void addObjectiveDifference(const ObjectiveDifference& diff) {
		for (auto& existingDiff : objectiveDifferences) {
			if (existingDiff.getCharacterName() == diff.getCharacterName()) {
				existingDiff = diff;
				return;
			}
		}
		objectiveDifferences.push_back(diff);
	}

	void setValues(const std::string& text, bool completed, int prog, int req, int idx) {
		objectiveText = text;
		isCompleted = completed;
		progress = prog;
		required = req;
		index = idx;
	}

	void setProgress(bool complete, int prog, int req) {
		isCompleted = complete;
		progress = prog;
		required = req;
	}

	void setProgress(bool complete, int prog)
	{
		isCompleted = complete;
		progress = prog;
	}

	bool getCompleted() const
	{
		return isCompleted;
	}

	ObjValues getValues() const {
		return { objectiveText, isCompleted, progress, required, index, objectiveDifferences };
	}

	bool hasObjectiveDifferences() const {
		return !objectiveDifferences.empty();
	}

	std::vector<ObjectiveDifference> getAllDifferences() const {
		return objectiveDifferences;
	}

	int getIndex() const {
		return index;
	}

	int getProgress() const {
		return progress;
	}

private:
	std::string objectiveText;
	bool isCompleted;
	int progress;
	int required;
	int index;
	std::vector<ObjectiveDifference> objectiveDifferences;
};

class Task {
public:
	//constructors
	Task() : taskName("") {}
	
	explicit Task(const std::string& taskName)
		: taskName(taskName) {}

	Task(const std::string& taskName, unsigned long long uid)
		: taskName(taskName), uid(uid) {}

	Task(const std::string& taskName, const std::vector<Objective>& objectives)
		: taskName(taskName), objectives(objectives) {}


	//setters
	void setTaskName(const std::string& taskNm) {
		this->taskName = taskNm;
	}

	void addObjective(const Objective& obj) {
		objectives.push_back(obj);
	}

	void setUid(unsigned long long i_uid)
	{
		uid = i_uid;
	}

	void setObjectives(const std::vector<Objective>& obj) {
		this->objectives = obj;
	}
	
	void clearMissingList() {
		missingList.clear();
	}

	void addToMissingList(const std::string& missingChar) {
		missingList.push_back(missingChar);
	}

	void setMissingList(const std::vector<std::string>& missingChars) {
		this->missingList = missingChars;
	}

	//getters
	std::vector<Objective>& getObjectives() {
		return objectives;
	}

	// Overload for const Task
	const std::vector<Objective>& getObjectives() const {
		return objectives;
	}

	std::string getTaskName() const {
		return taskName;
	}

	std::vector<std::string> getMissingList() const {
		return missingList;
	}

	//is this function used?
	std::string getMissingNameByIndex(int idx) const {
		return missingList[idx];
	}

	unsigned long long getUid() const {
		return uid;
	}

private:
	std::string taskName;
	unsigned long long uid;
	std::vector<Objective> objectives;
	std::vector<std::string> missingList;
};

class Character {
public:
	//constructors
	Character() : charId(-1), charName(""), groupLeaderName("") {}

	explicit Character(const std::string& characterName)
		: charId(-1), charName(characterName), groupLeaderName("") {}

	Character(int id, const std::string& characterName)
		: charId(id), charName(characterName), groupLeaderName("") {}

	Character(int id, const std::string& characterName, const std::string& leaderName)
		: charId(id), charName(characterName), groupLeaderName(leaderName) {}

	//setters
	void setCharId(const int id)
	{
		charId = id;
	}

	void setCharName(const std::string& characterName) {
		this->charName = characterName;
	}

	void setGroupLeaderName(const std::string& leaderName) {
		this->groupLeaderName = leaderName;
	}

	void setZoneShortName(const std::string& shortName) {
		this->zoneShortName = shortName;
	}

	void addTask(Task& task) {
		task.clearMissingList();
		task.setUid(generateTaskUid(task.getTaskName()));
		tasks.push_back(task);
	}

	void clearTasks() {
		tasks.clear();
	}

	void deleteTaskByName(const std::string& taskName) {
		for (auto it = tasks.begin(); it != tasks.end(); ++it) {
			if (it->getTaskName() == taskName) {
				tasks.erase(it);
				break;
			}
		}
	}

	//getters
	int getId() const
	{
		return charId;
	}

	std::string getCharName(bool anonMode) const {
		if (!anonMode) {
			return charName;
		}
		return std::string(1, charName.front()) + "***" + std::string(1, charName.back());
	}

	std::string getCharName() const {
		return getCharName(false);
	}

	std::string getGroupLeaderName(bool anonMode) const {
		if (!anonMode) {
			return groupLeaderName;
		}

		return std::string(1, groupLeaderName.front()) + "***" + std::string(1, groupLeaderName.back());
	}

	std::string getGroupLeaderName() const {
		return getGroupLeaderName(false);
	}

	std::string getZoneShortName() const {
		return zoneShortName;
	}

	std::optional<Task> getTaskByUid(unsigned long long uid) {
		for (auto& task : tasks) {
			if (task.getUid() == uid) {
				return task;
			}
		}
		return std::nullopt;
	}

	std::optional<Task> getTaskByIndex(int idx) {
		if (idx >= 0 && idx < tasks.size()) {
			return tasks[idx];
		}
		else {
			return std::nullopt;  // Return an empty optional if the index is out of range
		}
	}

	std::vector<Task>& getTasks() {
		return tasks;
	}

	int getTasksCount() const
	{
		return static_cast<int>(tasks.size());
	}

private:
	int charId;
	std::string charName;
	std::string groupLeaderName;
	std::string zoneShortName;
	std::vector<Task> tasks;
		
	unsigned long long generateTaskUid(const std::string& taskName) const {
		std::hash<std::string> hashFunction;
		return (hashFunction(taskName));
	}
};

class Peer {
private:
	std::string name;
	std::string groupLeaderName;
	std::string zoneName;
	int id;
	int missedHeartbeats;

public:
	//constructors
	Peer(const std::string& name, const std::string& leaderName, const std::string& zone, int id) : name(name), groupLeaderName(leaderName), zoneName(zone), id(id), missedHeartbeats(0) {}
		
	Peer(const std::string& name) : name(name) {}

	//setters
	void resetHeartbeats() {
		missedHeartbeats = 0;
	}

	void delayHeartbeats() {
		missedHeartbeats = -30;
	}

	void incrementHeartbeats()
	{
		missedHeartbeats += 1;
	}

	//getters
	int getMissedHeartbeats() const {
		return missedHeartbeats;
	}

	std::string getName(bool anonMode) const {
		if (!anonMode)
		{
			return name;
		}
		return std::string(1, name.front()) + "***" + std::string(1, name.back());
	}

	std::string getName() const
	{
		return getName(false);
	}

	int getId() const {
		return id;
	}
};

class TaskTable {
public:
	//public members
	bool showTaskHudWindow = false;
	bool anonMode = false;
	bool isRegistered = false;
	int selectedCharIndex = 0;
	int selectedPeerIndex = 0;
	int selectedTaskIndex = 0;

	//setters
	void addCharacter(const Character& character) {
		characters.push_back(character);
	}

	void removeCharacter(int id) {
		for (auto it = characters.begin(); it != characters.end(); ++it) {
			if (it->getId() == id) {
				characters.erase(it);
				return;
			}
		}
	}

	void addPeer(const std::string& name, const std::string& leaderName, const std::string& zoneName, int id) {
		peerList.emplace_back(name, leaderName, zoneName, id);
	}

	void removePeerById(int id) {
		for (auto it = peerList.begin(); it != peerList.end(); ) {
			if (it->getId() == id) {
				it = peerList.erase(it);
				return;
			}
			else {
				++it;
			}
		}
	}

	void removePeerByName(const std::string& name) {
		for (auto it = peerList.begin(); it != peerList.end(); ) {
			if (it->getName() == name) {
				it = peerList.erase(it);
				return;
			}
			else {
				++it;
			}
		}
	}

	void sortPeersByName() {
		std::sort(peerList.begin(), peerList.end(),
			[](const Peer& a, const Peer& b) { return a.getName() < b.getName(); });
	}

	void clearAllCharacters() {
		characters.clear();
	}

	void clearPeerList() {
		peerList.clear();
	}

	//getters
	bool isEmpty() const {
		return characters.empty();
	}

	std::vector<Character>& getCharacters() {
		return characters;
	}

	bool isNameInPeerList(const std::string& name) const
	{
		for (const auto& peer : peerList) {
			if (peer.getName() == name) {
				return true;
			}
		}
		return false;
	}
	
	std::optional<Character> getCharacterById(int id) {
		for (const auto& character : characters) {
			if (character.getId() == id) {
				return character;
			}
		}
		return std::nullopt;
	}
	
	std::optional<Peer> getPeerById(int id) const {
		for (const auto& peer : peerList) 
		{
			if (peer.getId() == id) {
				return peer;
			}
		}
		return std::nullopt;
	}

	int getPeerCount() const {
		return static_cast<int>(peerList.size());
	}

	int getCharacterCount() const {
		return static_cast<int>(characters.size());
	}

	Peer getPeerAtIndex(int idx) {
		return peerList[idx];
	}

	std::vector<Peer>& getPeers() {
		return peerList;
	}

	std::optional<Character> getCharacterByName(const std::string& name) {
		for (const auto& character : characters) {
			if (character.getCharName() == name) {
				return character;
			}
		}
		return std::nullopt;
	}


private:
	std::vector<Character> characters;
	std::vector<Peer> peerList;
};

void drawComboBoxes(const std::string& myGroupLeader, const std::string& myZone, const bool& amIGrouped);
void drawCharactersMissingTask(std::string_view myGroupLeader, std::string_view myZone, const Task& selectedTask);
void drawCharsMissingObj(std::string_view myGroupLeader, std::string_view myZone, const bool& amIGrouped, const Objective& objective);
void drawTaskDetails(std::string_view myGroupLeader, std::string_view myZone, const bool& amIGrouped, const Task& selectedTask);
void drawTaskHud(const Task& selectedTask);
void drawTaskHudLoading();
std::vector<Task> getTasks();
void requestTasks();
void requestPeers();
void compareTasks();

extern TaskTable taskTable;
extern postoffice::DropboxAPI THDropbox;