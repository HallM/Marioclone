#include "GameScene.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>

#include "../Scriptlang/Program.h"
#include "../Scriptlang/Compiler.h"

#include "AssetManager.h"
#include "EntityManager.h"

const int PLAYER_STARTING_HEALTH = 1;
const int PLAYER_SMALL_PIERCE = 0;
const int PLAYER_BIG_PIERCE = 1;

const int BLOCK_HARDNESS = 1;
const int ENTITY_HARDNESS = 0;

std::string MARIO_SPRITESHEET = "MarioSmall";
std::string MARIO_STAND_ANIMATION = "Stand";
std::string MARIO_RUN_ANIMATION = "Run";
std::string MARIO_FALL_ANIMATION = "Jump";

//
// Ideas:
// 2. Advanced tile map where we combine AABBs
// 3. sort pos + aabb in the same order by X ascending
//    also could sort sprites by Z maybe ?
// 4. add a "nearby" style iteration to not iterate everything
// 6. can we do bouncing?
// 
// bouncing things
// breakable blocks
// q blocks give money
// walking over coins
// q blocks spawn items that move
// q blocks that require multiple hits
// grow/shrink
// fireball
// enemies
// bullet bill spawner
// 
// make the AABB material system expandable
// 

void print_f32(float x) {
    std::cout << "f32: ";
    std::cout << x << "\n";
}
void print_s32(int x) {
    std::cout << "s32: ";
    std::cout << x << "\n";
}

struct OnCollisionEvent {
	const AABB* myAABB;
	const Transform* myTransform;
	const AABB* collidedAABB;
	const Transform* collidedTransform;
// entityManager: mut ref EntityManager
// scene: mut ref GameScene
// myID: EntityID
// myAABB: ref AABB
// myTransform: ref Transform
// collidedID: EntityID
// collidedAABB: ref AABB
// collidedTransform: ref Transform
};
void print_transform(OnCollisionEvent* t) {
    std::cout << "transform pos: ";
    std::cout << size_t(t->myTransform) << " -> ";
    std::cout << t->myTransform->position.x << " ";
    std::cout << t->myTransform->position.y << "\n";
}
struct OnCollisionHandler {
	typedef std::function<void(VM&, VMFixedStack&, OnCollisionEvent*)> FHandler;

	std::shared_ptr<Program> script;
	std::shared_ptr<VMFixedStack> state;
	FHandler handler;
	OnCollisionHandler() {}
	OnCollisionHandler(std::shared_ptr<Program> sc, std::shared_ptr<VMFixedStack> st, FHandler h) : script(sc), state(st), handler(h) {}
};

GameScene::GameScene(const Tilemap level) :
	_render_texture(),
	_camera(sf::FloatRect(0.f, 0.f, 256.0f, 240.f)),
	_gui_view(sf::FloatRect(0.f, 0.f, 256.0f, 240.f)),
	_level(level),
	_player(0),
	_render_colliders(false),
	_milestone_reached(0),
	_fpsclock(),
	_frames(0)
{
	entity_manager().register_component<Sprite>();
	entity_manager().register_component<Animation>();
	entity_manager().register_component<Transform>();
	entity_manager().register_component<Movement>();
	entity_manager().register_component<AABB>();
	entity_manager().register_component<Sensors>();
	entity_manager().register_component<Mortal>();
	entity_manager().register_component<ZIndex>();
	entity_manager().register_component<CTilemapRenderLayer>();
	entity_manager().register_component<CTilemapParallaxLayer>();
	entity_manager().register_component<OnCollisionHandler>();

	min_screen_x = _camera.getSize().x / 2.0f;
	max_screen_x = (float)(_level.width * _level.tile_width) - min_screen_x;

	RegisterActionSystem(&GameScene::InputSystem);
	RegisterFixedUpdateSystem(&GameScene::AISystem);
	RegisterFixedUpdateSystem(&GameScene::LifetimeSystem);
	RegisterFixedUpdateSystem(&GameScene::GravitySystem);
	RegisterFixedUpdateSystem(&GameScene::MovementSystem);
	RegisterFixedUpdateSystem(&GameScene::DetectCollisionSystem);
	//RegisterFixedUpdateSystem(&GameScene::ResolveCollisionSystem);
	RegisterFixedUpdateSystem(&GameScene::DestructionSystem);
	RegisterFixedUpdateSystem(&GameScene::PlayerDeathSystem);
	RegisterFixedUpdateSystem(&GameScene::PlayerVictorySystem);
	RegisterFixedUpdateSystem(&GameScene::SetPlayerAnimationSystem);
	RegisterFixedUpdateSystem(&GameScene::AnimationSystem);

	RegisterRenderSystem(&GameScene::Render);
	RegisterRenderGUISystem(&GameScene::DrawGUI);
	RegisterRenderGUISystem(&GameScene::DrawBuffer);

	_script_compiler
		.build_struct<sf::Vector2f>("Vec2f")
		.add_member<float>("x", offsetof(sf::Vector2f, x))
		.add_member<float>("y", offsetof(sf::Vector2f, y))
		.build();

	_script_compiler
		.build_struct<AABB>("AABB")
		.add_member<sf::Vector2f>("size", offsetof(AABB, size))
		.add_member<sf::Vector2f>("half_size", offsetof(AABB, half_size))
		.add_member<sf::Vector2f>("previous_position", offsetof(AABB, previous_position))
		.add_member<sf::Vector2f>("previous_velocity", offsetof(AABB, previous_velocity))
		.build();

	_script_compiler
		.build_struct<Transform>("Transform")
		.add_member<sf::Vector2f>("pos", offsetof(Transform, position))
		.build();

    _script_compiler
		.build_struct<OnCollisionEvent>("OnCollisionEvent")
		.add_member<const AABB*>("myAABB", offsetof(OnCollisionEvent, myAABB))
		.add_member<const Transform*>("myTransform", offsetof(OnCollisionEvent, myTransform))
		.add_member<const AABB*>("collidedAABB", offsetof(OnCollisionEvent, collidedAABB))
		.add_member<const Transform*>("collidedTransform", offsetof(OnCollisionEvent, collidedTransform))
		.build();

	_script_compiler.import_method<void,float>("print_f32", print_f32);
	_script_compiler.import_method<void,int>("print_s32", print_s32);
	_script_compiler.import_method<void,OnCollisionEvent*>("print_transform", print_transform);

	_script_vm = std::make_shared<VM>(VMSTACK_PAGE_SIZE);
}

