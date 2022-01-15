#pragma once

#include <iostream>
#include <functional>
#include <optional>
#include <tuple>
#include <typeindex>
#include <typeinfo> 
#include <unordered_map>
#include <vector>

namespace MattECS {
	typedef size_t EntityID;

	template <typename C>
	class ComponentManager {
	public:
		class iterator {
			unsigned int _index = 0;
			ComponentManager<C>* _c = nullptr;
		public:
			explicit iterator(ComponentManager<C>* c, unsigned int i) : _c(c), _index(i) {}
			bool is_end() const { return _index >= _c->_ids.size(); }
			bool has_next() const { return (_index + 1) < _c->_ids.size(); }
			iterator& operator++() { _index++; return *this; }
			iterator operator++(int) { iterator it(_c, _index); _index++; return it; }
			iterator& operator--() { _index--; return *this; }
			iterator operator--(int) { iterator it(_c, _index); _index--; return it; }
			bool operator==(iterator other) const { return (is_end() && other.is_end()) || _index == other._index; }
			bool operator!=(iterator other) const { return !((is_end() && other.is_end()) || _index == other._index); }
			EntityID entity() const { return _c->_ids[_index]; }
			C* value() const { return &_c->_values[_index]; }
		};

		ComponentManager() : _should_sort(false), _changed(false) {}

		iterator begin() { return iterator(this, 0); }
		iterator end() { return iterator(this, _ids.size()); }
		iterator find(EntityID id) {
			auto it = _id_to_index.find(id);
			if (it == _id_to_index.end()) {
				return end();
			}
			unsigned int index = it->second;
			return iterator(this, index);
		}

		const C* cvalue(EntityID id) {
			auto it = _id_to_index.find(id);
			if (it == _id_to_index.end()) {
				return nullptr;
			}
			return &_values[it->second];
		}
		C* value(EntityID id) {
			auto it = _id_to_index.find(id);
			if (it == _id_to_index.end()) {
				return nullptr;
			}
			return &_values[it->second];
		}

