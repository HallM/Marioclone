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

	class IComponentManager {
	public:
		virtual ~IComponentManager() = default;
		virtual void update_all() = 0;
		virtual void delete_item(EntityID id) = 0;
	};

	template <typename C>
	class ComponentManager : public IComponentManager {
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
		virtual ~ComponentManager() {}

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

		virtual void delete_item(EntityID id) {
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

		virtual void update_all() {
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

	class EntityManager {
	public:
		template <typename CFirst, typename... COthers>
		class Querier {
		public:
			class iterator {
			public:
				iterator(ComponentManager<CFirst>::iterator it, ComponentManager<CFirst>::iterator end, std::tuple<ComponentManager<CFirst>*, ComponentManager<COthers>*...>& cmanagers, bool is_optional[1 + sizeof...(COthers)]) : _it(it), _end(end), _cmanagers(cmanagers) {
					for (unsigned int i = 1; i < sizeof...(COthers) + 1; i++) {
						_is_optional[i] = is_optional[i];
					}
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
				//// postfix
				//iterator operator++(int) {
				//	iterator it(_it, _end, _values);
				//	++_it;
				//	return it;
				//}

				bool operator==(iterator other) const { return _it == other._it; }
				bool operator!=(iterator other) const { return _it != other._it; }

				EntityID entity() const { return _it.entity(); }
				template <typename C>
				const C* value() {
					return std::get<C*>(_values);
				}
				template <typename C>
				C* mut() {
					std::get<ComponentManager<C>*>(_cmanagers)->set_changed();
					return std::get<C*>(_values);
				}
			private:
				//iterator(ComponentManager<CFirst>::iterator it, ComponentManager<CFirst>::iterator end, std::tuple<ComponentManager<CFirst>*, ComponentManager<COthers>*...>& cmanagers, std::tuple<CFirst*, COthers*...>& v) : _it(it), _end(end), _cmanagers(cmanagers), _values(v), _has_all(true) {
				//	// assume that this is a legit point
				//}

				void _next() {
					do {
						++_it;
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
					bool isNulls[sizeof...(COthers) + 1] = {
						_it.value() == nullptr,
						(std::get<COthers*>(_values) == nullptr)...
					};
					for (unsigned int i = 1; i < sizeof...(COthers) + 1; i++) {
						if (!_is_optional[i] && isNulls[i]) {
							_has_all = false;
							break;
						}
					}
				}
				bool _has_all;
				std::tuple<CFirst*, COthers*...> _values;
				ComponentManager<CFirst>::iterator _it;
				ComponentManager<CFirst>::iterator _end;
				bool _is_optional[1 + sizeof...(COthers)];
				std::tuple<ComponentManager<CFirst>*, ComponentManager<COthers>*...> _cmanagers;
			};

			Querier(std::tuple<ComponentManager<CFirst>*, ComponentManager<COthers>*...> managers) : _cmanagers(managers) {
				for (size_t i = 0; i < 1 + sizeof...(COthers); ++i) {
					_is_optional[i] = false;
				}
			}

			// Setting optional on the first is a NOOP. its not possible, sorry.
			template <typename C>
			Querier& optional() {
				// TODO: fail on setting CFirst as optional
				_is_optional[_cindex<C, CFirst, COthers...>()] = true;
				return *this;
			}

			iterator begin() {
				auto cm = std::get<ComponentManager<CFirst>*>(_cmanagers);
				return iterator(cm->begin(), cm->end(), _cmanagers, _is_optional);
			}
			iterator end() {
				auto cm = std::get<ComponentManager<CFirst>*>(_cmanagers);
				return iterator(cm->end(), cm->end(), _cmanagers, _is_optional);
			}
			iterator find(EntityID id) {
				auto cm = std::get<ComponentManager<CFirst>*>(_cmanagers);
				return iterator(cm->find(id), cm->end(), _cmanagers, _is_optional);
			}
		private:
			std::tuple<ComponentManager<CFirst>*, ComponentManager<COthers>*...> _cmanagers;
			bool _is_optional[1 + sizeof...(COthers)];

			template<typename CT, typename CH, typename... CR>
			constexpr size_t _cindex() {
				if constexpr (std::is_same<CT, CH>::value) {
					return 0;
				}
				return 1 + _cindex<CT, CR...>();
			}
		};

		EntityManager() {
			_last_id = 0;
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

		~EntityManager() {
			for (auto& it : _idautomanagers) {
				delete it.second;
			}
		}

		template <typename C>
		void register_component() {
			auto ti = std::type_index(typeid(C));
			ComponentManager<C>* new_cm = new ComponentManager<C>();
			_idautomanagers[ti] = new_cm;
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
			for (auto& it : _idautomanagers) {
				it.second->delete_item(id);
			}
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
			for (auto& it : _idautomanagers) {
				it.second->update_all();
			}
		}
	private:
		template <typename C>
		ComponentManager<C>* _manager() {
			// return &std::get<ComponentManager<C>>(_cmanagers);
			auto ti = std::type_index(typeid(C));
			return (ComponentManager<C>*)_idautomanagers[ti];
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

		EntityID _last_id;
		std::unordered_map<std::type_index, IComponentManager*> _idautomanagers;
	};
};