GameScene::~GameScene() {}

std::shared_ptr<Program> GameScene::GetScript(std::string name) {
	auto f = _cached_scripts.find(name);
	if (f != _cached_scripts.end()) {
		return f->second;
	}

    std::ifstream infile(name);
    std::string contents(
        (std::istreambuf_iterator<char>(infile)),
        (std::istreambuf_iterator<char>())
    );
	std::cout << "Compile " << name << "\n";
	std::cout << contents << "\n";

	auto prog = _script_compiler.compile(name, contents);
	_cached_scripts[name] = prog;
	return prog;
}

std::optional<SceneError> GameScene::Load(GameManager& gm) {
	if (!_render_texture.create(256, 240)) {
		return SceneError("Failed to create render destination");
	}

	AssetManager& asset_manager = gm.asset_manager();

	auto font = asset_manager.get_font("Roboto");
	_fps_text = sf::Text("0 fps", *font, 16);
	_fps_text.setPosition(200.0f, 5.0f);

	const float item_size = 16.0f;
	const float item_half = item_size / 2.0f;
	_milestone_reached = 0;

	// Sort all transforms by the X pos to make it easier to only render some items.
	entity_manager().sort<Transform>([](const Transform& t1, const Transform& t2) {
		return t1.position.x < t2.position.x;
	});
	// Sort such that higher z indices are drawn on top.
	entity_manager().sort<ZIndex>([](const ZIndex& t1, const ZIndex& t2) {
		return t1.z_index < t2.z_index;
	});

	auto animation_name = MARIO_FALL_ANIMATION;
	auto& tex = asset_manager.get_spritesheet_texture(MARIO_SPRITESHEET);
	auto spconfig = asset_manager.get_spritesheet_entry(MARIO_SPRITESHEET, animation_name);

	auto mario = entity_manager().entity();
	entity_manager().add<Mortal>(mario, PLAYER_STARTING_HEALTH);
	entity_manager().add<Movement>(mario, 0.0f, 0.0f);
	entity_manager().add<Transform>(mario,
		(float)(_level.milestones[_milestone_reached].x * _level.tile_width) + item_half,
		(float)(_level.milestones[_milestone_reached].y * _level.tile_height) + item_half);
	entity_manager().add<AABB>(mario,
		sf::Vector2f(_level.player.aabb.width, _level.player.aabb.height),
		AABB::Material::Solid,
		1,
		ENTITY_HARDNESS,
		PLAYER_SMALL_PIERCE);
	entity_manager().add<Sensors>(mario);
	entity_manager().add<ZIndex>(mario, _level.player.layer);
	entity_manager().add<Sprite>(mario, tex, sf::FloatRect((float)spconfig.x, (float)spconfig.y, (float)spconfig.width, (float)spconfig.height), sf::Vector2f(0.5f, 0.5f));
	entity_manager().add<Animation>(mario,
		animation_name,
		spconfig,
		true,
		"",
		false
		);
	_player = mario;

	// add non-deadly world AABBs
	float world_w = (float)(_level.width * _level.tile_width);
	float world_h = (float)(_level.height * _level.tile_height);
	float world_half_w = world_w / 2.0f;
	float world_half_h = world_h / 2.0f;

	// left boundary
	auto world_aabbs = entity_manager().entity();
	entity_manager().add<Transform>(world_aabbs, -1.0f, world_half_h);
	entity_manager().add<AABB>(world_aabbs, sf::Vector2f(2.0f, world_h), AABB::Material::Solid);
	// right boundary
	world_aabbs = entity_manager().entity();
	entity_manager().add<Transform>(world_aabbs, world_w + 1.0f, world_half_h);
	entity_manager().add<AABB>(world_aabbs, sf::Vector2f(2.0f, world_h), AABB::Material::Solid);
	// no there is not a top boundary. this is explicit.

	// add a deadly world AABB at the bottom
	// or a bunch cause of the optimization to limit searching...
	for (unsigned int i = 0; i < _level.width; i++) {
		float x = (float)(i * _level.tile_width + (_level.tile_width / 2));
		float w = (float)_level.tile_width;
		world_aabbs = entity_manager().entity();
		entity_manager().add<Transform>(world_aabbs, x, world_h + 1.0f);
		entity_manager().add<AABB>(world_aabbs, sf::Vector2f(w, 2.0f), AABB::Material::Permeable, 999, 999, 999);
	}

	// generate_components(_level, entity_manager(), gm.asset_manager());
	for (unsigned int i = 0; i < _level.layers.size(); i++) {
		const auto& layer = _level.layers[i];

		if (layer.tileset.texture.length() > 0) {
			auto mape = entity_manager().entity();
			sf::Texture& t = gm.asset_manager().get_texture(layer.tileset.texture);
			entity_manager().add<CTilemapRenderLayer>(mape, _level, i, t);
			entity_manager().add<Transform>(mape, 0.0f, 0.0f);
			entity_manager().add<ZIndex>(mape, i);
			if (layer.parallax != 1.0f) {
				entity_manager().add<CTilemapParallaxLayer>(mape, layer.parallax);
			}
		}

		float half_w = (float)_level.tile_width / 2.0f;
		float half_h = (float)_level.tile_height / 2.0f;

		// add AABBs
		for (const auto& tile : layer.tiles) {
			if (tile.id <= 0) {
				continue;
			}
			const auto& tile_info = layer.tileset.tiles[tile.id - 1];
			if (tile_info.aabb.width > 0 && tile_info.aabb.height > 0) {
				AABB::Material m;
				if (tile_info.passage) {
					m = AABB::Material::Permeable;
				}
				else {
					m = AABB::Material::Solid;
				}
				auto e = entity_manager().entity();
				entity_manager().add<Transform>(e, (float)(tile.x * _level.tile_width) + half_w, (float)(tile.y * _level.tile_height) + half_h);
				entity_manager().add<AABB>(
					e,
					sf::Vector2f(tile_info.aabb.width, tile_info.aabb.height),
					m, tile_info.damage, tile_info.hardness, tile_info.piercing);
				if (tile.events.size() > 0) {
					std::shared_ptr<Program> script = GetScript(tile.events[0].script);
					auto handler = script->method<void,OnCollisionEvent*>("onCollide");
					std::shared_ptr<VMFixedStack> state = script->generate_state();
					for (auto it : tile.events[0].vars) {
						if (std::holds_alternative<int>(it.second)) {
					        auto address = script->get_global_address(it.first);
							*state->at<int>(address) = std::get<int>(it.second);
						}
						else if (std::holds_alternative<float>(it.second)) {
					        auto address = script->get_global_address(it.first);
							*state->at<float>(address) = std::get<float>(it.second);
						}
					}

					std::cout << "Add handler " << tile.events[0].script << " for " << e << "\n";
					entity_manager().add<OnCollisionHandler>(
						e,
						script, state, handler
					);
				}
			}
		}
	}

	// colliders that are not solid (lava, victory zones)
	// and bouncy colliders!
	// add victory AABB
	// destructable blocks
	// coin spawner blocks
	// items + item spawner blocks
	// enemies
	// warp zones
	// score
	// physics

	entity_manager().finalize_update();
	return {};
}

