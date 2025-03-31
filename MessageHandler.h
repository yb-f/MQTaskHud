#pragma once

#include "Taskhud.pb.h"
#include "MQTaskHud.h"

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
	// Send functions
	static void requestTasks();
	static void sendTasks(const std::vector<Task>& tasks);
	static void sendHeartbeatMessages(HeartbeatType type, int charId);
	static void sendHeartbeatMessages(HeartbeatType type);

	// Process functions
	static void processIncomingMessage(const proto::TaskHud::TaskTable& protoTaskTable);
	static void processRequestMessage();
	static void processRegisterMessage(const proto::TaskHud::HeartbeatMessages& heartbeatMessage);
	static void processRegisterResponseMessage(const proto::TaskHud::HeartbeatMessages& heartbeatMessage);
	static void processPauseHeartbeatMessage(const proto::TaskHud::HeartbeatMessages& heartbeatMessage);
	static void processResumeHeartbeatMessage(const proto::TaskHud::HeartbeatMessages& heartbeatMessage);
	static void processHeartbeatMessage(const proto::TaskHud::HeartbeatMessages& heartbeatMessage);
};