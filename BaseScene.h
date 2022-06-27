#pragma once

#include <functional>
#include <optional>
#include <unordered_map>
#include <vector>

#include <SFML/Graphics.hpp>

#include "Action.h"
#include "EntityManager.h"
#include "GameManager.h"

//enum class SceneStage {
//	// These occur per game loop.
//	// POST_LOOP are items that always happen last (after rendering).
//	SCENE_POST_LOOP,
//	// PRE_LOOP happens at the very beginning. In a sense this happens
//	// immediately following POST_LOOP. This happens before events.
//	SCENE_PRE_LOOP,
//
//	// Called once per game loop only IF there are new actions to process.
//	SCENE_ON_ACTIONS,
//
//	// These always occur in a fixed step.
//	SCENE_PRE_PHYSICS,
//	SCENE_PHYSICS,
//	SCENE_POST_PHYSICS,
//
//	// These always occur every rendered frame.
//	SCENE_PRE_RENDER,
//	SCENE_RENDER,
//	SCENE_GUI_RENDER,
//	SCENE_POST_RENDER,
//};

struct SceneError {
	std::string description;
	SceneError(std::string desc) : description(desc) {}
};

class IScene {
public:
	virtual ~IScene() = default;

	// Life cycle methods
	// These can all return errors that will cause the game to open an error box and exit.
	virtual std::optional<SceneError> Load(GameManager& gm) = 0;
	virtual std::optional<SceneError> Unload(GameManager& gm) = 0;
	virtual std::optional<SceneError> Show(GameManager& gm) = 0;
	virtual std::optional<SceneError> Hide(GameManager& gm) = 0;

	// Game loop lifecycle
	virtual void BeginLoop(GameManager& gm) = 0;

	// Various OnEvent handlers, or really just the one for now.
	virtual void OnAction(GameManager& gm, const std::vector<Action>& actions, const std::unordered_map<ActionType, ActionState>& action_states) = 0;

	// TODO: I probably need more stages...
	virtual void FixedUpdate(GameManager& gm) = 0;
	virtual void Render(GameManager& gm, sf::RenderWindow& window, int last_render) = 0;
	virtual void RenderGUI(GameManager& gm, sf::RenderWindow& window, int last_render) = 0;

	virtual void EndLoop(GameManager& gm) = 0;
};

template <typename Derived>
class BaseScene : public IScene {
public:
	// Loop systems are run every game loop.
	typedef std::function<void(Derived&, GameManager&)> LoopSystem;

	// Action systems take in a list of actions.
	// These are run at-most once per game loop, but are not called when
	// no actions exist. The other argument is the current state to key off states
	// instead of on keypress/release.
	typedef std::function<void(Derived&, GameManager&, const std::vector<Action>&, const std::unordered_map<ActionType, ActionState>&)> ActionSystem;

	// FixedUpdate systems expect to be run at a fixed rate and thus can expect a
	// fixed step length. This is often 20ms (50hz). It is possible for a loop to
	// not run fixed update and possible for a loop to run multiple fixed updates.
	typedef std::function<void(Derived&, GameManager&)> FixedUpdateSystem;

	// Render systems are run at a different rate than fixed update and may not
	// run at a fixed interval. The update frequency can either be the main game
	// loop, a separate rendering loop thread, or controlled by vsync.
	// They recieve the number of millis since the last update.
	typedef std::function<void(Derived&, GameManager&, sf::RenderWindow&, int)> RenderSystem;

	BaseScene();
	virtual ~BaseScene();

	// Life cycle methods
	virtual std::optional<SceneError> Load(GameManager& gm);
	virtual std::optional<SceneError> Unload(GameManager& gm);
	virtual std::optional<SceneError> Show(GameManager& gm);
	virtual std::optional<SceneError> Hide(GameManager& gm);

	// Game loop lifecycle
	void BeginLoop(GameManager& gm);

	// Various OnEvent handlers, or really just the one for now.
	void OnAction(GameManager& gm, const std::vector<Action>& actions, const std::unordered_map<ActionType, ActionState>& action_states);

	// TODO: I probably need more stages...
	void FixedUpdate(GameManager& gm);
	void Render(GameManager& gm, sf::RenderWindow& window, int last_render);
	void RenderGUI(GameManager& gm, sf::RenderWindow& window, int last_render);

	void EndLoop(GameManager& gm);