std::optional<SceneError> GameScene::Unload(GameManager& gm) {
	_player = 0;
	gm.SetBackgroundColor(sf::Color::Black);
	return {};
}

std::optional<SceneError> GameScene::Show(GameManager& gm) {
	_fpsclock.restart();
	gm.SetBackgroundColor(sf::Color(92, 148, 252));
	// gm.SetCamera(_camera);

	std::unordered_map<sf::Keyboard::Key, ActionType> actions;
	actions[sf::Keyboard::Key::Up] = ActionType::UP;
	actions[sf::Keyboard::Key::Down] = ActionType::DOWN;
	actions[sf::Keyboard::Key::Left] = ActionType::LEFT;
	actions[sf::Keyboard::Key::Right] = ActionType::RIGHT;
	actions[sf::Keyboard::Key::W] = ActionType::UP;
	actions[sf::Keyboard::Key::S] = ActionType::DOWN;
	actions[sf::Keyboard::Key::A] = ActionType::LEFT;
	actions[sf::Keyboard::Key::D] = ActionType::RIGHT;
	actions[sf::Keyboard::Key::LShift] = ActionType::SHOOT;
	actions[sf::Keyboard::Key::Space] = ActionType::JUMP;
	actions[sf::Keyboard::Key::Enter] = ActionType::SELECT;
	actions[sf::Keyboard::Key::P] = ActionType::PAUSE;
	actions[sf::Keyboard::Key::G] = ActionType::GRID;
	actions[sf::Keyboard::Key::Escape] = ActionType::MENU;
	gm.SetActions(actions);
	return {};
}

