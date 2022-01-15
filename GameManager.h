#pragma once

#include <unordered_map>
#include <vector>

#include <SFML/Graphics.hpp>

#include "Action.h"
#include "AssetManager.h"

// Forward decl to avoid circular references.
class IScene;

class GameManager
{
public:
	GameManager(AssetManager* assets, sf::RenderWindow* window);
	~GameManager();

	void Quit();

	void PushScene(IScene* scene);
	void PopScene();
	void ReplaceScene(IScene* scene);

	void SetCamera(const sf::View& camera);
	void SetActions(const std::unordered_map<sf::Keyboard::Key, ActionType>& action_map);

	void SetBackgroundColor(const sf::Color c);

	void RunLoop();

	AssetManager& asset_manager();

private:
	AssetManager* _asset_manager;

	bool _do_pop;
	IScene* _to_push;
	std::vector<IScene*> _scene_stack;

	std::unordered_map<sf::Keyboard::Key, ActionType> _action_map;
	std::unordered_map<ActionType, ActionState> _current_action_states;

	sf::RenderWindow* _window;

	sf::Color _bg;
};
