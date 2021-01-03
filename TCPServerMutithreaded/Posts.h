#pragma once
#include <mutex>
#include <vector>
#include "RequestParser.h"

class Posts
{
public:
	std::mutex m_mutex;
	PostRequest tempPost;
	std::vector<std::string> post;

public:
	Posts(PostRequest& P)
	{
		tempPost = P;
	} 

	PostRequest getTempPost()
	{
		return tempPost;
	}

	void addPost();
	
};