// Get user input and translate to movement on the player
// Components: Velocity*, BulletSpawner
// May spawn with: Velocity, AABB, Lifetime, Animation
void GameScene::InputSystem(GameManager& gm, const std::vector<Action>& actions, const std::unordered_map<ActionType, ActionState>& action_states) {
	auto smq = entity_manager().query<Sensors, Movement>();
	auto it = smq.find(_player);

	const Sensors& s = it.value<Sensors>();

	if (action_states.find(ActionType::LEFT)->second == ActionState::START) {
		if (!s.left) {
			it.mut<Movement>().velocity.x = -_level.player.run_speed;
		}
	}
	else if (action_states.find(ActionType::RIGHT)->second == ActionState::START) {
		if (!s.right) {
			it.mut<Movement>().velocity.x = _level.player.run_speed;
		}
	}
	else {
		it.mut<Movement>().velocity.x = 0;
	}

	if (action_states.find(ActionType::JUMP)->second == ActionState::START) {
		if (!s.top && s.bottom) {
			it.mut<Movement>().velocity.y = -_level.player.jump_speed;
		}
	}

	for (auto action : actions) {
		switch (action.type) {
		case ActionType::GRID:
			if (action.state == ActionState::END) {
				_render_colliders = !_render_colliders;
			}
			break;
		case ActionType::SHOOT:
			break;
		case ActionType::SELECT:
			break;
		case ActionType::MENU:
			gm.PopScene();
			break;
		}
	}
}

// Determine how on-screen enemies should move
// Components: Velocity*
void GameScene::AISystem(GameManager& gm) {}

// Update objects with limited lifetime and destroy afterwards
// Components: Lifetime*
void GameScene::LifetimeSystem(GameManager& gm) {}

// Make objects fall if subject to gravity
// Components: Velocity*, Gravity
void GameScene::GravitySystem(GameManager& gm) {
	auto smq = entity_manager().query<Sensors, Movement>();
	auto it = smq.find(_player);

	const Sensors& s = it.value<Sensors>();
	const Movement& m = it.value<Movement>();

	if (!s.bottom) {
		if (m.velocity.y < _level.player.fall_speed) {
			it.mut<Movement>().velocity.y = fmin(_level.player.fall_speed, m.velocity.y + _level.gravity);
		}
	}
	else if (m.velocity.y > 0.0f) {
		it.mut<Movement>().velocity.y = 0.0f;
	}
}
// Move objects with velocity
// Components: Velocity, Position*
void GameScene::MovementSystem(GameManager& gm) {
	auto atq = entity_manager().query<AABB, Transform>();
	for (auto it = atq.begin(); it != atq.end(); ++it) {
		AABB& aabb = it.mut<AABB>();
		const Transform& t = it.value<Transform>();
		aabb.previous_position = t.position;
	}

	auto mtsq = entity_manager().query<Movement, Transform, Sensors>().optional<Sensors>();
	for (auto it = mtsq.begin(); it != mtsq.end(); ++it) {
		if (it.has<Sensors>()) {
			const Sensors& s = it.value<Sensors>();
			Movement& m = it.mut<Movement>();
			if (m.velocity.x > 0 && s.right) {
				m.velocity.x = 0.0f;
			}
			if (m.velocity.x < 0 && s.left) {
				m.velocity.x = 0.0f;
			}
			if (m.velocity.y > 0 && s.bottom) {
				m.velocity.y = 0.0f;
			}
			if (m.velocity.y < 0 && s.top) {
				m.velocity.y = 0.0f;
			}
		}

		const Movement& m = it.value<Movement>();
		Transform& t = it.mut<Transform>();
		t.position.x += m.velocity.x;
		t.position.y += m.velocity.y;
	}

	const Transform& t = entity_manager().get<Transform>(_player);
	for (unsigned int i = _milestone_reached; i < _level.milestones.size(); i++) {
		float mx = (float)(_level.milestones[i].x * _level.tile_width);
		if (t.position.x > mx) {
			_milestone_reached = i;
		}
		else {
			break;
		}
	}
}

