#include <iostream>
#include <algorithm>
#include <string>
#include <thread>
#include <vector>

#include <map>
#include <iterator>
#include <unordered_map>
#include <shared_mutex>

#include "config.h"
#include "TCPServer.h"
#include "TCPClient.h"
#include "RequestParser.h"

#define DEFAULT_PORT 12345
bool terminateServer = false;
void serverThreadFunction(TCPServer* server, ReceivedSocketData && data);

std::shared_mutex m_mutex;
std::shared_mutex m_mutex2;
std::unordered_map<std::string, std::vector<std::string>> umap;
int maxL = 140;


std::string parseRequest(std::string request);
std::string handle_posts(PostRequest& post);
std::string handle_Reads(ReadRequest& read);
std::string handle_Count(CountRequest& count);
std::string handle_List();


int main()
{
	TCPServer server(DEFAULT_PORT);

	ReceivedSocketData receivedData;

	std::vector<std::thread> serverThreads;

	std::cout << "Starting server. Send \"exit\" (without quotes) to terminate." << std::endl;

	while (!terminateServer)
	{
		receivedData = server.accept();

		if (!terminateServer)
		{
			serverThreads.emplace_back(serverThreadFunction, &server, receivedData);
		}
	}

	for (auto& th : serverThreads)
		th.join();

	std::cout << "Server terminated." << std::endl;

	return 0;
}



 std::string handle_posts(PostRequest& post)
{
	std::unique_lock<std::shared_mutex> locker(m_mutex);
	std::string Topic = post.getTopicId();
	std::string Messg = post.getMessage();
	if (Messg.size() > maxL)
	{
		Messg = Messg.substr(0, 140);
	}

	if (Topic.size() > maxL)
	{
		Topic = Topic.substr(0, 140);
	}

	if (umap.find(Topic) != umap.end())
	{
		umap[Topic].push_back(Messg);
		return std::to_string(umap[Topic].size() - 1);
	}
	else
	{
		std::vector<std::string> post;
		post.push_back(Messg);
		umap.insert(std::pair<std::string, std::vector<std::string>>(Topic, post));
		return std::to_string(umap[Topic].size() - 1);
	}
	return std::to_string(0);
}

 std::string handle_Reads(ReadRequest& read)
{
	std::shared_lock<std::shared_mutex> locker(m_mutex);
	std::string Topic = read.getTopicId();
	if (Topic.size() <= maxL)
	{
		if (umap.find(Topic) != umap.end())
		{
			auto temp = umap[read.getTopicId()];
			if (read.getPostId() < temp.size())
			{
				return umap[Topic][read.getPostId()];
			}

		}
	}
	else
	{
		Topic = Topic.substr(0, 140);

		if (umap.find(Topic) != umap.end())
		{
			auto temp = umap[Topic];
			if (read.getPostId() < temp.size())
			{
				return umap[Topic][read.getPostId()];
			}
		}

	}
	return "";
}

std::string handle_List()
{
	std::shared_lock<std::shared_mutex> locker(m_mutex);
	std::vector<std::string> key;
	std::string temp;
	for (std::unordered_map<std::string, std::vector<std::string>>::iterator it = umap.begin(); it != umap.end(); ++it) {
		key.push_back(it->first);
	}
	for (int i = 0; i < key.size(); i++)
	{
		temp += key[i] + "#";
	}
	return temp;
}


std::string handle_Count(CountRequest& count)
{
	std::shared_lock<std::shared_mutex> locker(m_mutex);
	if (count.getTopicId().size() <= maxL)
	{
		if (umap.find(count.getTopicId()) != umap.end())
		{
			return std::to_string(umap[count.getTopicId()].size());
		}
	}
	return "0";
}


std::string parseRequest(std::string request) {
	PostRequest post = PostRequest::parse(request);
	if (post.valid) { return handle_posts(post); }

	ReadRequest read = ReadRequest::parse(request);
	if (read.valid) { return handle_Reads(read); }

	CountRequest count = CountRequest::parse(request);
	if (count.valid) { return handle_Count(count); }

	ListRequest list = ListRequest::parse(request);
	if (list.valid) { return handle_List(); }

	ExitRequest exitReq = ExitRequest::parse(request);
	if (exitReq.valid) { return exitReq.toString(); }

	std::cout << "Unknown request: " << request << std::endl;
	std::cout << std::endl;
	return "";
}




void serverThreadFunction(TCPServer* server, ReceivedSocketData && data)
{
	unsigned int socketIndex = data.ClientSocket;

	do {
		server->receiveData(data, 0);

		if (data.request != "" && data.request != "EXIT")
		{
			data.reply = parseRequest(data.request);
			server->sendReply(data);
		}
		else if (data.request == "EXIT")
		{
			data.reply = data.request;
			server->sendReply(data);
		}
	} while (data.request != "EXIT" && !terminateServer);

	if (!terminateServer && data.request == "EXIT")
	{
		terminateServer = true;

		TCPClient tempClient(std::string("127.0.0.1"), DEFAULT_PORT);
		tempClient.OpenConnection();
		tempClient.CloseConnection();
	}

	server->closeClientSocket(data);
}