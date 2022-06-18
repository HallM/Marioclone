#pragma once

#include <cassert>
#include <iostream>
#include <functional>
#include <optional>
#include <tuple>
#include <typeindex>
#include <typeinfo> 
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "BufferVector.h"
#include "timsort.hpp"

namespace MattECS {
	typedef size_t EntityID;

	class IComponentContainer {
	public:
		virtual ~IComponentContainer() = default;
		virtual void update_all() = 0;
		virtual void delete_item(EntityID id) = 0;
	};

	template <typename C>
	void no_orderer(std::vector<size_t>& indices, const std::vector<C>& values) {
	}
	template <typename C, typename bool(*Less)(const C&, const C&)>
	void less_than_orderer(std::vector<size_t>& indices, const std::vector<C>& values) {
		gfx::timsort(
			indices,
			Less,
			[&values](size_t index) -> const C& {
				return values.at(index);
			}
		);
	}

	template <typename C>
	void no_change_handler(EntityID id, const C& obj) {
	}

	template <
		typename C,
		typename void(*Orderer)(std::vector<size_t>&, const std::vector<C>&) = no_orderer<C>,
		typename void(*OnChange)(EntityID, const C&) = no_change_handler<C>
	>
	class ComponentContainer : public IComponentContainer {
	public:
		class iterator {
			size_t _index = 0;
			ComponentContainer<C>* _c = nullptr;
		public:
			explicit iterator(ComponentContainer<C>* c, size_t i) : _c(c), _index(i) {}
			bool is_end() const { return _index >= _c->_ids.size(); }
			bool has_next() const { return (_index + 1) < _c->_ids.size(); }
			iterator& operator++() { _index++; return *this; }
			iterator operator++(int) { iterator it(_c, _index); _index++; return it; }
			iterator& operator--() { _index--; return *this; }
			iterator operator--(int) { iterator it(_c, _index); _index--; return it; }
			bool operator==(iterator other) const { return (is_end() && other.is_end()) || _index == other._index; }
			bool operator!=(iterator other) const { return !((is_end() && other.is_end()) || _index == other._index); }
			EntityID entity() const { return _c->_ids[_index]; }
			C& value() const { return _c->_values.at(_index); }
			std::optional<size_t> index() const {
				if (is_end()) { return {}; }
				return _index;
			}
		};

		ComponentContainer(size_t entities) : _changed(false), _values() {
			_values.reserve(entities);
		}
		virtual ~ComponentContainer() {}

		iterator begin() { return iterator(this, 0); }
		iterator end() { return iterator(this, _ids.size()); }
		iterator find(EntityID id) {
			auto it = _id_to_index.find(id);
			if (it == _id_to_index.end()) {
				return end();
			}
			size_t index = it->second;
			return iterator(this, index);
		}

		bool has(EntityID id) const {
			return _id_to_index.find(id) != _id_to_index.end();
		}

		const C& cvalue(EntityID id) {
			auto index = _id_to_index[id];
			return _values.at(index);
		}
		C& value(EntityID id) {
			auto index = _id_to_index[id];
			set_changed(index);
			return _values.at(index);
		}
		const C& cvalue_or(EntityID id, const C& vor) {
			auto i = _id_to_index.find(id);
			if (i == _id_to_index.end()) {
				return vor;
			}
			return _values.at(i->second);
		}
		C& value_or(EntityID id, C& vor) {
			auto i = _id_to_index.find(id);
			if (i == _id_to_index.end()) {
				return vor;
			}
			auto index = i->second;
			set_changed(index);
			return _values.at(index);
		}

		C& at(size_t index) {
			return _values.at(index);
		}

		virtual void delete_item(EntityID id) {
			if (_id_to_index.find(id) != _id_to_index.end()) {
				_changed = true;
				_deleted_items.insert(id);
			}
		}
		template <typename... Args>
		void add_item(EntityID id, Args&&... args) {
			if (_id_to_index.find(id) == _id_to_index.end()) {
				_changed = true;
				_new_items.emplace(std::piecewise_construct,
					std::forward_as_tuple(id), std::forward_as_tuple(args...));
			}
		}

		void set_changed(size_t index) {
			_changed = true;
			_dirty[index] = true;
		}

		virtual void update_all() {
			for (auto id : _deleted_items) {
				unsigned int index = _id_to_index[id];
				unsigned int last = _ids.size() - 1;
				EntityID swap_with = _ids[last];

				// swap the back with the item deleted before removing entries.
				std::swap(_ids[index], _ids[last]);
				std::swap(_values[index], _values[last]);
				_dirty[index] = _dirty[last];
				_id_to_index[swap_with] = index;

				_ids.pop_back();
				_values.pop_back();
				_dirty.pop_back();
				_id_to_index.erase(id);
			}
			_deleted_items.clear();

			for (auto& item : _new_items) {
				unsigned int index = _ids.size();
				auto id = item.first;
				_ids.push_back(id);
				//_values.push_back(item.second);
				_values.push_back(std::move(item.second));
				//new (_values.typed_ptr<C>(index)) C(std::move(item.second));
				_dirty.push_back(true);
				_id_to_index[id] = index;
			}
			_new_items.clear();

			if (_changed) {
				if constexpr (OnChange != no_change_handler) {
					for (size_t i = 0; i < _dirty.size(); i++) {
						_dirty[i] = false;
						OnChange(_ids[i], _values.at(i));
					}
				}
				if constexpr (Orderer != no_orderer) {
					_sort();
				}
				_changed = false;
			}
		}

	private:
		void _sort() {
			std::vector<size_t> indices(_ids.size());
			for (size_t i = 0; i < _ids.size(); i++) {
				indices[i] = i;
			}
			if constexpr (Orderer != no_orderer) {
				Orderer(indices, _values);
			}
			_apply_vec_sort<EntityID>(_ids, indices);
			// _apply_sort<C>(_values, indices);
			_apply_value_sort(indices);

			for (unsigned int i = 0; i < _ids.size(); i++) {
				_id_to_index[_ids[i]] = i;
			}
		}

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
		void _apply_value_sort(const std::vector<size_t>& indices) {
			std::vector<bool> sorted(_values.size());
			for (size_t i = 0; i < _values.size(); i++) {
				if (sorted[i]) { continue; }
				sorted[i] = true;
				size_t prev = i;
				size_t j = indices[prev];
				while (i != j) {
					//_values.swap(prev, j);
					std::swap(_values[prev], _values[j]);
					sorted[j] = true;
					prev = j;
					j = indices[prev];
				}
			}
		}

		// _changed is set if any are dirty
		bool _changed;
		// track which elements changed or at least what mut()s were called.
		std::vector<bool> _dirty;

		std::unordered_map<EntityID, unsigned int> _id_to_index;
		std::vector<EntityID> _ids;
		std::vector<C> _values;

		std::unordered_map<EntityID, C> _new_items;
		std::unordered_set<EntityID> _deleted_items;
	};
}