//std::tuple<bool, sf::Vector2f> overlap(AABB* aabb1, Transform* t1, AABB* aabb2, Transform* t2) {
std::tuple<bool, sf::Vector2f> overlap(
	const sf::Vector2f& half_size1, const sf::Vector2f& pos1,
	const sf::Vector2f& half_size2, const sf::Vector2f& pos2) {

	float total_width = half_size1.x + half_size2.x;
	float nx = pos1.x - pos2.x;
	// overlap_x will be >0 when there is an overlap, which must be true cause of what I did above.
	float overlap_x = total_width - fabs(nx);
	if (overlap_x <= 0) {
		return std::make_tuple(false, sf::Vector2f(0.0f, 0.0f));
	}
	// nx is <0 when A is to the left of B.
	if (nx < 0) {
		overlap_x = -overlap_x;
	}

	float total_height = half_size1.y + half_size2.y;
	float ny = pos1.y - pos2.y;
	float overlap_y = total_height - fabs(ny);
	if (overlap_y <= 0) {
		return std::make_tuple(false, sf::Vector2f(0.0f, 0.0f));
	}
	if (ny < 0) {
		overlap_y = -overlap_y;
	}

	return std::make_tuple(true, sf::Vector2f(overlap_x, overlap_y));
}

// Detects overlap of AABBs, but does not resolve. just stores results.
// Components: AABB, Collision*
void GameScene::DetectCollisionSystem(GameManager& gm) {
	auto atq = entity_manager().query<Transform, AABB>();
	auto atq_end = atq.end();

	for (auto it = atq.begin(); it != atq_end; ++it) {
		AABB& aabb = it.mut<AABB>();
		const Transform& t = it.value<Transform>();
		aabb.collision = false;
		aabb.previous_velocity.x = t.position.x - aabb.previous_position.x;
		aabb.previous_velocity.y = t.position.y - aabb.previous_position.y;
	}

	// TODO: entities could be pushed inside of another moved entity... and boom
	// Using a separate query for the optional pieces to only load when needed.
	auto mmq = entity_manager().query<Mortal, Movement>().optional<Mortal>().optional<Movement>();
	for (auto it = atq.begin(); it != atq_end; ++it) {
		MattECS::EntityID e1 = it.entity();
		const AABB& aabb1 = it.value<AABB>();
		const Transform& t1 = it.value<Transform>();

		for (auto it2 = atq.find(e1); it2 != atq_end; ++it2) {
			MattECS::EntityID e2 = it2.entity();
			if (e1 == e2) {
				continue;
			}
			const AABB& aabb2 = it2.value<AABB>();
			const Transform& t2 = it2.value<Transform>();

			float too_far_away = aabb1.half_size.x + aabb2.half_size.x + 16.0f;
			if (t2.position.x > t1.position.x + too_far_away) {
				break;
			}

			bool has_overlap;
			sf::Vector2f how_much;
			std::tie(has_overlap, how_much) = overlap(aabb1.half_size, t1.position, aabb2.half_size, t2.position);

			if (has_overlap) {
				it.mut<AABB>().collision = true;
				it2.mut<AABB>().collision = true;

				if (auto maybehandler = entity_manager().tryGet<OnCollisionHandler>(e1)) {
					auto handler = maybehandler.value();
					OnCollisionEvent evt = {&aabb1, &t1, &aabb2, &t2};
					std::cout << "adding pos " << size_t(&t1) << " | " << size_t(&evt) << "\n";
					handler->handler(*_script_vm, *handler->state, &evt);
				}
				if (auto maybehandler = entity_manager().tryGet<OnCollisionHandler>(e2)) {
					auto handler = maybehandler.value();
					OnCollisionEvent evt = {&aabb2, &t2, &aabb1, &t1};
					std::cout << "adding pos " << size_t(&t2) << " | " << size_t(&evt) << "\n";
					handler->handler(*_script_vm, *handler->state, &evt);
				}

				auto mmit1 = mmq.find(e1);
				bool is_dynamic1 = mmit1.has<Movement>();
				bool is_deadly1 = aabb1.damage > 0.0f;
				bool is_mortal1 = mmit1.has<Mortal>();

				auto mmit2 = mmq.find(e2);
				bool is_dynamic2 = mmit2.has<Movement>();
				bool is_deadly2 = aabb2.damage > 0.0f;
				bool is_mortal2 = mmit2.has<Mortal>();

				// lava instantly kills any mortal that touches it
				// fireballs remove 1 hp from any mortal that touches it AND the fireball dies
				// lavaball removes 1 hp from any mortal that touches it, but does not die itself
				// between two mortals neither are player:
				//   nothing happens
				// between mortal and player
				//   top = the one with y velocity > 0 (falling) OR the not-player
				//   bottom = the one that is not the top
				//   the bottom loses 1 hp
				//   if the bottom dies, then the top keeps falling
				//   else the top bounces up
				// after all then, then resolve collisions
				// the unit AI will handle turning etc

				// if both deadly and both mortal, resolve who "dies"
				// if 1 deadly and other is mortal

				if (is_deadly1 && is_mortal2 && aabb1.piercing >= aabb2.hardness) {
					mmit2.mut<Mortal>().health -= aabb1.damage;
				}
				if (is_deadly2 && is_mortal1 && aabb2.piercing >= aabb1.hardness) {
					mmit1.mut<Mortal>().health -= aabb2.damage;
				}

				// if either is permeable, then we don't adjust positions.
				if (aabb1.material == AABB::Material::Permeable || aabb2.material == AABB::Material::Permeable) {
					continue;
				}

				if (is_dynamic1 || is_dynamic2) {
					float vel_x_1 = is_dynamic1 ? (t1.position.x - aabb1.previous_position.x) : 0.0f;
					float vel_y_1 = is_dynamic1 ? (t1.position.y - aabb1.previous_position.y) : 0.0f;
					float vel_x_2 = is_dynamic2 ? (t2.position.x - aabb2.previous_position.x) : 0.0f;
					float vel_y_2 = is_dynamic2 ? (t2.position.y - aabb2.previous_position.y) : 0.0f;

					if (vel_x_1 == 0 && vel_y_1 == 0 && vel_x_2 == 0 && vel_y_2 == 0) {
						continue;
					}

					// we want to find when the two entities are JUST overlapping.
					// how much tells us how far to move them apart.
					// it's also how much overlap there is

					// halfsizes = abs(spos1.x + vel1.x * t - (spos2.x + vel2.x * t))
					// the rest assume's spos1.x > spos2.x, just negate halfsizes if otherwise.
					// halfsizes + spos2.x + vel2.x * t = spos1.x + vel1.x * t
					// halfsizes + spos2.x + vel2.x * t - spos1.x = vel1.x * t
					// halfsizes + spos2.x - spos1.x = vel1.x * t - vel2.x * t
					// (halfsizes + spos2.x - spos1.x) / (vel1.x - vel2.x) = t
					// if vel1.x == vel2.x, there is no solution. they're moving in the same direction.

					float desired_x = aabb1.half_size.x + aabb2.half_size.x;
					float desired_y = aabb1.half_size.y + aabb2.half_size.y;

					if (aabb2.previous_position.x > aabb1.previous_position.x) {
						desired_x = -desired_x;
					}
					if (aabb2.previous_position.y > aabb1.previous_position.y) {
						desired_y = -desired_y;
					}

					float tx = 1.0f;
					float ty = 1.0f;
					if (how_much.x != 0.0f && (vel_x_1 != 0.0 || vel_x_2 != 0.0) && vel_x_1 != vel_x_2) {
						float t = (desired_x + aabb2.previous_position.x - aabb1.previous_position.x) / (vel_x_1 - vel_x_2);
						if (t >= 0.0f && t < 1.0f) {
							tx = t;
						}
					}
					if (how_much.y != 0.0f && (vel_y_1 != 0.0 || vel_y_2 != 0.0) && vel_y_1 != vel_y_2) {
						float t = (desired_y + aabb2.previous_position.y - aabb1.previous_position.y) / (vel_y_1 - vel_y_2);
						if (t >= 0.0f && t < 1.0f) {
							ty = t;
						}
					}
					// we use the smallest T to make sure both are satisifed.
					// the smallest T will be the closest to the original position
					// tx/ty will be 0 if it cannot be satisfied this frame, but we assume
					// that the other will satisfy since the object JUST became overlapped.
					auto t = fmin(tx, ty);

					if (t >= 0.0 && t < 1.0f) {
						if (is_dynamic1 && (vel_x_1 != 0.0f || vel_y_1 != 0.0)) {
							Transform& tr = it.mut<Transform>();
							tr.position.x = aabb1.previous_position.x + vel_x_1 * t;
							tr.position.y = aabb1.previous_position.y + vel_y_1 * t;
						}
						if (is_dynamic2 && (vel_x_2 != 0.0f || vel_y_2 != 0.0)) {
							Transform& tr = it2.mut<Transform>();
							tr.position.x = aabb2.previous_position.x + vel_x_2 * t;
							tr.position.y = aabb2.previous_position.y + vel_y_2 * t;
						}
					}
				}
			}

		}
	}

	auto staq = entity_manager().query<Sensors, Transform, AABB>();
	for (auto it = staq.begin(); it != staq.end(); ++it) {
		auto e1 = it.entity();
		const AABB& aabb1 = it.value<AABB>();
		const Transform& t1 = it.value<Transform>();
		Sensors& s = it.mut<Sensors>();

		const float sensor_dist = 1;

		sf::Vector2f h_sensor_size = sf::Vector2f(sensor_dist, aabb1.half_size.y);
		sf::Vector2f w_sensor_size = sf::Vector2f(aabb1.half_size.x, sensor_dist);

		sf::Vector2f left_sensor = sf::Vector2f(t1.position.x - aabb1.half_size.x, t1.position.y);
		sf::Vector2f right_sensor = sf::Vector2f(t1.position.x + aabb1.half_size.x, t1.position.y);
		sf::Vector2f top_sensor = sf::Vector2f(t1.position.x, t1.position.y - aabb1.half_size.y);
		sf::Vector2f bottom_sensor = sf::Vector2f(t1.position.x, t1.position.y + aabb1.half_size.y);

		s.left = false;
		s.right = false;
		s.top = false;
		s.bottom = false;

		for (auto it2 = atq.begin(); it2 != atq.end(); ++it2) {
			auto e2 = it2.entity();
			const AABB& aabb2 = it2.value<AABB>();
			const Transform& t2 = it2.value<Transform>();

			if (e1 == e2) {
				continue;
			}

			if (aabb2.material == AABB::Material::Permeable) {
				continue;
			}

			auto check = overlap(h_sensor_size, left_sensor, aabb2.half_size, t2.position);
			if (std::get<0>(check)) {
				s.left = true;
			}
			check = overlap(h_sensor_size, right_sensor, aabb2.half_size, t2.position);
			if (std::get<0>(check)) {
				s.right = true;
			}
			check = overlap(w_sensor_size, top_sensor, aabb2.half_size, t2.position);
			if (std::get<0>(check)) {
				s.top = true;
			}
			check = overlap(w_sensor_size, bottom_sensor, aabb2.half_size, t2.position);
			if (std::get<0>(check)) {
				s.bottom = true;
			}
		}
	}
}

