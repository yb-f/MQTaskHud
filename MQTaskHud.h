#pragma once

#define PLUGIN_MSG "\ay[\agTaskHud\ay]\aw "


#include "Taskhud.pb.h"
#include <mq/Plugin.h>

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
		std::string_view characterName;
	};

	//constructors
	ObjectiveDifference()
		: objectiveIndex(-1), equalProgress(false), isAhead(false), characterName("") {}

	ObjectiveDifference(int index, bool equal, bool ahead, const std::string& name)
		: objectiveIndex(index), equalProgress(equal), isAhead(ahead), characterName(name)
	{
		anonCharacterName = std::string(1, characterName.front()) + "***" + std::string(1, characterName.back());
	}

	//setter
	void setValues(int index, bool equal, bool ahead, const std::string& name)
	{
		objectiveIndex = index;
		equalProgress = equal;
		isAhead = ahead;
		characterName = name;
		anonCharacterName = std::string(1, characterName.front()) + "***" + std::string(1, characterName.back());
	}

	//getters
	const DiffValues getValues() const
	{
		return { objectiveIndex, equalProgress, isAhead, characterName };
	}

	bool isEqualProgress() const {
		return equalProgress;
	}

	bool isObjAhead() const {
		return isAhead;
	}

	const std::string_view getCharacterName(bool anonMode) const
	{
		if (anonMode)
		{
			return anonCharacterName;
		}
		return characterName;
	}

	const std::string_view getCharacterName() const
	{
		return getCharacterName(false);
	}

private:
	int objectiveIndex;
	bool equalProgress;
	bool isAhead;
	std::string characterName;
	std::string anonCharacterName;
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

	//constructor
	Objective(const std::string& text, bool completed, int prog, int req, int idx)
		: objectiveText(text), isCompleted(completed), progress(prog), required(req), index(idx) {}

	//setters
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

	void setProgress(bool complete, int prog)
	{
		isCompleted = complete;
		progress = prog;
	}

	//getters
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

	const std::vector<ObjectiveDifference>& getAllDifferences() const {
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
	explicit Task(const std::string& taskName)
		: taskName(taskName) {}

	//setters
	void setTaskName(const std::string& taskNm) {
		taskName = taskNm;
	}

	void addObjective(const Objective& obj) {
		objectives.push_back(obj);
	}

	void setUid(unsigned long long i_uid)
	{
		uid = i_uid;
	}

	void setObjectives(const std::vector<Objective>& obj) {
		objectives = obj;
	}
	
	void clearMissingList() {
		missingList.clear();
	}

	void addToMissingList(const std::string& missingChar) {
		missingList.push_back(missingChar);
	}

	void setMissingList(const std::vector<std::string>& missingChars) {
		missingList = missingChars;
	}

	//getters
	std::vector<Objective>& getObjectives() {
		return objectives;
	}

	// Overload for const Task
	const std::vector<Objective>& getObjectives() const {
		return objectives;
	}

	const std::string_view getTaskName() const {
		return taskName;
	}

	const std::vector<std::string>& getMissingList() const {
		return missingList;
	}

	const std::string_view getMissingNameByIndex(int idx) const {
		return missingList.at(idx);
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
	//constructor
	Character(const std::string& characterName, const std::string& groupLeaderName, const std::string& zoneShortName, const int& charId)
		: charId(charId), charName(characterName), groupLeaderName(groupLeaderName), zoneShortName(zoneShortName)
	{
		anonCharName = createAnonymizedName(characterName);
		anonGroupLeaderName = createAnonymizedName(groupLeaderName);
	}
	

	//setters
	void setGroupLeaderName(const std::string& leaderName) {
		groupLeaderName = leaderName;
		anonGroupLeaderName = createAnonymizedName(leaderName);
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

	const std::string_view getGroupLeaderName(bool anonMode) const
	{
		if (anonMode)
		{
			return anonGroupLeaderName;
		}
		return groupLeaderName;
	}

	const std::string_view getGroupLeaderName() const {
		return getGroupLeaderName(false);
	}

	const std::string_view getCharacterName(bool anonMode) const
	{
		if (anonMode)
		{
			return anonCharName;
		}
		return charName;
	}

	const std::string_view getCharacterName() const
	{
		return getCharacterName(false);
	}

	const std::string_view getZoneShortName() const {
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
	std::string anonCharName;
	std::string groupLeaderName;
	std::string anonGroupLeaderName;
	std::string zoneShortName;
	std::vector<Task> tasks;

	static std::string createAnonymizedName(const std::string& name) {
		return std::string(1, name.front()) + "***" + std::string(1, name.back());
	}

	unsigned long long generateTaskUid(const std::string_view taskName) const {
		std::hash<std::string_view> hashFunction;
		return (hashFunction(taskName));
	}
};

class Peer {
public:
	//constructors
	Peer(const std::string& name, const std::string& leaderName, const std::string& zone, int id) :
	name(name), groupLeaderName(leaderName), zoneName(zone), id(id), missedHeartbeats(0)
	{
		anonName = createAnonName(name);
		anonGroupLeaderName = createAnonName(groupLeaderName);
	}

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

	void setZoneName(std::string zoneNm)
	{
		zoneName = zoneNm;
	}

	void setGroupLeaderName(std::string leaderName)
	{
		groupLeaderName = leaderName;
		anonGroupLeaderName = createAnonName(leaderName);
	}

	//getters
	int getMissedHeartbeats() const {
		return missedHeartbeats;
	}

	const std::string_view getName(bool anonMode) const
	{
		if (anonMode)
		{
			return anonName;
		}
		return name;

	}

	const std::string_view getName() const
	{
		return getName(false);
	}

	const std::string_view getGroupLeaderName(bool anonMode) const
	{
		if (anonMode)
		{
			return anonGroupLeaderName;
		}
		return groupLeaderName;
	}

	const std::string_view getGroupLeaderName() const
	{
		return getGroupLeaderName(false);
	}

	int getId() const {
		return id;
	}

	const std::string_view getZoneName() const
	{
		return zoneName;
	}

	
private:
	std::string name;
	std::string groupLeaderName;
	std::string zoneName;
	std::string anonName;
	std::string anonGroupLeaderName;
	int id;
	int missedHeartbeats;

	static std::string createAnonName(const std::string& name) {
		return std::string(1, name.front()) + "***" + std::string(1, name.back());
	}
};

class TaskTable {
public:
	//public members
	bool showTaskHudWindow = false;
	bool anonMode = false;
	bool isRegistered = false;
	bool communicationEnabled = false;
	bool welcomeSent = false;
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
		peerList.emplace_back(Peer(name, leaderName, zoneName, id));
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
		for (auto& character : characters) {
			if (character.getId() == id) {
				return character;
			}
		}
		return std::nullopt;
	}
	
	std::optional<Peer> getPeerById(int id) {
		for (auto& peer : peerList) 
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

	Peer& getPeerAtIndex(int idx) {
		return peerList[idx];
	}

	std::vector<Peer>& getPeers() {
		return peerList;
	}

	std::optional<Character> getCharacterByName(const std::string_view name) {
		for (auto& character : characters) {
			if (character.getCharacterName() == name) {
				return character;
			}
		}
		return std::nullopt;
	}


private:
	std::vector<Character> characters;
	std::vector<Peer> peerList;
};


std::vector<Task> getTasks();
void requestTasks();
void requestPeers();
void compareTasks();

extern TaskTable taskTable;
extern postoffice::DropboxAPI THDropbox;