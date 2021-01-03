#include <iostream>
#include <algorithm>
#include <string>
#include <thread>
#include <vector>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <iterator>
#include <unordered_map>
#include <shared_mutex>


#include "config.h"
#include "TCPServer.h"
#include "RequestParser.h"
#include "TCPClient.h"
#include "Posts.h"


std::mutex mu;
std::shared_mutex mu2;

int maxL = 140;

#define DEFAULT_PORT 12345

bool terminateServer = false;

void serverThreadFunction(TCPServer server, ReceivedSocketData && data);
std::string parseRequest(std::string request);
std::unordered_map<std::string, std::vector<std::string>> umap;
std::string handle_posts(PostRequest& post);
std::string handle_Reads(ReadRequest& read);

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
			serverThreads.emplace_back(serverThreadFunction, server, receivedData);
		}
	}

	for (auto& th : serverThreads)
		th.join();

	std::cout << "Server terminated." << std::endl;

	return 0;
}

std::string handle_posts(PostRequest& post)
{
	//std::lock_guard<std::mutex> locker(mu2);
	std::unique_lock<std::shared_mutex> locker(mu2);

	if (post.getMessage().size() <= maxL && post.getTopicId().size() <= maxL)
	{
		if (umap.find(post.getTopicId()) != umap.end())
		{
			umap[post.getTopicId()].push_back(post.getMessage());
			return std::to_string(umap[post.getTopicId()].size());
		}
		else
		{
			Posts P(post);
			P.addPost();
			umap.insert(std::pair<std::string, std::vector<std::string>>(P.tempPost.getTopicId(), P.post));
			return std::to_string(umap[P.tempPost.getTopicId()].size());
		}
	}
	
	return std::to_string(0);
}

std::string handle_Reads(ReadRequest& read)
{
	//std::lock_guard<std::mutex> locker(mu2);
	std::shared_lock<std::shared_mutex> locker(mu2);
	if (read.getTopicId().size() <= maxL)
	{
		if (umap.find(read.getTopicId()) != umap.end())
		{
			auto temp = umap[read.getTopicId()];
			if (read.getPostId() < temp.size())
			{
				return umap[read.getTopicId()][read.getPostId()];
			}
			
		}
	}
	return "";
}

std::string parseRequest(std::string request) {
	PostRequest post = PostRequest::parse(request);
	if (post.valid) { return handle_posts(post); }

	ReadRequest read = ReadRequest::parse(request);
	if (read.valid) { return handle_Reads(read); }

	/*CountRequest count = CountRequest::parse(request);
	if (count.valid) { return count.getTopicId(); }

	ListRequest list = ListRequest::parse(request);
	if (list.valid) { return list.toString(); }*/

	ExitRequest exitReq = ExitRequest::parse(request);
	if (exitReq.valid) { return exitReq.toString(); }

	std::cout << "Unknown request: " << request << std::endl;
	std::cout << std::endl;
	return "";
}

//std::string parseRequest(std::string request) {
//	PostRequest post = PostRequest::parse(request);
//	if (post.valid) { return post.getMessage(); }
//
//	ReadRequest read = ReadRequest::parse(request);
//	if (read.valid) { return read.getTopicId(); }
//
//	CountRequest count = CountRequest::parse(request);
//	if (count.valid) { return count.getTopicId(); }
//
//	ListRequest list = ListRequest::parse(request);
//	if (list.valid) { return list.toString(); }
//
//	ExitRequest exitReq = ExitRequest::parse(request);
//	if (exitReq.valid) { return exitReq.toString(); }
//
//	std::cout << "Unknown request: " << request << std::endl;
//	std::cout << std::endl;
//}

void serverThreadFunction(TCPServer server, ReceivedSocketData && data)
{
	unsigned int socketIndex = data.ClientSocket;

	do {
		server.receiveData(data, 0);

		if (data.request != "" && data.request != "EXIT" )
		{
			data.reply = parseRequest(data.request);
			server.sendReply(data);
		}
		else if (data.request == "EXIT")
		{
			data.reply = data.request;
			server.sendReply(data);
		}
	} while (data.request != "EXIT" && !terminateServer);

	if (!terminateServer && data.request == "EXIT")
	{
		terminateServer = true;

		TCPClient tempClient(std::string("127.0.0.1"), DEFAULT_PORT);
		
		tempClient.OpenConnection();
		tempClient.CloseConnection();
	}

	server.closeClientSocket(data);
}