// Resolve overlapping AABBs by shifting moving objects.
// Components: Collision, Velocity*, Position*
void GameScene::ResolveCollisionSystem(GameManager& gm) {}
// Destroy things that are destructable when hit by a collision in a specific manner
// Components: Collision, FragmentSpawner
// May spawn with: Velocity, Lifetime, Animation
void GameScene::DestructionSystem(GameManager& gm) {
	bool respawn_player = false;
	auto q = entity_manager().query<Mortal>();
	for (auto it = q.begin(); it != q.end(); ++it) {
		if (it.value<Mortal>().health <= 0) {
			if (it.entity() == _player) {
				// TODO: animation & respawn
				auto mtmq = entity_manager().query<Movement, Transform>();
				auto pit = mtmq.find(_player);

				Mortal& h = it.mut<Mortal>();
				Movement& m = pit.mut<Movement>();
				Transform& t = pit.mut<Transform>();

				h.health = PLAYER_STARTING_HEALTH;
				m.velocity.x = 0.0f;
				m.velocity.y = _level.player.jump_speed;
				t.position.x = (float)(_level.milestones[_milestone_reached].x * _level.tile_width) + 8.0f;
				t.position.y = (float)(_level.milestones[_milestone_reached].y * _level.tile_height) + 8.0f;
			}
			else {
				// TODO: fragment system or items spawner
				// or animation for like goombas
				entity_manager().remove_all(it.entity());
			}
		}
	}
}
// Player dies when falling out of bounds or when touching an enemy
// Components: Collision? or just Position
void GameScene::PlayerDeathSystem(GameManager& gm) {}
// Player wins if hitting the flag pole collider
// Components: Collision? or just Position
void GameScene::PlayerVictorySystem(GameManager& gm) {}

