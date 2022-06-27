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

#include "ComponentContainer.h"

namespace MattECS {
	typedef size_t EntityID;
	const size_t MAX_ENTITIES = 10000;

	class EntityManager {
	public:
		template <typename CFirst, typename... COthers>
		class Querier {
		public:
			class iterator {
			public:
				iterator(ComponentContainer<CFirst>::iterator it, ComponentContainer<CFirst>::iterator end, std::tuple<ComponentContainer<CFirst>*, ComponentContainer<COthers>*...>& cmanagers, bool is_optional[1 + sizeof...(COthers)]) : _it(it), _end(end), _cmanagers(cmanagers) {
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

				// C++ accessors by type
				template <typename C>
				bool has() {
					return _indices[_cindex<C, CFirst, COthers...>()].has_value();
				}
				template <typename C>
				const C& value() {
					auto index = _indices[_cindex<C, CFirst, COthers...>()].value();
					return std::get<ComponentContainer<C>*>(_cmanagers)->at(index);
				}
				template <typename C>
				C& mut() {
					auto index = _indices[_cindex<C, CFirst, COthers...>()].value();
					std::get<ComponentContainer<C>*>(_cmanagers)->set_changed(index);
					return std::get<ComponentContainer<C>*>(_cmanagers)->at(index);
				}

				// Script accessors by index (though I guess useful for C++ too...)
				template <size_t I>
				bool has() {
					return _indices[I].has_value();
				}
				template <size_t I>
				const auto value() {
					auto index = _indices[I].value();
					return &std::get<I>(_cmanagers)->at(index);
				}
				template <size_t I>
				auto mut() {
					auto index = _indices[I].value();
					auto cm = std::get<I>(_cmanagers);
					cm->set_changed(index);
					return cm->at(index);
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
					((_indices[Is+1] = std::get<ComponentContainer<COthers>*>(_cmanagers)->find(id).index()), ...);
					_has_all = ((_is_optional[Is+1] || _indices[Is+1]) && ...);
				}
				bool _has_all;
				std::optional<size_t> _indices[1 + sizeof...(COthers)];
				ComponentContainer<CFirst>::iterator _it;
				ComponentContainer<CFirst>::iterator _end;
				bool _is_optional[1 + sizeof...(COthers)];
				std::tuple<ComponentContainer<CFirst>*, ComponentContainer<COthers>*...>& _cmanagers;

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

			Querier(std::tuple<ComponentContainer<CFirst>*, ComponentContainer<COthers>*...> managers) : _cmanagers(managers) {
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
				auto cm = std::get<ComponentContainer<CFirst>*>(_cmanagers);
				return iterator(cm->begin(), cm->end(), _cmanagers, _is_optional);
			}
			iterator end() {
				auto cm = std::get<ComponentContainer<CFirst>*>(_cmanagers);
				return iterator(cm->end(), cm->end(), _cmanagers, _is_optional);
			}
			iterator find(EntityID id) {
				auto cm = std::get<ComponentContainer<CFirst>*>(_cmanagers);
				return iterator(cm->find(id), cm->end(), _cmanagers, _is_optional);
			}
		private:
			std::tuple<ComponentContainer<CFirst>*, ComponentContainer<COthers>*...> _cmanagers;
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
			_next_component_id = 0;
		}
		~EntityManager() {
			for (auto& it : _components) {
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

		template <
			typename C,
			typename void(*Orderer)(std::vector<size_t>&, SparseHashmap<EntityID, C>&) = no_orderer<C>,
			typename void(*OnChange)(EntityID, const C&) = no_change_handler<C>
		>
		size_t register_component() {
			size_t id = _next_component_id++;
			auto ti = std::type_index(typeid(C));
			ComponentContainer<C, Orderer>* new_cm = new ComponentContainer<C, Orderer, OnChange>(MAX_ENTITIES);
			_cpp_types[ti] = id;
			_components[id] = new_cm;
			// _idautomanagers[ti] = new_cm;
			return id;
		}
		size_t register_script_component() {
			size_t id = _next_component_id++;
			ComponentContainer<char>* new_cm = new ComponentContainer<char>(MAX_ENTITIES);
			_components[id] = new_cm;
			return id;
		}

		EntityID entity() {
			EntityID new_id = _last_id++;
			return new_id;
		}

		template <typename C>
		const C& get(EntityID id) {
			auto c = _manager<C>();
			return c->cvalue(id);
		}
		template <typename C>
		const C* getptr(EntityID id) {
			auto c = _manager<C>();
			return &c->cvalue(id);
		}
		template <typename C>
		C* mut(EntityID id) {
			auto c = _manager<C>();
			return &c->value(id);
		}

		template <typename C>
		const std::optional<const C*> tryGet(EntityID id) {
			auto c = _manager<C>();
			if (c->has(id)) {
				return &c->cvalue(id);
			}
			return {};
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
			for (auto& it : _components) {
				it.second->delete_item(id);
			}
		}

		// finalize_update should be called when no iterators are held
		// to avoid invalidating iterators. This will finalize added/removed
		// components and entities.
		void finalize_update() {
			for (auto& it : _components) {
				it.second->update_all();
			}
		}

		void end_frame() {
			for (auto& it : _components) {
				it.second->end_frame();
			}
		}
	private:
		template <typename C>
		ComponentContainer<C>* _manager() {
			auto ti = std::type_index(typeid(C));
			size_t id = _cpp_types[ti];
			assert(_components.find(id) != _components.end());
			return (ComponentContainer<C>*)_components[id];
		}

		// https://stackoverflow.com/questions/61281843/creating-compile-time-key-value-map-in-c
		EntityID _last_id;
		// std::unordered_map<std::type_index, IComponentContainer*> _idautomanagers;
		size_t _next_component_id;
		std::unordered_map<std::type_index, size_t> _cpp_types;
		std::unordered_map<size_t, IComponentContainer*> _components;
	};
};
