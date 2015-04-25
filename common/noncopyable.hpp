#pragma once

namespace sprawl
{
	class noncopyable;
}

class sprawl::noncopyable
{
public:
	noncopyable() {}
private:
	noncopyable(const noncopyable& other);
	noncopyable& operator=(const noncopyable& other);
};
