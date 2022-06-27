#pragma once

#include <cassert>
#include <algorithm>
#include <iostream>
#include <functional>
#include <optional>
#include <tuple>
#include <typeindex>
#include <typeinfo> 
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "SparseHashmap.h"
#include "timsort.hpp"

namespace MattECS {
	typedef size_t EntityID;

	class IComponentContainer {
	public:
		virtual ~IComponentContainer() = default;
		virtual void end_frame() = 0;
		virtual void update_all() = 0;
		virtual void delete_item(EntityID id) = 0;
	};

	template <typename C>
	void no_orderer(std::vector<size_t>& indices, SparseHashmap<EntityID, C>& values) {
	}
	template <typename C, typename bool(*Less)(const C&, const C&)>
	void less_than_orderer(std::vector<size_t>& indices, SparseHashmap<EntityID, C>& data) {
		gfx::timsort(
			indices,
			Less,
			[&data](size_t index) -> const C& {
				return data.value_at(index);
			}
		);
	}

	template <typename C>
	void no_change_handler(EntityID id, const C& obj) {
	}

	template <
		typename C,
		typename void(*Orderer)(std::vector<size_t>&, SparseHashmap<EntityID, C>&) = no_orderer<C>,
		typename void(*OnChange)(EntityID, const C&) = no_change_handler<C>
	>
	class ComponentContainer : public IComponentContainer {
	public:
		class iterator {
			size_t _index = 0;
			ComponentContainer<C>* _c = nullptr;
		public:
			explicit iterator(ComponentContainer<C>* c, size_t i) : _c(c), _index(i) {}
			bool is_end() const { return _index >= _c->_data.size(); }
			bool has_next() const { return (_index + 1) < _c->_data.size(); }
			iterator& operator++() { _index++; return *this; }
			iterator operator++(int) { iterator it(_c, _index); _index++; return it; }
			iterator& operator--() { _index--; return *this; }
			iterator operator--(int) { iterator it(_c, _index); _index--; return it; }
			bool operator==(iterator other) const { return (is_end() && other.is_end()) || _index == other._index; }
			bool operator!=(iterator other) const { return !((is_end() && other.is_end()) || _index == other._index); }
			EntityID entity() const { return _c->_data.key_at(_index); }
			C& value() const { return _c->_data.value_at(_index); }
			std::optional<size_t> index() const {
				if (is_end()) { return {}; }
				return _index;
			}
		};

		ComponentContainer(size_t entities) : _changed(false), _data(entities) {
			//_values.reserve(entities);
			//_clones.reserve(entities);
		}
		virtual ~ComponentContainer() {}

		iterator begin() { return iterator(this, 0); }
		iterator end() { return iterator(this, _data.size()); }
		iterator find(EntityID id) {
			auto maybe_index = _data.find_index_of(id);
			if (!maybe_index) {
				return end();
			}
			size_t index = maybe_index.value();
			return iterator(this, index);
		}

		bool has(EntityID id) const {
			return _data.has(id);
		}

		const C& cvalue(EntityID id) {
			return _data.value_of(id);
		}
		C& value(EntityID id) {
			auto index = _data.index_of(id);
			set_changed(index);
			return _data.value_at(index);
		}
		const C& cvalue_or(EntityID id, const C& vor) {
			if (!_data.has(id)) {
				return vor;
			}
			return _data.value_of(id);
		}
		C& value_or(EntityID id, C& vor) {
			if (!_data.has(id)) {
				return vor;
			}
			auto index = _data.index_of(id);
			set_changed(index);
			return _data.value_at(index);
		}

		C& at(size_t index) {
			return _data.value_at(index);
		}

		virtual void delete_item(EntityID id) {
			if (_data.has(id)) {
				_changed = true;
				_deleted_items.insert(id);
			}
		}
		template <typename... Args>
		void add_item(EntityID id, Args&&... args) {
			if (!_data.has(id)) {
				_changed = true;
				_new_items.emplace(std::piecewise_construct,
					std::forward_as_tuple(id), std::forward_as_tuple(args...));
			}
		}

		void set_changed(size_t index) {
			_changed = true;
			//_dirty[index] = true;
		}

		virtual void end_frame() {
			_clone = _data;
		}

		virtual void update_all() {
			for (auto id : _deleted_items) {
				_data.remove(id);
			}
			_deleted_items.clear();

			for (auto& item : _new_items) {
				//_data.add(item.first, std::move(item.second));
				_data.add(item.first, item.second);
			}
			_new_items.clear();

			if (_changed) {
				//if constexpr (OnChange != no_change_handler) {
				//	for (size_t i = 0; i < _dirty.size(); i++) {
				//		_dirty[i] = false;
				//		OnChange(_ids[i], _values.at(i));
				//	}
				//}
				if constexpr (Orderer != no_orderer) {
					_sort();
				}
				_changed = false;
			}
		}

	private:
		void _sort() {
			std::vector<size_t> indices(_data.size());
			for (size_t i = 0; i < _data.size(); i++) {
				indices[i] = i;
			}
			if constexpr (Orderer != no_orderer) {
				Orderer(indices, _data);
			}
			_data.apply_sort(indices);
		}

		// _changed is set if any are dirty
		bool _changed;

		// track which elements changed or at least what mut()s were called.
		//std::vector<bool> _dirty;
		// next step: we want a circular buffer of these for frames.
		// if we want a rewind up to 8 fixedupdate frames, we need 8 sparsemaps.
		// then we need a way to query/view a very specific frame, but default to latest.
		SparseHashmap<EntityID, C> _data;
		SparseHashmap<EntityID, C> _clone;

		// just a test
		// std::vector<C> _clones;

		std::unordered_map<EntityID, C> _new_items;
		std::unordered_set<EntityID> _deleted_items;
	};
}

