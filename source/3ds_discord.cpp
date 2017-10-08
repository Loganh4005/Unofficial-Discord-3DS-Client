#include "3ds_discord.h"
#include <algorithm>

ThreeDSDiscordClient::ThreeDSDiscordClient(const std::string token) :
	ThreeDSWebsocketClient(token),
	currentServerHandle(0),
	currentChannelHandle(0)
{

}

void ThreeDSDiscordClient::addNewMessage(SleepyDiscord::Message message) {
	renderer.addMessageToQueue(message.author.username + ":\n" + message.content);
}

void ThreeDSDiscordClient::onMessage(SleepyDiscord::Message message) {
	if (message.channel_id == getCurrentChannel().id) {
		addNewMessage(message);
	}
}

void ThreeDSDiscordClient::onServer(SleepyDiscord::Server server) {
	servers.push_back(server);
	printf("Added %s to the server list\n", server.name.c_str());
}

void ThreeDSDiscordClient::switchServer() {
	if (servers.size() == 0) return;
		//change current server
		if (servers.size() <= ++currentServerHandle)
			currentServerHandle = 0;
		//change channel to first channel in that server
		for (currentChannelHandle = 0; currentChannelHandle < getCurrentServer().channels.size(); ++currentChannelHandle) {
			if (getCurrentChannel().position == 0) break;
		}
	printf("Switched channel to %s #%s\n", getCurrentServer().name.c_str(), getCurrentChannel().name.c_str());
}

void ThreeDSDiscordClient::switchChannel() {
	if (servers.size() == 0) return;
	//get the position of the next channel
	 unsigned int newChannelPosition = getCurrentChannel().position + 1;
	const unsigned int channelCount = getCurrentServer().channels.size();
	if (channelCount <= newChannelPosition)
		newChannelPosition = 0;
	do {
		//get the channel in that position
		for (currentChannelHandle = 0; currentChannelHandle < channelCount; ++currentChannelHandle) {
			if (getCurrentChannel().position == newChannelPosition
				&&
				getCurrentChannel().type == SleepyDiscord::Channel::SERVER_TEXT
				) {
				printf("Switched channel to %s #%s\n", getCurrentServer().name.c_str(), getCurrentChannel().name.c_str());
				return;
			}
		}
		newChannelPosition = 0;
	} while (true);
}

void ThreeDSDiscordClient::loadMessages() {
	std::vector<SleepyDiscord::Message> messages = getMessages(getCurrentChannel().id, limit, "", 8);
	for (std::vector<SleepyDiscord::Message>::reverse_iterator i = messages.rbegin(), end = messages.rend(); i != end; ++i) {
		addNewMessage(*i);
	}
}

void ThreeDSDiscordClient::launchKeyboardAndSentMessage() {
	//tell discord that we are typing
	if (sendTyping(getCurrentChannel().id) == false) //error check
		return;
	//init keyboard
	const int numOfButtons = 2;
	const int messageCharLimit = 2000;
	SwkbdState keyboard;
	swkbdInit(&keyboard, SWKBD_TYPE_NORMAL, numOfButtons, messageCharLimit);
	//set the hint to the name of the channel
	std::string keyboardHint;
	const char* keyboardHintStart = "Message #";
	keyboardHint.reserve(sizeof(keyboardHintStart) + getCurrentChannel().name.length());
	keyboardHint += keyboardHintStart;
	keyboardHint += getCurrentChannel().name;
	swkbdSetHintText(&keyboard, keyboardHint.c_str());
	//back to init keyboard
	swkbdSetInitialText(&keyboard, "");
	swkbdSetFeatures(&keyboard, SWKBD_DEFAULT_QWERTY | SWKBD_DARKEN_TOP_SCREEN);
	swkbdSetValidation(&keyboard, SWKBD_NOTEMPTY_NOTBLANK, 0, 0);
	//get input from the keyboard
	std::vector<char> buffer(messageCharLimit);
	SwkbdButton button = swkbdInputText(&keyboard, &buffer[0], messageCharLimit);
	//read input
	if (button != SWKBD_BUTTON_CONFIRM)
		return;	//return if user did not press confirm
	auto iteratorToZero = std::find(buffer.begin(), buffer.end(), 0); //find end of string
	if (iteratorToZero == buffer.end()) {
		printf("message doesn't end");
		return;
	}
	//send message
	sendMessage(getCurrentChannel().id, std::string(buffer.begin(), iteratorToZero));
}