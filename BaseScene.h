#pragma once

#include <functional>
#include <unordered_map>
#include <vector>

#include <SFML/Graphics.hpp>

#include "Action.h"
#include "AssetManager.h"
#include "Components.h"
#include "GameManager.h"

// Loop systems are run every game loop.
typedef std::function<void(GameManager*)> LoopSystem;

// Action systems take in a list of actions and return any actions they did NOT
// process. These are run at-most once per game loop, but are not called when
// no actions exist. The other argument is the current state to key off states
// instead of on keypress/release.
typedef std::function<std::vector<Action>(GameManager*, std::vector<Action>, const std::unordered_map<ActionType, ActionState>&)> ActionSystem;

// FixedUpdate systems expect to be run at a fixed rate and thus can expect a
// fixed step length. This is often 20ms (50hz). It is possible for a loop to
// not run fixed update and possible for a loop to run multiple fixed updates.
typedef std::function<void(GameManager*)> FixedUpdateSystem;

// Render systems are run at a different rate than fixed update and may not
// run at a fixed interval. The update frequency can either be the main game
// loop, a separate rendering loop thread, or controlled by vsync.
// They recieve the number of millis since the last update.
typedef std::function<void(GameManager*, sf::RenderWindow*, int)> RenderSystem;

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

class BaseScene {
public:
	BaseScene(AssetManager* asset_manager);
	~BaseScene();

	// Life cycle methods
	virtual void Load(GameManager* gm);
	virtual void Unload(GameManager* gm);
	virtual void Show(GameManager* gm);
	virtual void Hide(GameManager* gm);

	// System management methods
	void RegisterBeginLoopSystem(LoopSystem system);
	void RegisterEndLoopSystem(LoopSystem system);
	void RegisterActionSystem(ActionSystem system);
	void RegisterFixedUpdateSystem(FixedUpdateSystem system);
	void RegisterRenderSystem(RenderSystem system);
	void RegisterRenderGUISystem(RenderSystem system);

	// Game loop lifecycle
	void BeginLoop(GameManager* gm);

	// Various OnEvent handlers, or really just the one for now.
	void OnAction(GameManager* gm, std::vector<Action> actions, const std::unordered_map<ActionType, ActionState>& action_states);

	// TODO: I probably need more stages...
	void FixedUpdate(GameManager* gm);
	void Render(GameManager* gm, sf::RenderWindow* window, int last_render);
	void RenderGUI(GameManager* gm, sf::RenderWindow* window, int last_render);

	void EndLoop(GameManager* gm);

protected:
	AssetManager* _asset_manager;

	EntityManager _entity_manager;
	// std::unordered_map<SceneStage, std::vector<System>> _systems;
	std::vector<LoopSystem> _begin_loop_systems;
	std::vector<LoopSystem> _end_loop_systems;
	std::vector<ActionSystem> _action_systems;
	std::vector<FixedUpdateSystem> _fixed_systems;
	std::vector<RenderSystem> _render_systems;
	std::vector<RenderSystem> _gui_systems;

private:
	template <typename F, typename... Args>
	void _run_systems(std::vector<F>& systems, Args... args);
};
