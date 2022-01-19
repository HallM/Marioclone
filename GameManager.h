#pragma once

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include <SFML/Graphics.hpp>

#include "Action.h"
#include "AssetManager.h"
#include "MapManager.h"

// Forward decl to avoid circular references.
class IScene;

class GameManager
{
public:
	GameManager(std::unique_ptr<AssetManager> assets, std::unique_ptr<MapManager> maps, std::unique_ptr<sf::RenderWindow> window);
	~GameManager();

	void Quit();

	void PushScene(std::unique_ptr<IScene> scene);
	void PopScene();
	void ReplaceScene(std::unique_ptr<IScene> scene);

	void SetCamera(const sf::View& camera);
	void SetActions(const std::unordered_map<sf::Keyboard::Key, ActionType>& action_map);

	void SetBackgroundColor(const sf::Color c);

	void RunLoop();

	AssetManager& asset_manager();
	MapManager& map_manager();

private:
	std::unique_ptr<AssetManager> _asset_manager;
	std::unique_ptr<MapManager> _map_manager;

	bool _do_pop;
	std::optional<std::unique_ptr<IScene>> _to_push;
	std::vector<std::unique_ptr<IScene>> _scene_stack;

	std::unordered_map<sf::Keyboard::Key, ActionType> _action_map;
	std::unordered_map<ActionType, ActionState> _current_action_states;

	std::unique_ptr<sf::RenderWindow> _window;

	sf::Color _bg;
};
