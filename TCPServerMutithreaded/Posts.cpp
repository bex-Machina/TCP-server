#include "Posts.h"

void Posts::addPost()
{
	//std::unique_lock<std::mutex> locker(m_mutex);
	post.push_back(tempPost.getMessage());
}