		void delete_item(EntityID id) {
			if (_id_to_index.find(id) != _id_to_index.end()) {
				_changed = true;
				_deleted_items.push_back(id);
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

		void set_changed() {
			_changed = true;
		}

		void add_sorter(std::function<bool(const C&, const C&)> less) {
			_less = less;
			_should_sort = true;
			_changed = true;
		}

		void update_all() {
			for (auto id : _deleted_items) {
				unsigned int index = _id_to_index[id];
				unsigned int last = _ids.size() - 1;
				std::swap(_ids[index], _ids[last]);
				std::swap(_values[index], _values[last]);
				_ids.pop_back();
				_values.pop_back();
				_id_to_index.erase(id);
			}
			_deleted_items.clear();

			for (auto& item : _new_items) {
				unsigned int index = _ids.size();
				auto id = item.first;
				_ids.push_back(id);
				_values.push_back(item.second);
				_id_to_index[id] = index;
			}
			_new_items.clear();

			if (_should_sort && _changed) {
				_sort();
			}
			_changed = false;
		}

	private:
		void _sort() {
			// TODO: figure out a sort that doesnt require a copy of _values
			std::vector<EntityID> copy_ids(_ids);
			std::vector<C> copy_values(_values);
			_merge_sort(copy_ids, copy_values, _ids.size());
			for (unsigned int i = 0; i < _ids.size(); i++) {
				_id_to_index[_ids[i]] = i;
			}
		}

		void _merge_sort(std::vector<EntityID>& copy_ids, std::vector<C>& copy_values, unsigned int len) {
			for (unsigned int w = 1; w < len; w = 2 * w) {
				for (unsigned int i = 0; i < len; i = i + 2 * w) {
					unsigned int right = i + w;
					if (len < right) {
						right = len;
					}
					unsigned int end = i + 2 * w;
					if (len < end) {
						end = len;
					}
					_merge(copy_ids, copy_values, i, right, end);
				}
				for (unsigned int i = 0; i < len; i++) {
					_ids[i] = copy_ids[i];
					_values[i] = copy_values[i];
				}
			}
		}

		void _merge(std::vector<EntityID>& copy_ids, std::vector<C>& copy_values, unsigned int left, unsigned int right, unsigned int end) {
			unsigned int i = left;
			unsigned int j = right;
			for (unsigned int k = left; k < end; k++) {
				if (i < right && (j >= end || _less(_values[i], _values[j]))) {
					copy_values[k] = _values[i];
					copy_ids[k] = _ids[i];
					i++;
				}
				else {
					copy_values[k] = _values[j];
					copy_ids[k] = _ids[j];
					j++;
				}
			}
		}

		bool _should_sort;
		std::function<bool(const C&, const C&)> _less;

		bool _changed;

		std::unordered_map<EntityID, unsigned int> _id_to_index;
		std::vector<EntityID> _ids;
		std::vector<C> _values;

		std::unordered_map<EntityID, C> _new_items;
		std::vector<EntityID> _deleted_items;
	};

	template <typename... Components>
	class TemplateEntityManager {
	public:
		template <typename CFirst, typename... COthers>
		class Querier {
		public:
			class iterator {
			public:
				iterator(ComponentManager<CFirst>::iterator it, ComponentManager<CFirst>::iterator end, std::tuple<ComponentManager<CFirst>*, ComponentManager<COthers>*...>& cmanagers) : _it(it), _end(end), _cmanagers(cmanagers) {
					_set_values();
					if (!_has_all && _it != _end) {
						_next();
					}
				}

				// prefix
				iterator& operator++() {
					_next();
					return *this;
				}
				// postfix
				iterator operator++(int) {
					iterator it(_it, _end, _values);
					_it++;
					return it;
				}

				bool operator==(iterator other) const { return _it == other._it; }
				bool operator!=(iterator other) const { return _it != other._it; }

				EntityID entity() const { return _it.entity(); }
				std::tuple<CFirst*, COthers*...>& values() {
					return _values;
				}
			private:
				iterator(ComponentManager<CFirst>::iterator it, ComponentManager<CFirst>::iterator end, std::tuple<ComponentManager<CFirst>*, ComponentManager<COthers>*...>& cmanagers, std::tuple<CFirst*, COthers*...>& v) : _it(it), _end(end), _cmanagers(cmanagers), _values(v), _has_all(true) {
					// assume that this is a legit point
				}

				void _next() {
					do {
						_it++;
						_set_values();
					} while (!_has_all && _it != _end);
				}
				void _set_values() {
					if (_it == _end) {
						return;
					}
					auto id = _it.entity();
					_has_all = true;
					_values = std::make_tuple(
						_it.value(),
						std::get<ComponentManager<COthers>*>(_cmanagers)->value(id)...
					);
					if (_it.value() == nullptr) {
						_has_all = false;
						return;
					}
					bool isNulls[sizeof...(COthers)] = { (std::get<COthers*>(_values) == nullptr)... };
					for (unsigned int i = 0; i < sizeof...(COthers); i++) {
						if (isNulls[i]) {
							_has_all = false;
							break;
						}
					}
				}
				bool _has_all;
				std::tuple<CFirst*, COthers*...> _values;
				ComponentManager<CFirst>::iterator _it;
				ComponentManager<CFirst>::iterator _end;
				std::tuple<ComponentManager<CFirst>*, ComponentManager<COthers>*...> _cmanagers;
			};
			
			Querier(std::tuple<ComponentManager<CFirst>*,ComponentManager<COthers>*...> managers) : _cmanagers(managers) {}

			iterator begin() {
				auto cm = std::get<ComponentManager<CFirst>*>(_cmanagers);
				return iterator(cm->begin(), cm->end(), _cmanagers);
			}
			iterator end() {
				auto cm = std::get<ComponentManager<CFirst>*>(_cmanagers);
				return iterator(cm->end(), cm->end(), _cmanagers);
			}
			iterator find(EntityID id) {
				auto cm = std::get<ComponentManager<CFirst>*>(_cmanagers);
				return iterator(cm->find(id), cm->end(), _cmanagers);
			}
		private:
			std::tuple<ComponentManager<CFirst>*, ComponentManager<COthers>*...> _cmanagers;
		};

		TemplateEntityManager() {
			_last_id = 0;

			register_component<Components...>();
		}

		template <typename CFirst, typename... COthers>
		Querier<CFirst, COthers...> query() {
			return Querier(
				std::make_tuple(
					_manager<CFirst>(),
					_manager<COthers>()...
				)
			);
		}

		template <typename C>
		void register_component() {
			auto ti = std::type_index(typeid(C));
			ComponentManager<C>* new_cm = new ComponentManager<C>();
			_idautomanagers[ti] = (void*)new_cm;
		}
		template <typename C, typename Second, typename... Others>
		void register_component() {
			register_component<C>();
			register_component<Second, Others...>();
		}

		~TemplateEntityManager() {
			for (auto& it : _idautomanagers) {
				delete it.second;
			}
			for (size_t i = 0; i < _automanagers.size(); i++) {
				delete _automanagers[i];
			}
		}

		EntityID entity() {
			EntityID new_id = _last_id++;
			return new_id;
		}

		template <typename C>
		void sort(std::function<bool(const C&, const C&)> less) {
			auto c = _manager<C>();
			c->add_sorter(less);
		}

		template <typename C>
		const C* get(EntityID id) {
			auto c = _manager<C>();
			auto it = c->find(id);
			if (it == c->end()) {
				return nullptr;
			}
			return it.value();
		}
		template <typename CFirst, typename CSecond, typename... COthers>
		std::tuple<const CFirst*, const CSecond*, const COthers*...> get(EntityID id) {
			return std::make_tuple(get<CFirst>(id), get<CSecond>(id), get<COthers>(id)...);
		}

		// Much like getComponents() except the first part is bool to let you know if ALL are there.
		template <typename... C>
		std::optional<std::tuple<const C*...>> require(EntityID id) {
			auto v = std::make_tuple(get<C>(id)...);
			bool isNulls[sizeof...(C)] = { (std::get<const C*>(v) == nullptr)... };
			bool anyNull = false;
			for (unsigned int i = 0; i < sizeof...(C); i++) {
				if (isNulls[i]) {
					return {};
				}
			}
			return v;
		}

		template <typename C, typename... Args>
		void add(EntityID id, Args&&... args) {
			auto c = _manager<C>();
			c->add_item<Args...>(id, std::forward<Args>(args)...);
		}

		template <typename C>
		void remove(EntityID id) {
			auto c = _manager<C>();
			c->delete_item(id);
		}
		template <typename C, typename Second, typename... Others>
		void remove(EntityID id) {
			remove<C>(id);
			remove<Second, Others...>(id);
		}

		void remove_all(EntityID id) {
			remove<Components...>(id);
		}

		template <typename C>
		void update(EntityID id, std::function<void(C*)> callable) {
			auto v = get_mut<C>(id);
			callable(v);
		}
		template <typename CFirst, typename... COthers>
		void update(EntityID id, std::function<void(CFirst*, COthers*...)> callable) {
			auto v = get_mut<CFirst, COthers...>(id);
			callable(std::get<CFirst*>(v), std::get<COthers*>(v)...);
		}

		// finalize_update should be called when no iterators are held
		// to avoid invalidating iterators. This will finalize added/removed
		// components and entities.
		void finalize_update() {
			_finalize_component<Components...>();
		}

		// Try to order Cs from smallest to largest.
		template <typename CFirst, typename... COthers>
		void forEach(std::function<void(EntityID, const CFirst*, const COthers*...)> callable) {
			auto wrapped = [this, &callable](EntityID id, const CFirst* v1, const COthers*... vrest) -> bool {
				callable(id, v1, vrest...);
				return true;
			};
			forUntil<CFirst, COthers...>(wrapped);
		}
		template <typename C>
		void forEach(std::function<void(EntityID, const C*)> callable) {
			auto wrapped = [this, &callable](EntityID id, const C* v) -> bool {
				callable(id, v);
				return true;
			};
			forUntil<C>(wrapped);
		}

		template <typename CFirst, typename... COthers>
		void forUntil(std::function<bool(EntityID, const CFirst*, const COthers*...)> callable) {
			auto c = _manager<CFirst>();
			for (auto it = c->begin(); it != c->end(); it++) {
				EntityID id = it.entity();
				const CFirst* elem1 = it.value();

				auto maybe = require<COthers...>(id);
				if (maybe) {
					auto v = maybe.value();
					if (!callable(id, elem1, std::get<const COthers*>(v)...)) {
						return;
					}
				}
			}
		}
		template <typename CFirst>
		void forUntil(std::function<bool(EntityID, const CFirst*)> callable) {
			auto c = _manager<CFirst>();
			for (auto it = c->begin(); it != c->end(); it++) {
				EntityID id = it.entity();
				const CFirst* elem1 = it.value();
				if (!callable(id, elem1)) {
					return;
				}
			}
		}

		// In the callback, return false when we shouldnt keep searching in that direction.
		// The items to the left are done first, then the right.
		// The start element is included as the first entity
		template <typename CFirst, typename... COthers>
		void forAround(EntityID start, std::function<bool(EntityID, const CFirst*, const COthers*...)> callable) {
			auto c = _manager<CFirst>();
			for (auto it = c->find(start); it != c->begin(); it--) {
				EntityID id = it.entity();
				const CFirst* elem1 = it.value();

				auto maybe = require<COthers...>(id);
				if (maybe) {
					auto v = maybe.value();
					if (!callable(id, elem1, std::get<const COthers*>(v)...)) {
						break;
					}
				}
			}
			auto start_it = c->find(start);
			for (auto it = ++start_it; it != c->end(); it++) {
				EntityID id = it.entity();
				const CFirst* elem1 = it.value();

				auto maybe = require<COthers...>(id);
				if (maybe) {
					auto v = maybe.value();
					if (!callable(id, elem1, std::get<const COthers*>(v)...)) {
						break;
					}
				}
			}
		}
		template <typename CFirst>
		void forAround(EntityID start, std::function<bool(EntityID, const CFirst*)> callable) {
			auto c = _manager<CFirst>();
			for (auto it = c->find(start); it != c->begin(); it--) {
				EntityID id = it.entity();
				const CFirst* elem1 = it.value();
				if (!callable(id, elem1)) {
					break;
				}
			}
			auto start_it = c->find(start);
			for (auto it = ++start_it; it != c->end(); it++) {
				EntityID id = it.entity();
				const CFirst* elem1 = it.value();
				if (!callable(id, elem1)) {
					break;
				}
			}
		}

		// Try to order Cs from smallest to largest.
		template <typename Value, typename CFirst, typename... COthers>
		Value reduce(std::function<Value(Value, EntityID, const CFirst*, const COthers*...)> callable, Value start) {
			Value cur = start;
			auto c = _manager<CFirst>();
			for (auto it = c->begin(); it != c->end(); it++) {
				EntityID id = it.entity();
				const CFirst* elem1 = it.value();

				auto maybe = requireComponents<COthers...>(id);
				if (maybe) {
					auto v = maybe.value();
					cur = callable(cur, id, elem1, std::get<const COthers*>(v)...);
				}
			}
			return cur;
		}
		template <typename Value, typename CFirst>
		Value reduce(std::function<Value(Value, EntityID, const CFirst*)> callable, Value start) {
			Value cur = start;
			auto c = _manager<CFirst>();
			for (auto it = c->begin(); it != c->end(); it++) {
				EntityID id = it.entity();
				const CFirst* elem1 = it.value();
				cur = callable(cur, id, elem1);
			}
			return cur;
		}
	private:
		template <typename C>
		ComponentManager<C>* _manager() {
			// return &std::get<ComponentManager<C>>(_cmanagers);

			auto ti = std::type_index(typeid(C));
			return (ComponentManager<C>*)_idautomanagers[ti];
			//auto it = _idautomanagers.find(ti);
			//if (it != _idautomanagers.end()) {
			//	return ((ComponentManager<C>*)it->second);
			//}
			//ComponentManager<C>* new_cm = new ComponentManager<C>();
			//_idautomanagers[ti] = (void*)new_cm;
			//return new_cm;

			//auto it = _amindex.find(typeid(C));
			//if (it != _amindex.end()) {
			//	return ((ComponentManager<C>*)_automanagers[it->second]);
			//}
			//ComponentManager<C>* new_cm = new ComponentManager<C>();
			//size_t index = _automanagers.size();
			//_automanagers.push_back((void*)new_cm);
			//_amindex[typeid(C)] = index;
			//return new_cm;
		}

		template <typename C>
		C* get_mut(EntityID id) {
			auto c = _manager<C>();
			c->set_changed();
			auto it = c->find(id);
			if (it == c->end()) {
				return nullptr;
			}
			return it.value();
		}
		template <typename CFirst, typename CSecond, typename... COthers>
		std::tuple<CFirst*, CSecond*, COthers*...> get_mut(EntityID id) {
			return std::make_tuple(get_mut<CFirst>(id), get_mut<CSecond>(id), get_mut<COthers>(id)...);
		}

		template <typename C>
		void _finalize_component() {
			auto c = _manager<C>();
			c->update_all();
		}
		template <typename C, typename Second, typename... Others>
		void _finalize_component() {
			_finalize_component<C>();
			_finalize_component<Second, Others...>();
		}

		EntityID _last_id;
		std::tuple<ComponentManager<Components>...> _cmanagers;
		std::unordered_map<std::type_index, void*> _idautomanagers;
		std::unordered_map<std::type_index, size_t> _amindex;
		std::vector<void*> _automanagers;
	};
};
