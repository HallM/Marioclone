#pragma once

#include <cassert>
#include <algorithm>
#include <iostream>
#include <optional>
#include <unordered_map>
#include <vector>

template <typename K, typename V>
class SparseHashmap {
public:
    SparseHashmap() {
	}
    SparseHashmap(size_t capacity) {
		_keys.reserve(capacity);
		_values.reserve(capacity);
	}
    SparseHashmap(const SparseHashmap& to_copy) {
		_key_to_index = to_copy._key_to_index;
		_keys = to_copy._keys;
		_values = to_copy._values;
	}

    ~SparseHashmap() {}

	SparseHashmap& operator=(const SparseHashmap& to_copy) {
		_key_to_index = to_copy._key_to_index;
		_keys = to_copy._keys;
		_values = to_copy._values;
		return *this;
	}

	std::vector<K>::iterator keys() {
		return _keys.begin();
	}
	size_t size() const {
		return _keys.size();
	}

	bool has(const K& key) const {
		return _key_to_index.find(key) != _key_to_index.end();
	}
	size_t index_of(const K& key) const {
		return _key_to_index.at(key);
	}
	std::optional<size_t> find_index_of(K& key) {
		auto i = _key_to_index.find(key);
		if (i == _key_to_index.end()) {
			return {};
		}
		return i->second;
	}

	const K& key_at(size_t index) const {
		return _keys.at(index);
	}
	V& value_at(size_t index) {
		return _values.at(index);
	}
	V& value_of(K& key) {
		size_t index = _key_to_index.at(key);
		return _values.at(index);
	}

	void add(const K& key, V& value) {
		size_t index = _keys.size();
		_keys.push_back(key);
		_values.push_back(std::move(value));
		_key_to_index[key] = index;
	}
	template <typename... Args>
	void emplace(K& key, Args&&... args) {
		size_t index = _keys.size();
		_keys.push_back(key);
		_values.emplace_back(std::forward<Args>(args)...);
		_key_to_index[key] = index;
	}

	void remove(K& key) {
		size_t index = _key_to_index[key];
		size_t last = _keys.size() - 1;
		K& swap_key = _keys[last];

		// set the index first since swap_key changes (its a ref) after the swap.
		_key_to_index[swap_key] = index;
		// swap the back with the item deleted before removing entries.
		std::swap(_keys[index], _keys[last]);
		std::swap(_values[index], _values[last]);

		_keys.pop_back();
		_values.pop_back();
		_key_to_index.erase(key);
	}

	void apply_sort(const std::vector<size_t>& indices) {
		_apply_vec_sort(_keys, indices);
		_apply_vec_sort(_values, indices);
		for (size_t i = 0; i < _keys.size(); i++) {
			_key_to_index[_keys[i]] = i;
		}
	}

private:
	template <typename V>
	void _apply_vec_sort(std::vector<V>& tosort, const std::vector<size_t>& indices) {
		std::vector<bool> sorted(tosort.size());
		for (size_t i = 0; i < tosort.size(); i++) {
			if (sorted[i]) { continue; }
			sorted[i] = true;
			size_t prev = i;
			size_t j = indices[prev];
			while (i != j) {
				std::swap(tosort[prev], tosort[j]);
				sorted[j] = true;
				prev = j;
				j = indices[prev];
			}
		}
	}

	std::unordered_map<K, size_t> _key_to_index;
	std::vector<K> _keys;
	std::vector<V> _values;
};
