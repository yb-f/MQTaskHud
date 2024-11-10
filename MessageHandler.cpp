#include <mq/Plugin.h>
#include "mq/api/ActorAPI.h"
#include "MQTaskHud.h"
#include "MessageHandler.h"
#include "routing/PostOffice.h"

//send
void MessageHandler::sendTasks(const std::vector<Task>& tasks) {
	if (GetGameState() != GAMESTATE_INGAME) {
		return;
	}
	proto::TaskHud::TaskTable message;
	message.set_character(pLocalPlayer->DisplayedName);
	message.set_groupleadername(pLocalPC->Group ? std::string(pLocalPC->Group->GetGroupLeader()->GetName()) : "none");
	message.set_zone(pZoneInfo->ShortName);
	message.set_charid(GetCurrentProcessId());

	for (const auto& task : tasks) {
		auto* protoTask = message.add_tasks();
		protoTask->set_name(task.getTaskName());
		for (const auto& objective : task.getObjectives()) {
			auto* protoObjective = protoTask->add_objectives();
			const auto& values = objective.getValues();
			protoObjective->set_objectivetext(values.objectiveText);
			protoObjective->set_iscompleted(values.isCompleted);
			protoObjective->set_progress(values.progress);
			protoObjective->set_required(values.required);
			protoObjective->set_index(values.index);
		}
	}

	proto::TaskHud::TaskHud payload;
	payload.set_id(proto::TaskHud::Incoming);
	payload.set_payload(message.SerializeAsString());
	postoffice::Address address;
	THDropbox.Post(address, payload);
}

void MessageHandler::sendHeartbeatMessages(HeartbeatType type, int charId) {
	proto::TaskHud::HeartbeatMessages message;
	message.set_name(pLocalPlayer->DisplayedName);
	message.set_zone(pZoneInfo->ShortName);
	message.set_groupleadername(pLocalPC->Group ? std::string(pLocalPC->Group->GetGroupLeader()->GetName()) : "none");
	message.set_charid(GetCurrentProcessId());
	postoffice::Address address;
	proto::TaskHud::TaskHud payload;
	switch (type) {
	case HeartbeatType::Register: 
		payload.set_id(proto::TaskHud::Register);
		break;
	case HeartbeatType::RegisterResponse: 
		payload.set_id(proto::TaskHud::RegisterResponse);
		address.PID = charId;
		break;
	case HeartbeatType::PauseHeartbeat: 
		payload.set_id(proto::TaskHud::PauseHeartbeat);
		break;
	case HeartbeatType::ResumeHeartbeat: 
		payload.set_id(proto::TaskHud::ResumeHeartbeat);
		break;
	case HeartbeatType::Heartbeat: 
		payload.set_id(proto::TaskHud::Heartbeat);
		break;
	}
	payload.set_payload(message.SerializeAsString());

	THDropbox.Post(address, payload);
}

void MessageHandler::sendHeartbeatMessages(HeartbeatType type)
{
	sendHeartbeatMessages(type, 0);
}

//process data
void MessageHandler::processIncomingMessage(proto::TaskHud::TaskTable& protoTaskTable) {
	Character newCharacter(protoTaskTable.character());
	newCharacter.setGroupLeaderName(protoTaskTable.groupleadername());
	for (const auto& protoTask : protoTaskTable.tasks()) {
		Task newTask(protoTask.name());
		for (const auto& protoObjective : protoTask.objectives()) {
			Objective newObjective(
				protoObjective.objectivetext(),
				protoObjective.iscompleted(),
				protoObjective.progress(),
				protoObjective.required(),
				protoObjective.index()
			);
			newTask.addObjective(newObjective);
		}
		newCharacter.addTask(newTask);
	}
	newCharacter.setZoneShortName(protoTaskTable.zone());
	newCharacter.setCharId(protoTaskTable.charid());
	taskTable.addCharacter(newCharacter);
	if (taskTable.getCharacterCount() == taskTable.getPeerCount())
	{
		compareTasks();
	}
}

void MessageHandler::processRequestMessage() {
	taskTable.clearAllCharacters();
	std::vector<Task> my_tasks = getTasks();
	sendTasks(my_tasks);
}


//process registration
//register message
void MessageHandler::processRegisterMessage(proto::TaskHud::HeartbeatMessages& heartbeatMessage) {
	bool peerExists = false;
	bool updated = false;

	for (auto& peer : taskTable.getPeers())
	{
		if (peer.getName() == heartbeatMessage.name())
		{
			peerExists = true;
			if (peer.getId() == heartbeatMessage.charid())
			{
				updated = false;
				break;
			}
			else
			{
				taskTable.removePeerById(peer.getId());
				updated = true;
				break;
			}
		}
	}

	if (!peerExists || updated)
	{
		taskTable.addPeer(heartbeatMessage.name(), heartbeatMessage.groupleadername(), heartbeatMessage.zone(), heartbeatMessage.charid());
		taskTable.sortPeersByName();
		if (auto&& peerOpt = taskTable.getPeerById(heartbeatMessage.charid()))
		{
			auto& peer = *peerOpt;
			peer.resetHeartbeats();
		}
	}
	sendHeartbeatMessages(HeartbeatType::RegisterResponse, heartbeatMessage.charid());
}

//register response message
void MessageHandler::processRegisterResponseMessage(proto::TaskHud::HeartbeatMessages& heartbeatMessage) {
	bool peerExists = false;
	bool updated = false;

	for (auto& peer : taskTable.getPeers())
	{
		if (peer.getName() == heartbeatMessage.name())
		{
			peerExists = true;
			if (peer.getId() == heartbeatMessage.charid())
			{
				updated = false;
				break;
			}
			else
			{
				taskTable.removePeerById(heartbeatMessage.charid());
				updated = true;
				break;
			}
		}
	}

	if (!peerExists || updated)
	{
		taskTable.addPeer(heartbeatMessage.name(), heartbeatMessage.groupleadername(), heartbeatMessage.zone(), heartbeatMessage.charid());
		taskTable.sortPeersByName();
		if (auto&& peerOpt = taskTable.getPeerById(heartbeatMessage.charid()))
		{
			auto& peer = *peerOpt;
			peer.resetHeartbeats();
		}
	}
}


//process heartbeats
//pause message
void MessageHandler::processPauseHeartbeatMessage(proto::TaskHud::HeartbeatMessages& heartbeatMessage) {
	if (heartbeatMessage.name() == pLocalPlayer->DisplayedName) return;
	bool peerExists = false;
	for (auto& peer : taskTable.getPeers())
	{
		if (peer.getName() == heartbeatMessage.name())
		{
			peer.delayHeartbeats();
		}
	}
	if (peerExists == false)
	{
		requestPeers();
	}
}

//resume message
void MessageHandler::processResumeHeartbeatMessage(proto::TaskHud::HeartbeatMessages& heartbeatMessage) {
	bool peerExists = false;
	for (auto& peer : taskTable.getPeers())
	{
		if (peer.getName() == heartbeatMessage.name())
		{
			peer.resetHeartbeats();
		}
	}
	if (peerExists == false)
	{
		requestPeers();
	}
}

//heartbeat message
void MessageHandler::processHeartbeatMessage(proto::TaskHud::HeartbeatMessages& heartbeatMessage) {
	bool peerExists = false;
	for (auto& peer : taskTable.getPeers())
	{
		if (peer.getName() == heartbeatMessage.name())
		{
			peer.resetHeartbeats();
		}
	}
	if (peerExists == false)
	{
		requestPeers();
	}
}