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
	noncopyable(noncopyable const& other);
	noncopyable& operator=(noncopyable const& other);
};
