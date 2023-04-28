#ifndef DEPENDENCIES_H_
#define DEPENDENCIES_H_

#include <memory>

namespace server
{
	struct DataBuffer
	{
		int size;
		void* data_ptr;
		DataBuffer() noexcept : size(0), data_ptr(nullptr) {}
		DataBuffer(int sz, void* ptr) noexcept : size(sz), data_ptr(ptr) {}
		DataBuffer(const DataBuffer& rhs) : size(rhs.size), data_ptr(::operator new(rhs.size))
		{
			memcpy(data_ptr, rhs.data_ptr, rhs.size);
		}
		DataBuffer(DataBuffer&& tmp) noexcept : size(tmp.size), data_ptr(tmp.data_ptr)
		{
			tmp.data_ptr = nullptr;
		}
		~DataBuffer() { if (data_ptr) ::operator delete(data_ptr, size); }

		bool isEmpty() const { return !size || !data_ptr; }

		operator bool() const { return size && data_ptr; }
	};
}

#endif // !DEPENDENCIES_H_
