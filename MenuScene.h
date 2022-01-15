#pragma once

#include <vector>

#include <SFML/Graphics.hpp>

#include "Action.h"
#include "BaseScene.h"
#include "Levels.h"

class MenuScene : public BaseScene {
public:
	MenuScene(AssetManager* asset_manager, Levels* level_manager);

	virtual void Load(GameManager* gm);
	virtual void Show(GameManager* gm);

private:
	std::vector<Action> HandleInput(GameManager* gm, std::vector<Action> actions, const std::unordered_map<ActionType, ActionState>& action_states);
	void RenderMenu(GameManager* gm, sf::RenderWindow* window, int last_update);

	Levels* _level_manager;

	unsigned int _item_selected;
	std::vector<sf::Text> _menu_items;

	sf::Text _title;
	sf::Text _selection_indicator;
};
