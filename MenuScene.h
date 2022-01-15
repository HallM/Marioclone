#pragma once

#include <memory>
#include <vector>

#include <SFML/Graphics.hpp>

#include "Action.h"
#include "BaseScene.h"
#include "Levels.h"

class MenuScene;

class MenuScene : public BaseScene<MenuScene> {
public:
	MenuScene(const Levels& level_manager);
	virtual ~MenuScene();

	virtual void Load(GameManager& gm);
	virtual void Show(GameManager& gm);

	void HandleInput(GameManager& gm, const std::vector<Action>& actions, const std::unordered_map<ActionType, ActionState>& action_states);
	void RenderMenu(GameManager& gm, sf::RenderWindow& window, int last_update);

private:
	const Levels& _level_manager;

	unsigned int _item_selected;
	std::vector<sf::Text> _menu_items;

	sf::Text _title;
	sf::Text _selection_indicator;
};
