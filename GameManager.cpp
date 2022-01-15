#include "GameManager.h"

#include "Action.h"
#include "BaseScene.h"

const int MS_PER_TICK = 20;

GameManager::GameManager(std::unique_ptr<AssetManager> assets, std::unique_ptr<sf::RenderWindow> window) :
	_asset_manager(std::move(assets)),
	_window(std::move(window)),
	_do_pop(false),
	_to_push(),
	_bg(sf::Color::Black) {}

GameManager::~GameManager() {}

void GameManager::Quit() {
	_window->close();
}

void GameManager::PushScene(std::unique_ptr<IScene> scene) {
	_to_push = std::move(scene);
}

void GameManager::PopScene() {
	_do_pop = true;
}

void GameManager::ReplaceScene(std::unique_ptr<IScene> scene) {
	PopScene();
	PushScene(std::move(scene));
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
		if (_to_push) {
			if (_scene_stack.size() > 0) {
				IScene& scene = *_scene_stack.back();
				scene.Hide(*this);
			}

			_scene_stack.push_back(std::move(_to_push.value()));
			IScene& scene = *_scene_stack.back();
			auto maybe_error = scene.Load(*this);
			if (maybe_error) {
				std::cerr << maybe_error.value().description << "\n";
				return;
			}
			maybe_error = scene.Show(*this);
			if (maybe_error) {
				std::cerr << maybe_error.value().description << "\n";
				return;
			}
			_to_push = {};
		}

		while (!_do_pop && !_to_push && _window->isOpen()) {
			IScene& scene = *_scene_stack.back();

			scene.BeginLoop(*this);

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
			scene.OnAction(*this, actions, _current_action_states);

			if (update_clock.getElapsedTime().asMilliseconds() >= MS_PER_TICK) {
				sf::Time elapsed = update_clock.restart();
				time_remain += elapsed.asMilliseconds();

				while (time_remain >= MS_PER_TICK) {
					sf::Clock clk;
					scene.FixedUpdate(*this);
					auto el = clk.getElapsedTime().asMicroseconds();
					std::cout << el << " us\n";
					time_remain -= MS_PER_TICK;
				}
			}

			sf::Time elapsed = render_clock.restart();
			_window->clear(_bg);
			scene.Render(*this, *_window, elapsed.asMilliseconds());
			scene.RenderGUI(*this, *_window, elapsed.asMilliseconds());
			_window->display();

			scene.EndLoop(*this);
		}

		if (_do_pop) {
			_do_pop = false;
			if (_scene_stack.size() > 0) {
				IScene& s = *_scene_stack.back();
				auto maybe_error = s.Hide(*this);
				if (maybe_error) {
					std::cerr << maybe_error.value().description << "\n";
					return;
				}
				maybe_error = s.Unload(*this);
				if (maybe_error) {
					std::cerr << maybe_error.value().description << "\n";
					return;
				}
				_scene_stack.pop_back();

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