// Set the player's correct sprite/animation sequence
void GameScene::SetPlayerAnimationSystem(GameManager& gm) {
	auto q = entity_manager().query<Movement, Animation, Sprite, Transform>();
	auto it = q.find(_player);

	const Movement& m = it.value<Movement>();
	const Animation& ani = it.value<Animation>();

	if (m.velocity.x < 0) {
		it.mut<Transform>().scale = sf::Vector2f(-1.0f, 1.0f);
	}
	else if (m.velocity.x > 0) {
		it.mut<Transform>().scale = sf::Vector2f(1.0f, 1.0f);
	}

	std::string change_animation = "";
	if (m.velocity.y != 0) {
		change_animation = MARIO_FALL_ANIMATION;
	}
	else if (m.velocity.x != 0) {
		change_animation = MARIO_RUN_ANIMATION;
	}
	else {
		change_animation = MARIO_STAND_ANIMATION;
	}

	if (change_animation != "" && ani.name != change_animation) {
		auto spconfig = gm.asset_manager().get_spritesheet_entry(MARIO_SPRITESHEET, change_animation);

		Animation& ani = it.mut<Animation>();
		Sprite& s = it.mut<Sprite>();

		s.set_rect(sf::FloatRect((float)spconfig.x, (float)spconfig.y, (float)spconfig.width, (float)spconfig.height));
		ani.config = spconfig;
		ani.current_frame = 0;
		ani.destroyAfter = false;
		ani.loop = true;
		ani.name = change_animation;
		ani.next_animation = "";
	}
}
// Run the animations on objects and yes this is FixedUpdate, not render update.
// Components: Animation*
void GameScene::AnimationSystem(GameManager& gm) {
	auto asq = entity_manager().query<Animation, Sprite>();
	for (auto it = asq.begin(); it != asq.end(); ++it) {
		if (it.value<Animation>().config.animation_frames <= 1) {
			continue;
		}
		Animation& ani = it.mut<Animation>();
		Sprite& s = it.mut<Sprite>();
		ani.current_frame++;
		auto actual_frame = ani.current_frame / ani.config.animation_rate;
		if (actual_frame >= ani.config.animation_frames) {
			ani.current_frame = 0;
			actual_frame = 0;
		}

		int x = (actual_frame * (ani.config.width + 1)) + ani.config.x;
		s.set_rect(sf::FloatRect((float)x, (float)ani.config.y, (float)ani.config.width, (float)ani.config.height));
	}

	auto cq = entity_manager().query<CTilemapRenderLayer>();
	for (auto it = cq.begin(); it != cq.end(); ++it) {
		it.mut<CTilemapRenderLayer>().animate();
	}

	float camera_left = _camera.getCenter().x - _camera.getSize().x / 2;
	float camera_top = 0.0f;

	auto ptq = entity_manager().query<CTilemapParallaxLayer, Transform>();
	for (auto it = ptq.begin(); it != ptq.end(); ++it) {
		move_parallax_layer(it.mut<Transform>(), it.value<CTilemapParallaxLayer>(), camera_left, camera_top);
	}
}

