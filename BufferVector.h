#pragma once

#include <cassert>
#include <memory>

class DumbestVector {
public:
	DumbestVector(size_t element_bytes, size_t start_count) : _element_bytes(element_bytes), _capacity(start_count), _count(0) {
		// we keep a +1 just for swaps
		size_t bytes = element_bytes * (start_count + 1);
		_data = (char*)malloc(bytes);
	}

	~DumbestVector() {
		free(_data);
		_data = nullptr;
	}

	void push_back() { assert(_count < _capacity); _count++; }
	void pop_back() { assert(_count > 0); _count--; }

	void swap(size_t i1, size_t i2) {
		auto a = get_ptr(i1);
		auto b = get_ptr(i2);
		auto tmp = get_ptr(_capacity);
		memcpy(tmp, a, _element_bytes);
		memcpy(a, b, _element_bytes);
		memcpy(b, tmp, _element_bytes);
	}
	void move(size_t dest, size_t src) {
		auto d = get_ptr(dest);
		auto s = get_ptr(src);
		memcpy(d, s, _element_bytes);
	}

	size_t size() const { return _count; }
	size_t capacity() const { return _capacity; }
	char* get_ptr(size_t index) { return _data + (index * _element_bytes); }

	template <typename T>
	T* typed_ptr(size_t index) {
		char* ptr = get_ptr(index);
		return reinterpret_cast<T*>(ptr);
	}
	template <typename T>
	T& typed_ref(size_t index) {
		char* ptr = get_ptr(index);
		return *reinterpret_cast<T*>(ptr);
	}

	void resize(size_t new_count) {
		size_t bytes = _element_bytes * (new_count + 1);
		char* new_data = (char*)malloc(bytes);
		memcpy_s(new_data, bytes, _data, _element_bytes * _capacity);
		free(_data);
		_data = new_data;
		_capacity = new_count;
	}
private:
	const size_t _element_bytes;
	size_t _count;
	size_t _capacity;
	char* _data;
};
