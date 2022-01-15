#include "GameManager.h"

#include "Action.h"
#include "BaseScene.h"

const int MS_PER_TICK = 20;

GameManager::GameManager(AssetManager* assets, sf::RenderWindow* window) {
	_asset_manager = assets;
	_window = window;
	_do_pop = false;
	_to_push = nullptr;
	_bg = sf::Color::Black;
}

GameManager::~GameManager() {
	for (auto s : _scene_stack) {
		delete s;
	}
	_scene_stack.clear();
}

void GameManager::Quit() {
	_window->close();
}

void GameManager::PushScene(IScene* scene) {
	_to_push = scene;
}

void GameManager::PopScene() {
	_do_pop = true;
}

void GameManager::ReplaceScene(IScene* scene) {
	PopScene();
	PushScene(scene);
}

void GameManager::SetCamera(const sf::View& camera) {
	_window->setView(camera);
}

void GameManager::SetActions(const std::unordered_map<sf::Keyboard::Key, ActionType>& action_map) {
	_action_map = action_map;
	_current_action_states.clear();
	for (auto it : _action_map) {
		_current_action_states[it.second] = ActionState::END;
	}
}

void GameManager::SetBackgroundColor(const sf::Color c) {
	_bg = c;
}

void GameManager::RunLoop() {
	int time_remain = 0;
	sf::Clock update_clock;
	sf::Clock render_clock;

	while (_window->isOpen()) {
		if (_to_push != nullptr) {
			if (_scene_stack.size() > 0) {
				auto scene = _scene_stack.back();
				scene->Hide(*this);
			}

			_scene_stack.push_back(_to_push);
			_to_push->Load(*this);
			_to_push->Show(*this);
			_to_push = nullptr;
		}

		while (!_do_pop && _to_push == nullptr && _window->isOpen()) {
			auto scene = _scene_stack.back();

			scene->BeginLoop(*this);

			std::vector<Action> actions;
			sf::Event event;
			while (_window->pollEvent(event)) {
				// check the type of the event...
				switch (event.type) {
				case sf::Event::Closed:
					_window->close();
					break;

				case sf::Event::KeyPressed:
				case sf::Event::KeyReleased:
				{
					auto action_state = event.type == sf::Event::KeyPressed ? ActionState::START : ActionState::END;
					auto action_it = _action_map.find(event.key.code);
					if (action_it != _action_map.end()) {
						actions.push_back(Action{ action_it->second, action_state });
						_current_action_states[action_it->second] = action_state;
					}
					break;
				}
				default:
					break;
				}
			}
			scene->OnAction(*this, actions, _current_action_states);

			if (update_clock.getElapsedTime().asMilliseconds() >= MS_PER_TICK) {
				sf::Time elapsed = update_clock.restart();
				time_remain += elapsed.asMilliseconds();

				while (time_remain >= MS_PER_TICK) {
					sf::Clock clk;
					scene->FixedUpdate(*this);
					auto el = clk.getElapsedTime().asMicroseconds();
					std::cout << el << " us\n";
					time_remain -= MS_PER_TICK;
				}
			}

			sf::Time elapsed = render_clock.restart();
			_window->clear(_bg);
			scene->Render(*this, *_window, elapsed.asMilliseconds());
			scene->RenderGUI(*this, *_window, elapsed.asMilliseconds());
			_window->display();

			scene->EndLoop(*this);
		}

		if (_do_pop) {
			_do_pop = false;
			if (_scene_stack.size() > 0) {
				auto s = _scene_stack.back();
				s->Hide(*this);
				_scene_stack.pop_back();
				s->Unload(*this);
				delete s;

				SetCamera(_window->getDefaultView());
			}
			else {
				_window->close();
			}
		}
	}
}

AssetManager& GameManager::asset_manager() {
	return *_asset_manager;
}