// Render all objects, but not the GUI
// Components: Position, Animation
void GameScene::Render(GameManager& gm, sf::RenderWindow& window, int delta_ms) {
	_render_texture.clear(sf::Color(92, 148, 252));

	const Transform& player_t = entity_manager().get<Transform>(_player);
	float y = _camera.getCenter().y;
	float x = fmin(max_screen_x, fmax(min_screen_x, player_t.position.x));
	_camera.setCenter(x, y);
	//gm.SetCamera(_camera);
	_render_texture.setView(_camera);

	auto sq = entity_manager().query<Sprite>();
	auto tq = entity_manager().query<CTilemapRenderLayer>();

	auto ztq = entity_manager().query<ZIndex, Transform>();
	for (auto it = ztq.begin(); it != ztq.end(); ++it) {
		auto entity = it.entity();

		auto sqit = sq.find(entity);
		if (sqit != sq.end()) {
			sqit.value<Sprite>().render(_render_texture, it.value<Transform>().transform());
			continue;
		}

		auto tqit = tq.find(entity);
		if (tqit != tq.end()) {
			tqit.value<CTilemapRenderLayer>().render(_render_texture, it.value<Transform>().transform());
			continue;
		}
	}

	if (_render_colliders) {
		auto atq = entity_manager().query<AABB, Transform>();
		for (auto it = atq.begin(); it != atq.end(); ++it) {
			AABB& aabb = it.mut<AABB>();
			const Transform& t = it.value<Transform>();
			if (aabb.collision) {
				aabb.render_box.setOutlineColor(sf::Color::Red);
			}
			else {
				aabb.render_box.setOutlineColor(sf::Color::White);
			}
			aabb.render_box.setPosition(t.position.x, t.position.y);

			_render_texture.draw(aabb.render_box);
		}
	}
}
// Render the GUI if any
// Components: None, doesn't use entities I don't think.
void GameScene::DrawGUI(GameManager& gm, sf::RenderWindow& window, int delta_ms) {
	//gm.SetCamera(_gui_view);
	_render_texture.setView(_gui_view);
	_frames++;
	float s = _fpsclock.getElapsedTime().asSeconds();
	if (s > 1.0f) {
		int fps = _frames / (int)s;
		_frames = 0;
		_fpsclock.restart();

		std::stringstream fpsbuilder;
		fpsbuilder << fps << " fps";
		_fps_text.setString(fpsbuilder.str());
	}

	_render_texture.draw(_fps_text);
}

void GameScene::DrawBuffer(GameManager& gm, sf::RenderWindow& window, int delta_ms) {
	_render_texture.display();
	sf::Sprite sprite(_render_texture.getTexture());
	float x_scale = (float)window.getSize().x / 256.0f;
	sprite.setScale(x_scale, x_scale);
	window.draw(sprite);
}
