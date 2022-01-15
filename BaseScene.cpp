#include "BaseScene.h"

BaseScene::BaseScene(AssetManager* asset_manager) {
	_asset_manager = asset_manager;
}

BaseScene::~BaseScene() {
}

void BaseScene::Load(GameManager* gm) {}
void BaseScene::Unload(GameManager* gm) {}
void BaseScene::Show(GameManager* gm) {}
void BaseScene::Hide(GameManager* gm) {}


// System management methods
void BaseScene::RegisterBeginLoopSystem(LoopSystem system) {
	_begin_loop_systems.push_back(system);
}
void BaseScene::RegisterEndLoopSystem(LoopSystem system) {
	_end_loop_systems.push_back(system);
}
void BaseScene::RegisterActionSystem(ActionSystem system) {
	_action_systems.push_back(system);
}
void BaseScene::RegisterFixedUpdateSystem(FixedUpdateSystem system) {
	_fixed_systems.push_back(system);
}
void BaseScene::RegisterRenderSystem(RenderSystem system) {
	_render_systems.push_back(system);
}
void BaseScene::RegisterRenderGUISystem(RenderSystem system) {
	_gui_systems.push_back(system);
}

template <typename F, typename... Args>
void BaseScene::_run_systems(std::vector<F>& systems, Args... args) {
	if (systems.size() == 0) {
		return;
	}
	for (auto s : systems) {
		s(args...);
	}
	_entity_manager.finalize_update();
}

void BaseScene::BeginLoop(GameManager* gm) {
	_run_systems<LoopSystem, GameManager*>(_begin_loop_systems, gm);
}

void BaseScene::OnAction(GameManager* gm, std::vector<Action> actions, const std::unordered_map<ActionType, ActionState>& action_states) {
	if (_action_systems.size() == 0) {
		return;
	}
	std::vector<Action> last = actions;
	for (auto s : _action_systems) {
		last = s(gm, last, action_states);
	}
	_entity_manager.finalize_update();
}

void BaseScene::FixedUpdate(GameManager* gm) {
	_run_systems<FixedUpdateSystem, GameManager*>(_fixed_systems, gm);
}

void BaseScene::Render(GameManager* gm, sf::RenderWindow* window, int last_render) {
	_run_systems<RenderSystem, GameManager*, sf::RenderWindow*, int>(_render_systems, gm, window, last_render);
}

void BaseScene::RenderGUI(GameManager* gm, sf::RenderWindow* window, int last_render) {
	_run_systems<RenderSystem, GameManager*, sf::RenderWindow*, int>(_gui_systems, gm, window, last_render);
}

void BaseScene::EndLoop(GameManager* gm) {
	_run_systems<LoopSystem, GameManager*>(_end_loop_systems, gm);
}
