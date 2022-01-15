#pragma once

#include <unordered_map>
#include <vector>

#include <SFML/Graphics.hpp>

#include "Action.h"

// Forward decl to avoid circular references.
class BaseScene;

class GameManager
{
public:
	GameManager(sf::RenderWindow* window);
	~GameManager();

	void Quit();

	void PushScene(BaseScene* scene);
	void PopScene();
	void ReplaceScene(BaseScene* scene);

	void SetCamera(const sf::View& camera);
	void SetActions(const std::unordered_map<sf::Keyboard::Key, ActionType>& action_map);

	void SetBackgroundColor(const sf::Color c);

	void RunLoop();
private:
	bool _do_pop;
	BaseScene* _to_push;
	std::vector<BaseScene*> _scene_stack;

	std::unordered_map<sf::Keyboard::Key, ActionType> _action_map;
	std::unordered_map<ActionType, ActionState> _current_action_states;

	sf::RenderWindow* _window;

	sf::Color _bg;
};