	// System management methods
	void RegisterBeginLoopSystem(LoopSystem system);
	void RegisterEndLoopSystem(LoopSystem system);
	void RegisterActionSystem(ActionSystem system);
	void RegisterFixedUpdateSystem(FixedUpdateSystem system);
	void RegisterRenderSystem(RenderSystem system);
	void RegisterRenderGUISystem(RenderSystem system);

	MattECS::EntityManager& entity_manager();

private:
	MattECS::EntityManager _entity_manager;
	// std::unordered_map<SceneStage, std::vector<System>> _systems;
	std::vector<LoopSystem> _begin_loop_systems;
	std::vector<LoopSystem> _end_loop_systems;
	std::vector<ActionSystem> _action_systems;
	std::vector<FixedUpdateSystem> _fixed_systems;
	std::vector<RenderSystem> _render_systems;
	std::vector<RenderSystem> _gui_systems;

	template <typename F, typename... Args>
	void _run_systems(std::vector<F>& systems, Args... args);
};

template<typename Derived>
BaseScene<Derived>::BaseScene() {}

template <typename Derived>
BaseScene<Derived>::~BaseScene() {}

template <typename Derived>
std::optional<SceneError> BaseScene<Derived>::Load(GameManager& gm) {
	return {};
}

template <typename Derived>
std::optional<SceneError> BaseScene<Derived>::Unload(GameManager& gm) {
	return {};
}

template <typename Derived>
std::optional<SceneError> BaseScene<Derived>::Show(GameManager& gm) {
	return {};
}

template <typename Derived>
std::optional<SceneError> BaseScene<Derived>::Hide(GameManager& gm) {
	return {};
}


// System management methods
template <typename Derived>
void BaseScene<Derived>::RegisterBeginLoopSystem(LoopSystem system) {
	_begin_loop_systems.push_back(system);
}
template <typename Derived>
void BaseScene<Derived>::RegisterEndLoopSystem(LoopSystem system) {
	_end_loop_systems.push_back(system);
}
template <typename Derived>
void BaseScene<Derived>::RegisterActionSystem(ActionSystem system) {
	_action_systems.push_back(system);
}
template <typename Derived>
void BaseScene<Derived>::RegisterFixedUpdateSystem(FixedUpdateSystem system) {
	_fixed_systems.push_back(system);
}
template <typename Derived>
void BaseScene<Derived>::RegisterRenderSystem(RenderSystem system) {
	_render_systems.push_back(system);
}
template <typename Derived>
void BaseScene<Derived>::RegisterRenderGUISystem(RenderSystem system) {
	_gui_systems.push_back(system);
}

template <typename Derived>
template <typename F, typename... Args>
void BaseScene<Derived>::_run_systems(std::vector<F>& systems, Args... args) {
	if (systems.size() == 0) {
		return;
	}
	for (auto s : systems) {
		s(*static_cast<Derived*>(this), args...);
	}
	_entity_manager.finalize_update();
}

template <typename Derived>
void BaseScene<Derived>::BeginLoop(GameManager& gm) {
	_run_systems<LoopSystem, GameManager&>(_begin_loop_systems, gm);
}

template <typename Derived>
void BaseScene<Derived>::OnAction(GameManager& gm, const std::vector<Action>& actions, const std::unordered_map<ActionType, ActionState>& action_states) {
	if (_action_systems.size() == 0) {
		return;
	}
	for (auto s : _action_systems) {
		s(*static_cast<Derived*>(this), gm, actions, action_states);
	}
	_entity_manager.finalize_update();
}

template <typename Derived>
void BaseScene<Derived>::FixedUpdate(GameManager& gm) {
	_run_systems<FixedUpdateSystem, GameManager&>(_fixed_systems, gm);
	_entity_manager.end_frame();
}

template <typename Derived>
void BaseScene<Derived>::Render(GameManager& gm, sf::RenderWindow& window, int last_render) {
	_run_systems<RenderSystem, GameManager&, sf::RenderWindow&, int>(_render_systems, gm, window, last_render);
}

template <typename Derived>
void BaseScene<Derived>::RenderGUI(GameManager& gm, sf::RenderWindow& window, int last_render) {
	_run_systems<RenderSystem, GameManager&, sf::RenderWindow&, int>(_gui_systems, gm, window, last_render);
}

template <typename Derived>
void BaseScene<Derived>::EndLoop(GameManager& gm) {
	_run_systems<LoopSystem, GameManager&>(_end_loop_systems, gm);
}

template <typename Derived>
MattECS::EntityManager& BaseScene<Derived>::entity_manager() {
	return _entity_manager;
}
