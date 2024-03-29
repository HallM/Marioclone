#pragma once

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "../Scriptlang/Program.h"
#include "../Scriptlang/Compiler.h"
#include "../Scriptlang/VMStack.h"

#include "Action.h"
#include "BaseScene.h"
#include "Components.h"
#include "MapManager.h"

class GameScene : public BaseScene<GameScene> {
public:
	GameScene(const Map level);
	virtual ~GameScene();

	// Loads the level and generates all the entities.
	virtual std::optional<SceneError> Load(GameManager& gm);
	virtual std::optional<SceneError> Unload(GameManager& gm);
	virtual std::optional<SceneError> Show(GameManager& gm);

private:
	// Get user input and translate to movement on the player
	// Components: Velocity*, BulletSpawner
	// May spawn with: Velocity, AABB, Lifetime, Animation
	void InputSystem(GameManager& gm, const std::vector<Action>& actions, const std::unordered_map<ActionType, ActionState>& action_states);

	// The following are part of the Fixed Update system.

	// Determine how on-screen enemies should move
	// Components: Velocity*
	void AISystem(GameManager& gm);

	// Update objects with limited lifetime and destroy afterwards
	// Components: Lifetime*
	void LifetimeSystem(GameManager& gm);
	// Make objects fall if subject to gravity
	// Components: Velocity*, Gravity
	void GravitySystem(GameManager& gm);
	// Move objects with velocity
	// Components: Velocity, Position*
	void MovementSystem(GameManager& gm);

	// Detects overlap of AABBs, but does not resolve. just stores results.
	// Components: AABB, Collision*
	void DetectCollisionSystem(GameManager& gm);

	// Resolve overlapping AABBs by shifting moving objects.
	// Components: Collision, Velocity*, Position*
	void ResolveCollisionSystem(GameManager& gm);
	// Destroy things that are destructable when hit by a collision in a specific manner
	// Components: Collision, FragmentSpawner
	// May spawn with: Velocity, Lifetime, Animation
	void DestructionSystem(GameManager& gm);
	// Player dies when falling out of bounds or when touching an enemy
	// Components: Collision? or just Position
	void PlayerDeathSystem(GameManager& gm);
	// Player wins if hitting the flag pole collider
	// Components: Collision? or just Position
	void PlayerVictorySystem(GameManager& gm);

	// Set the player's correct sprite/animation sequence
	// Components: Animation*
	void SetPlayerAnimationSystem(GameManager& gm);
	// Run the animations on objects and yes this is FixedUpdate, not render update.
	// Components: Animation*
	void AnimationSystem(GameManager& gm);

	// The following are part of the Render cycle.

	// Render all objects, but not the GUI
	// Components: Position, Animation
	void Render(GameManager& gm, sf::RenderWindow& window, int delta_ms);
	// Render the GUI if any
	// Components: None, doesn't use entities I don't think.
	void DrawGUI(GameManager& gm, sf::RenderWindow& window, int delta_ms);
	// Draws the buffer to the screen.
	void DrawBuffer(GameManager& gm, sf::RenderWindow& window, int delta_ms);

	// Scripting API
	void AddCoin(int quantity);
	void FragmentEntity(MattECS::EntityID entity);
	void DestroyEntity(MattECS::EntityID entity);
	void SetEntityAnimation(MattECS::EntityID entity, GameManager* gm, int sheet_id, int animation_id);

	std::shared_ptr<Program> GetScript(std::string name);
	MattScript::Compiler _script_compiler;
	std::unordered_map<std::string, std::shared_ptr<Program>> _cached_scripts;
	std::shared_ptr<VM> _script_vm;
	
	MattECS::EntityID _player;
	int _coins;

	Map _level;

	sf::Text _coins_text;


	sf::RenderTexture _render_texture;
	sf::View _camera;
	sf::View _gui_view;
	float min_screen_x;
	float max_screen_x;

	bool _render_colliders;
	int _milestone_reached;

	int _frames;
	sf::Text _fps_text;
	sf::Clock _fpsclock;

	// there exists one vert array for each spritesheet (texture ptr)
	// now this impl does assume that items on different layers (Z)
	// have separate textures.
	// std::vector<sf::VertexArray> _vert_arrays;
	// where in the above array a ptr exists.
	// std::unordered_map<void*, unsigned int> _va_location;
};
