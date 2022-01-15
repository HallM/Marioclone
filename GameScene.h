#pragma once

#include <unordered_map>
#include <vector>

#include "Action.h"
#include "BaseScene.h"
#include "EntityManager.h"
#include "Levels.h"

class GameScene : public BaseScene {
public:
	GameScene(AssetManager* asset_manager, TileMap* level);
	~GameScene();

	// Loads the level and generates all the entities.
	virtual void Load(GameManager* gm);
	virtual void Unload(GameManager* gm);
	virtual void Show(GameManager* gm);

private:
	// Get user input and translate to movement on the player
	// Components: Velocity*, BulletSpawner
	// May spawn with: Velocity, AABB, Lifetime, Animation
	std::vector<Action> InputSystem(GameManager* gm, std::vector<Action> actions, const std::unordered_map<ActionType, ActionState>& action_states);

	// The following are part of the Fixed Update system.

	// Determine how on-screen enemies should move
	// Components: Velocity*
	void AISystem(GameManager* gm);

	// Update objects with limited lifetime and destroy afterwards
	// Components: Lifetime*
	void LifetimeSystem(GameManager* gm);
	// Make objects fall if subject to gravity
	// Components: Velocity*, Gravity
	void GravitySystem(GameManager* gm);
	// Move objects with velocity
	// Components: Velocity, Position*
	void MovementSystem(GameManager* gm);

	// Detects overlap of AABBs, but does not resolve. just stores results.
	// Components: AABB, Collision*
	void DetectCollisionSystem(GameManager* gm);

	// Resolve overlapping AABBs by shifting moving objects.
	// Components: Collision, Velocity*, Position*
	void ResolveCollisionSystem(GameManager* gm);
	// Destroy things that are destructable when hit by a collision in a specific manner
	// Components: Collision, FragmentSpawner
	// May spawn with: Velocity, Lifetime, Animation
	void DestructionSystem(GameManager* gm);
	// Player dies when falling out of bounds or when touching an enemy
	// Components: Collision? or just Position
	void PlayerDeathSystem(GameManager* gm);
	// Player wins if hitting the flag pole collider
	// Components: Collision? or just Position
	void PlayerVictorySystem(GameManager* gm);

	// Set the player's correct sprite/animation sequence
	// Components: Animation*
	void SetPlayerAnimationSystem(GameManager* gm);
	// Run the animations on objects and yes this is FixedUpdate, not render update.
	// Components: Animation*
	void AnimationSystem(GameManager* gm);

	// The following are part of the Render cycle.

	// Render all objects, but not the GUI
	// Components: Position, Animation
	void Render(GameManager* gm, sf::RenderWindow* window, int delta_ms);
	// Render the GUI if any
	// Components: None, doesn't use entities I don't think.
	void DrawGUI(GameManager* gm, sf::RenderWindow* window, int delta_ms);

	MattECS::EntityID _player;

	// this is not an owned pointer, but referenced.
	// expecting the Levels manager to own it.
	TileMap* _level;

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
