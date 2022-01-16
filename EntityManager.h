#pragma once

#include <iostream>
#include <functional>
#include <optional>
#include <tuple>
#include <typeindex>
#include <typeinfo> 
#include <unordered_map>
#include <vector>

#include "timsort.hpp"

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
			size_t _index = 0;
			ComponentManager<C>* _c = nullptr;
		public:
			explicit iterator(ComponentManager<C>* c, size_t i) : _c(c), _index(i) {}
			bool is_end() const { return _index >= _c->_ids.size(); }
			bool has_next() const { return (_index + 1) < _c->_ids.size(); }
			iterator& operator++() { _index++; return *this; }
			iterator operator++(int) { iterator it(_c, _index); _index++; return it; }
			iterator& operator--() { _index--; return *this; }
			iterator operator--(int) { iterator it(_c, _index); _index--; return it; }
			bool operator==(iterator other) const { return (is_end() && other.is_end()) || _index == other._index; }
			bool operator!=(iterator other) const { return !((is_end() && other.is_end()) || _index == other._index); }
			EntityID entity() const { return _c->_ids[_index]; }
			C& value() const { return _c->_values[_index]; }
			std::optional<size_t> index() const {
				if (is_end()) { return {}; }
				return _index;
			}
		};

		ComponentManager() : _has_sorter(false), _changed(false), _has_on_change(false) {}
		virtual ~ComponentManager() {}

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
			return _values[index];
		}
		C& value(EntityID id) {
			auto index = _id_to_index[id];
			return _values[index];
		}

		C& at(size_t index) {
			return _values[index];
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

		void set_changed(size_t index) {
			_changed = true;
			_dirty[index] = true;
		}

		void set_sorter(std::function<bool(const C&, const C&)> less) {
			_less = less;
			_has_sorter = true;
			_changed = true;
		}
		void set_on_change(std::function<void(EntityID, const C&)> on_change) {
			_has_on_change = true;
			_on_change = on_change;
		}

		virtual void update_all() {
			for (auto id : _deleted_items) {
				unsigned int index = _id_to_index[id];
				unsigned int last = _ids.size() - 1;
				std::swap(_ids[index], _ids[last]);
				std::swap(_values[index], _values[last]);
				_dirty[index] = _dirty[last];
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
				_values.push_back(item.second);
				_dirty.push_back(true);
				_id_to_index[id] = index;
			}
			_new_items.clear();

			if (_changed) {
				if (_has_on_change) {
					for (size_t i = 0; i < _dirty.size(); i++) {
						_dirty[i] = false;
						_on_change(_ids[i], _values[i]);
					}
				}
				if (_has_sorter) {
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
			gfx::timsort(
				indices,
				_less,
				[this](size_t a) -> const C& {
					return _values[a];
				}
			);
			_apply_sort<EntityID>(_ids, indices);
			_apply_sort<C>(_values, indices);

			for (unsigned int i = 0; i < _ids.size(); i++) {
				_id_to_index[_ids[i]] = i;
			}
		}

		template <typename V>
		void _apply_sort(std::vector<V>& tosort, const std::vector<size_t>& indices) {
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

		bool _has_sorter;
		std::function<bool(const C&, const C&)> _less;

		// _changed is set if any are dirty
		bool _changed;
		// track which elements changed or at least what mut()s were called.
		std::vector<bool> _dirty;
		bool _has_on_change;
		std::function<void(EntityID, const C&)> _on_change;

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
					_set_values(std::index_sequence_for<COthers...>{});
					if (!_has_all && _it != _end) {
						_next();
					}
				}

				// prefix
				iterator& operator++() {
					_next();
					return *this;
				}

				bool operator==(iterator other) const { return _it == other._it; }
				bool operator!=(iterator other) const { return _it != other._it; }

				EntityID entity() const { return _it.entity(); }
				template <typename C>
				bool has() {
					return _indices[_cindex<C, CFirst, COthers...>()].has_value();
				}

				template <typename C>
				const C& value() {
					auto index = _indices[_cindex<C, CFirst, COthers...>()].value();
					return std::get<ComponentManager<C>*>(_cmanagers)->at(index);
				}

				template <typename C>
				C& mut() {
					auto index = _indices[_cindex<C, CFirst, COthers...>()].value();
					std::get<ComponentManager<C>*>(_cmanagers)->set_changed(index);
					return std::get<ComponentManager<C>*>(_cmanagers)->at(index);
				}
			private:
				void _next() {
					do {
						++_it;
						_set_values(std::index_sequence_for<COthers...>{});
					} while (!_has_all && _it != _end);
				}
				template <std::size_t... Is>
				void _set_values(std::index_sequence<Is...>) {
					if (_it == _end) {
						return;
					}
					auto id = _it.entity();
					_indices[0] = _it.index();
					((_indices[Is+1] = std::get<ComponentManager<COthers>*>(_cmanagers)->find(id).index()), ...);
					_has_all = ((_is_optional[Is+1] || _indices[Is+1]) && ...);
				}
				bool _has_all;
				std::optional<size_t> _indices[1 + sizeof...(COthers)];
				ComponentManager<CFirst>::iterator _it;
				ComponentManager<CFirst>::iterator _end;
				bool _is_optional[1 + sizeof...(COthers)];
				std::tuple<ComponentManager<CFirst>*, ComponentManager<COthers>*...>& _cmanagers;

				template<typename CT, typename CH, typename... CR>
				constexpr size_t _cindex() {
					if constexpr (std::is_same<CT, CH>::value) {
						return 0;
					}
					return 1 + _cindex<CT, CR...>();
				}
				template<typename CT>
				constexpr size_t _cindex() {
					return 0;
				}
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
			template<typename CT>
			constexpr size_t _cindex() {
				return 0;
			}
		};

		EntityManager() {
			_last_id = 0;
		}
		~EntityManager() {
			for (auto& it : _idautomanagers) {
				delete it.second;
			}
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
			_idautomanagers[ti] = new_cm;
		}

		EntityID entity() {
			EntityID new_id = _last_id++;
			return new_id;
		}

		template <typename C>
		void sort(std::function<bool(const C&, const C&)> less) {
			auto c = _manager<C>();
			c->set_sorter(less);
		}

		template <typename C>
		void on_change(std::function<void(EntityID, const C&)> handler) {
			auto c = _manager<C>();
			c->set_on_change(handler);
		}

		template <typename C>
		const C& get(EntityID id) {
			auto c = _manager<C>();
			return c->cvalue(id);
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

		EntityID _last_id;
		std::unordered_map<std::type_index, IComponentManager*> _idautomanagers;
	};
};
