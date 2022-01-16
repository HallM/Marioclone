#include "GameScene.h"

#include <cmath>
#include <sstream>

#include "AssetManager.h"
#include "EntityManager.h"

const int BACKGROUND_Z_INDEX = 1;
const int PLAYER_Z_INDEX = 2;
const int OTHER_DYN_Z_INDEX = 3;
const int LAYERED_Z_INDEX = 4;

const int PLAYER_STARTING_HEALTH = 1;
const int PLAYER_SMALL_PIERCE = 0;
const int PLAYER_BIG_PIERCE = 1;

const int BLOCK_HARDNESS = 1;
const int ENTITY_HARDNESS = 0;

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


GameScene::GameScene(const std::shared_ptr<TileMap> level) :
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

	min_screen_x = _camera.getSize().x / 2.0f;
	max_screen_x = (float)(_level->width * _level->tile_width) - min_screen_x;

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
}

GameScene::~GameScene() {}

std::optional<SceneError> GameScene::Load(GameManager& gm) {
	AssetManager& asset_manager = gm.asset_manager();

	auto font = asset_manager.GetFont("Roboto");
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
	entity_manager().sort<Sprite>([](const Sprite& t1, const Sprite& t2) {
		return t1.z_index < t2.z_index;
	});

	auto animation_name = _level->player.fall_animation;
	auto ani_info = asset_manager.GetAnimation(animation_name);
	auto& tex = std::get<sf::Texture&>(ani_info);
	auto spconfig = std::get<SpriteAnimationConfig>(ani_info);

	auto mario = entity_manager().entity();
	entity_manager().add<Mortal>(mario, PLAYER_STARTING_HEALTH);
	entity_manager().add<Movement>(mario, 0.0f, 0.0f);
	entity_manager().add<Transform>(mario,
		(float)(_level->milestones[_milestone_reached].x * _level->tile_width) + item_half,
		(float)(_level->milestones[_milestone_reached].y * _level->tile_height) + item_half);
	entity_manager().add<AABB>(mario,
		sf::Vector2f(_level->player.aabb_width, _level->player.aabb_height),
		AABB::Material::Solid,
		1,
		ENTITY_HARDNESS,
		PLAYER_SMALL_PIERCE);
	entity_manager().add<Sensors>(mario);
	entity_manager().add<Sprite>(mario, tex, sf::FloatRect((float)spconfig.x, (float)spconfig.y, (float)spconfig.w, (float)spconfig.h), sf::Vector2f(0.5f, 0.5f), PLAYER_Z_INDEX);
	entity_manager().add<Animation>(mario,
		animation_name,
		spconfig,
		true,
		"",
		false
		);
	_player = mario;

	// add non-deadly world AABBs
	float world_w = (float)(_level->width * _level->tile_width);
	float world_h = (float)(_level->height * _level->tile_height);
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
	for (int i = 0; i < _level->width; i++) {
		float x = (float)(i * _level->tile_width + (_level->tile_width / 2));
		float w = (float)_level->tile_width;
		world_aabbs = entity_manager().entity();
		entity_manager().add<Transform>(world_aabbs, x, world_h + 1.0f);
		entity_manager().add<AABB>(world_aabbs, sf::Vector2f(w, 2.0f), AABB::Material::Permeable, 999, 999, 999);
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

	int total = 0;
	float x = 0;
	float y = 0;
	float tile_width_half = _level->tile_width / 2.0f;
	float tile_height_half = _level->tile_height / 2.0f;
	for (auto i : _level->tilemap) {
		if (i > 0) {
			auto t = _level->tile_types[i - 1];

			auto ani_info = asset_manager.GetAnimation(t.animation_name);
			auto& tex = std::get<sf::Texture&>(ani_info);
			auto spconfig = std::get<SpriteAnimationConfig>(ani_info);

			auto entity = entity_manager().entity();
			entity_manager().add<Transform>(entity, x + tile_width_half, y + tile_height_half);
			if (t.aabb_width > 0 && t.aabb_height > 0) {
				entity_manager().add<AABB>(entity, sf::Vector2f(t.aabb_width, t.aabb_height), AABB::Material::Solid, 0, BLOCK_HARDNESS, 0);
			}
			entity_manager().add<Sprite>(entity, tex, sf::FloatRect((float)spconfig.x, (float)spconfig.y, (float)spconfig.w, (float)spconfig.h), sf::Vector2f(0.5f, 0.5f), BACKGROUND_Z_INDEX);
			entity_manager().add<Animation>(entity,
				t.animation_name,
				spconfig,
				true,
				"",
				false
			);
		}

		total++;
		x += _level->tile_width;
		if (x >= (_level->width * _level->tile_width)) {
			x = 0.0f;
			y += _level->tile_height;
		}
	}
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
	gm.SetCamera(_camera);

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

	const Sensors* s = it.value<Sensors>();

	if (action_states.find(ActionType::LEFT)->second == ActionState::START) {
		if (!s->left) {
			it.mut<Movement>()->velocity.x = -_level->player.horizontal_speed;
		}
	}
	else if (action_states.find(ActionType::RIGHT)->second == ActionState::START) {
		if (!s->right) {
			it.mut<Movement>()->velocity.x = _level->player.horizontal_speed;
		}
	}
	else {
		it.mut<Movement>()->velocity.x = 0;
	}

	if (action_states.find(ActionType::JUMP)->second == ActionState::START) {
		if (!s->top && s->bottom) {
			it.mut<Movement>()->velocity.y = -_level->player.jump_speed;
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

	const Sensors* s = it.value<Sensors>();
	const Movement* m = it.value<Movement>();

	if (!s->bottom) {
		if (m->velocity.y < _level->player.terminal_velocity) {
			it.mut<Movement>()->velocity.y = fmin(_level->player.terminal_velocity, m->velocity.y + _level->player.gravity);
		}
	}
	else if (m->velocity.y > 0.0f) {
		it.mut<Movement>()->velocity.y = 0.0f;
	}
}
// Move objects with velocity
// Components: Velocity, Position*
void GameScene::MovementSystem(GameManager& gm) {
	auto atq = entity_manager().query<AABB, Transform>();
	for (auto it = atq.begin(); it != atq.end(); ++it) {
		AABB* aabb = it.mut<AABB>();
		const Transform* t = it.value<Transform>();
		aabb->previous_position = t->position;
	}

	auto mtq = entity_manager().query<Movement, Transform>();
	for (auto it = mtq.begin(); it != mtq.end(); ++it) {
		const Sensors* s = entity_manager().get<Sensors>(it.entity());
		if (s != nullptr) {
			Movement* m = it.mut<Movement>();
			if (m->velocity.x > 0 && s->right) {
				m->velocity.x = 0.0f;
			}
			if (m->velocity.x < 0 && s->left) {
				m->velocity.x = 0.0f;
			}
			if (m->velocity.y > 0 && s->bottom) {
				m->velocity.y = 0.0f;
			}
			if (m->velocity.y < 0 && s->top) {
				m->velocity.y = 0.0f;
			}
		}

		const Movement* m = it.value<Movement>();
		Transform* t = it.mut<Transform>();
		t->position.x += m->velocity.x;
		t->position.y += m->velocity.y;
	}

	const Transform* t = entity_manager().get<Transform>(_player);
	for (unsigned int i = _milestone_reached; i < _level->milestones.size(); i++) {
		float mx = (float)(_level->milestones[i].x * _level->tile_width);
		if (t->position.x > mx) {
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

bool point_in_rect(sf::Vector2f p, AABB* aabb, Transform* t) {
	float left = t->position.x - aabb->half_size.x;
	float right = t->position.x + aabb->half_size.x;
	float top = t->position.y - aabb->half_size.y;
	float bottom = t->position.y + aabb->half_size.y;

	if (p.x >= left && p.x <= right && p.y >= top && p.y <= bottom) {
		return true;
	}
	return false;
}

// Detects overlap of AABBs, but does not resolve. just stores results.
// Components: AABB, Collision*
void GameScene::DetectCollisionSystem(GameManager& gm) {
	auto atq = entity_manager().query<Transform, AABB>();
	auto atq_end = atq.end();

	for (auto it = atq.begin(); it != atq_end; ++it) {
		AABB* aabb = it.mut<AABB>();
		const Transform* t = it.value<Transform>();
		aabb->collision = false;
		aabb->previous_velocity.x = t->position.x - aabb->previous_position.x;
		aabb->previous_velocity.y = t->position.y - aabb->previous_position.y;
	}

	// TODO: entities could be pushed inside of another moved entity... and boom
	// Using a separate query for the optional pieces to only load when needed.
	auto mmq = entity_manager().query<Mortal, Movement>().optional<Mortal>().optional<Movement>();
	for (auto it = atq.begin(); it != atq_end; ++it) {
		MattECS::EntityID e1 = it.entity();
		const AABB* aabb1 = it.value<AABB>();
		const Transform* t1 = it.value<Transform>();
		const Movement* m1;
		bool is_dynamic1;
		bool is_deadly1;
		bool is_mortal1;

		MattECS::EntityID e2;
		const AABB* aabb2;
		const Transform* t2;
		const Movement* m2;
		bool is_dynamic2;
		bool is_deadly2;
		bool is_mortal2;

		for (auto it2 = atq.find(e1); it2 != atq_end; ++it2) {
			e2 = it2.entity();
			if (e1 == e2) {
				continue;
			}
			aabb2 = it2.value<AABB>();
			t2 = it2.value<Transform>();

			float too_far_away = aabb1->half_size.x + aabb2->half_size.x + 16.0f;
			if (t2->position.x > t1->position.x + too_far_away) {
				break;
			}

			bool has_overlap;
			sf::Vector2f how_much;
			std::tie(has_overlap, how_much) = overlap(aabb1->half_size, t1->position, aabb2->half_size, t2->position);

			if (has_overlap) {
				it.mut<AABB>()->collision = true;
				it2.mut<AABB>()->collision = true;

				auto mmit1 = mmq.find(e1);
				m1 = mmit1.value<Movement>();
				is_dynamic1 = m1 != nullptr;
				is_deadly1 = aabb1->damage > 0.0f;
				is_mortal1 = mmit1.value<Mortal>() != nullptr;

				auto mmit2 = mmq.find(e2);
				m2 = mmit2.value<Movement>();
				is_dynamic2 = m2 != nullptr;
				is_deadly2 = aabb2->damage > 0.0f;
				is_mortal2 = mmit2.value<Mortal>() != nullptr;

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

				if (is_deadly1 && is_mortal2 && aabb1->piercing >= aabb2->hardness) {
					mmit2.mut<Mortal>()->health -= aabb1->damage;
					//entity_manager().update<Mortal>(e2, [&aabb1](Mortal* h) { h->health -= aabb1->damage; });
				}
				if (is_deadly2 && is_mortal1 && aabb2->piercing >= aabb1->hardness) {
					mmit1.mut<Mortal>()->health -= aabb2->damage;
					//entity_manager().update<Mortal>(e1, [&aabb2](Mortal* h) { h->health -= aabb2->damage; });
				}

				// if either is permeable, then we don't adjust positions.
				if (aabb1->material == AABB::Material::Permeable || aabb2->material == AABB::Material::Permeable) {
					continue;
				}

				if (is_dynamic1 || is_dynamic2) {
					float vel_x_1 = is_dynamic1 ? (t1->position.x - aabb1->previous_position.x) : 0.0f;
					float vel_y_1 = is_dynamic1 ? (t1->position.y - aabb1->previous_position.y) : 0.0f;
					float vel_x_2 = is_dynamic2 ? (t2->position.x - aabb2->previous_position.x) : 0.0f;
					float vel_y_2 = is_dynamic2 ? (t2->position.y - aabb2->previous_position.y) : 0.0f;

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

					float desired_x = aabb1->half_size.x + aabb2->half_size.x;
					float desired_y = aabb1->half_size.y + aabb2->half_size.y;

					if (aabb2->previous_position.x > aabb1->previous_position.x) {
						desired_x = -desired_x;
					}
					if (aabb2->previous_position.y > aabb1->previous_position.y) {
						desired_y = -desired_y;
					}

					float tx = 1.0f;
					float ty = 1.0f;
					if (how_much.x != 0.0f && (vel_x_1 != 0.0 || vel_x_2 != 0.0) && vel_x_1 != vel_x_2) {
						float t = (desired_x + aabb2->previous_position.x - aabb1->previous_position.x) / (vel_x_1 - vel_x_2);
						if (t >= 0.0f && t < 1.0f) {
							tx = t;
						}
					}
					if (how_much.y != 0.0f && (vel_y_1 != 0.0 || vel_y_2 != 0.0) && vel_y_1 != vel_y_2) {
						float t = (desired_y + aabb2->previous_position.y - aabb1->previous_position.y) / (vel_y_1 - vel_y_2);
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
							Transform* tr = it.mut<Transform>();
							tr->position.x = aabb1->previous_position.x + vel_x_1 * t;
							tr->position.y = aabb1->previous_position.y + vel_y_1 * t;
						}
						if (is_dynamic2 && (vel_x_2 != 0.0f || vel_y_2 != 0.0)) {
							Transform* tr = it2.mut<Transform>();
							tr->position.x = aabb2->previous_position.x + vel_x_2 * t;
							tr->position.y = aabb2->previous_position.y + vel_y_2 * t;
						}
					}
				}
			}

		}
	}

	auto staq = entity_manager().query<Sensors, Transform, AABB>();
	for (auto it = staq.begin(); it != staq.end(); ++it) {
		auto e1 = it.entity();
		const AABB* aabb1 = it.value<AABB>();
		const Transform* t1 = it.value<Transform>();
		Sensors* s = it.mut<Sensors>();

		const float sensor_dist = 1;

		sf::Vector2f h_sensor_size = sf::Vector2f(sensor_dist, aabb1->half_size.y);
		sf::Vector2f w_sensor_size = sf::Vector2f(aabb1->half_size.x, sensor_dist);

		sf::Vector2f left_sensor = sf::Vector2f(t1->position.x - aabb1->half_size.x, t1->position.y);
		sf::Vector2f right_sensor = sf::Vector2f(t1->position.x + aabb1->half_size.x, t1->position.y);
		sf::Vector2f top_sensor = sf::Vector2f(t1->position.x, t1->position.y - aabb1->half_size.y);
		sf::Vector2f bottom_sensor = sf::Vector2f(t1->position.x, t1->position.y + aabb1->half_size.y);

		s->left = false;
		s->right = false;
		s->top = false;
		s->bottom = false;

		for (auto it2 = atq.begin(); it2 != atq.end(); ++it2) {
			auto e2 = it2.entity();
			const AABB* aabb2 = it2.value<AABB>();
			const Transform* t2 = it2.value<Transform>();

			if (e1 == e2) {
				continue;
			}

			if (aabb2->material == AABB::Material::Permeable) {
				continue;
			}

			auto check = overlap(h_sensor_size, left_sensor, aabb2->half_size, t2->position);
			if (std::get<0>(check)) {
				s->left = true;
			}
			check = overlap(h_sensor_size, right_sensor, aabb2->half_size, t2->position);
			if (std::get<0>(check)) {
				s->right = true;
			}
			check = overlap(w_sensor_size, top_sensor, aabb2->half_size, t2->position);
			if (std::get<0>(check)) {
				s->top = true;
			}
			check = overlap(w_sensor_size, bottom_sensor, aabb2->half_size, t2->position);
			if (std::get<0>(check)) {
				s->bottom = true;
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
		if (it.value<Mortal>()->health <= 0) {
			if (it.entity() == _player) {
				// TODO: animation & respawn
				auto mtmq = entity_manager().query<Movement, Transform>();
				auto pit = mtmq.find(_player);

				auto h = it.mut<Mortal>();
				auto m = pit.mut<Movement>();
				auto t = pit.mut<Transform>();

				h->health = PLAYER_STARTING_HEALTH;
				m->velocity.x = 0.0f;
				m->velocity.y = _level->player.jump_speed;
				t->position.x = (float)(_level->milestones[_milestone_reached].x * _level->tile_width) + 8.0f;
				t->position.y = (float)(_level->milestones[_milestone_reached].y * _level->tile_height) + 8.0f;
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

	const Movement* m = it.value<Movement>();
	const Animation* ani = it.value<Animation>();

	if (m->velocity.x < 0) {
		it.mut<Transform>()->scale = sf::Vector2f(-1.0f, 1.0f);
	}
	else if (m->velocity.x > 0) {
		it.mut<Transform>()->scale = sf::Vector2f(1.0f, 1.0f);
	}

	std::string change_animation = "";
	if (m->velocity.y != 0) {
		change_animation = _level->player.fall_animation;
	}
	else if (m->velocity.x != 0) {
		change_animation = _level->player.run_animation;
	}
	else {
		change_animation = _level->player.stand_animation;
	}

	if (change_animation != "" && ani->name != change_animation) {
		auto ani_info = gm.asset_manager().GetAnimation(change_animation);
		auto& tex = std::get<sf::Texture&>(ani_info);
		auto spconfig = std::get<SpriteAnimationConfig>(ani_info);

		Animation* ani = it.mut<Animation>();
		Sprite* s = it.mut<Sprite>();

		s->t = &tex;
		s->set_rect(sf::FloatRect((float)spconfig.x, (float)spconfig.y, (float)spconfig.w, (float)spconfig.h));
		ani->config = spconfig;
		ani->current_frame = 0;
		ani->destroyAfter = false;
		ani->loop = true;
		ani->name = change_animation;
		ani->next_animation = "";
	}
}
// Run the animations on objects and yes this is FixedUpdate, not render update.
// Components: Animation*
void GameScene::AnimationSystem(GameManager& gm) {
	auto asq = entity_manager().query<Animation, Sprite>();
	for (auto it = asq.begin(); it != asq.end(); ++it) {
		if (it.value<Animation>()->config.total_frames <= 1) {
			continue;
		}
		Animation* ani = it.mut<Animation>();
		Sprite* s = it.mut<Sprite>();
		ani->current_frame++;
		auto actual_frame = ani->current_frame / ani->config.frame_ticks;
		if (actual_frame >= ani->config.total_frames) {
			ani->current_frame = 0;
			actual_frame = 0;
		}

		int x = (actual_frame * (ani->config.w + 1)) + ani->config.x;
		s->set_rect(sf::FloatRect((float)x, (float)ani->config.y, (float)ani->config.w, (float)ani->config.h));
	}
}

// Render all objects, but not the GUI
// Components: Position, Animation
void GameScene::Render(GameManager& gm, sf::RenderWindow& window, int delta_ms) {
	const Transform* player_t = entity_manager().get<Transform>(_player);
	if (player_t != nullptr) {
		float y = _camera.getCenter().y;
		float x = fmin(max_screen_x, fmax(min_screen_x, player_t->position.x));
		_camera.setCenter(x, y);
		gm.SetCamera(_camera);
	}

	auto stq = entity_manager().query<Sprite, Transform>();
	for (auto it = stq.begin(); it != stq.end(); ++it) {
		it.value<Sprite>()->render(window, it.value<Transform>()->transform());
	}

	if (_render_colliders) {
		auto atq = entity_manager().query<AABB, Transform>();
		for (auto it = atq.begin(); it != atq.end(); ++it) {
			AABB* aabb = it.mut<AABB>();
			const Transform* t = it.value<Transform>();
			if (aabb->collision) {
				aabb->render_box.setOutlineColor(sf::Color::Red);
			}
			else {
				aabb->render_box.setOutlineColor(sf::Color::White);
			}
			aabb->render_box.setPosition(t->position.x, t->position.y);

			window.draw(aabb->render_box);
		}
	}
}
// Render the GUI if any
// Components: None, doesn't use entities I don't think.
void GameScene::DrawGUI(GameManager& gm, sf::RenderWindow& window, int delta_ms) {
	gm.SetCamera(_gui_view);
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

	window.draw(_fps_text);
}
