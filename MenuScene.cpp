#include "MenuScene.h"

#include <functional>
#include <memory>

#include "GameScene.h"

MenuScene::MenuScene(const Levels& level_manager) : _level_manager(level_manager), _item_selected(0) {
	RegisterActionSystem(&MenuScene::HandleInput);
	RegisterRenderGUISystem(&MenuScene::RenderMenu);
}

MenuScene::~MenuScene() {}

std::optional<SceneError> MenuScene::Load(GameManager& gm) {
	gm.SetBackgroundColor(sf::Color::Black);

	auto font = gm.asset_manager().GetFont("Roboto");

	for (const auto& level_name : _level_manager.GetLevelNames()) {
		sf::Text t(level_name, *font, 24);
		t.setFillColor(sf::Color::White);
		_menu_items.push_back(t);
	}

	sf::Text quit_text("Exit to Desktop", *font, 24);
	_menu_items.push_back(quit_text);

	_title = sf::Text("Totally Not Mario", *font, 48);
	_title.setFillColor(sf::Color::White);
	_title.setPosition(5.0f, 5.0f);
	_selection_indicator = sf::Text(">", *font, 24);
	_selection_indicator.setFillColor(sf::Color::White);
	return {};
}

std::optional<SceneError> MenuScene::Show(GameManager& gm) {
	gm.SetBackgroundColor(sf::Color::Black);

	std::unordered_map<sf::Keyboard::Key, ActionType> actions;
	actions[sf::Keyboard::Key::Up] = ActionType::UP;
	actions[sf::Keyboard::Key::Down] = ActionType::DOWN;
	actions[sf::Keyboard::Key::W] = ActionType::UP;
	actions[sf::Keyboard::Key::S] = ActionType::DOWN;
	actions[sf::Keyboard::Key::Space] = ActionType::SELECT;
	actions[sf::Keyboard::Key::Enter] = ActionType::SELECT;
	actions[sf::Keyboard::Key::Escape] = ActionType::MENU;
	gm.SetActions(actions);
	return {};
}

void MenuScene::HandleInput(GameManager& gm, const std::vector<Action>& actions, const std::unordered_map<ActionType, ActionState>& action_states) {
	unsigned int total_menu = _menu_items.size();
	bool can_go_up = _item_selected > 0;
	bool can_go_down = _item_selected < (total_menu - 1);

	for (auto action : actions) {
		if (action.state != ActionState::START) {
			continue;
		}
		switch (action.type) {
		case ActionType::UP:
			if (can_go_up) {
				_item_selected--;
			}
			break;
		case ActionType::DOWN:
			if (can_go_down) {
				_item_selected++;
			}
			break;
		case ActionType::SELECT:
			if (_item_selected == (total_menu - 1)) {
				gm.Quit();
			}
			else {
				auto level_name = _menu_items[_item_selected].getString();
				gm.PushScene(std::make_unique<GameScene>(_level_manager.GetLevel(level_name).value()));
			}
			break;
		case ActionType::MENU:
			gm.Quit();
			break;
		}
	}
}

void MenuScene::RenderMenu(GameManager& gm, sf::RenderWindow& window, int last_update) {
	float x = 100.0f;
	float y = 100.0f;

	unsigned int i = 0;

	for (auto menu_item : _menu_items) {
		menu_item.setPosition(x, y);
		window.draw(menu_item);

		if (i == _item_selected) {
			menu_item.setStyle(sf::Text::Bold);
			_selection_indicator.setPosition(5.0f, y);
		}
		else {
			menu_item.setStyle(sf::Text::Regular);
		}

		auto bounds = menu_item.getLocalBounds();
		y += bounds.top + bounds.height;
		i++;
	}

	window.draw(_title);
	window.draw(_selection_indicator);
}
