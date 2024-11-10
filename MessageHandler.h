#pragma once

#include "Taskhud.pb.h"
#include "MQTaskHud.h"
#include "mq/api/ActorAPI.h"

enum class HeartbeatType
{
	Register,
	RegisterResponse,
	PauseHeartbeat,
	ResumeHeartbeat,
	Heartbeat
};

class MessageHandler {
public:
	MessageHandler();

	// Send functions
	static void sendTasks(const std::vector<Task>& tasks);
	static void sendHeartbeatMessages(HeartbeatType type, int charId);
	static void sendHeartbeatMessages(HeartbeatType type);

	// Process functions
	static void processIncomingMessage(proto::TaskHud::TaskTable& protoTaskTable);
	static void processRequestMessage();
	static void processRegisterMessage(proto::TaskHud::HeartbeatMessages& heartbeatMessage);
	static void processRegisterResponseMessage(proto::TaskHud::HeartbeatMessages& heartbeatMessage);
	static void processPauseHeartbeatMessage(proto::TaskHud::HeartbeatMessages& heartbeatMessage);
	static void processResumeHeartbeatMessage(proto::TaskHud::HeartbeatMessages& heartbeatMessage);
	static void processHeartbeatMessage(proto::TaskHud::HeartbeatMessages& heartbeatMessage);
};