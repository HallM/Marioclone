#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include <SFML/Graphics.hpp>
#include "../Scriptlang/Program.h"
#include "../Scriptlang/VM.h"
#include "../Scriptlang/VMStack.h"

#include "AssetManager.h"

struct ZIndex {
	int z_index;
	ZIndex() : z_index(0) {}
	ZIndex(int z) : z_index(z) {}
};

struct Gravity {
	Gravity() {}
};

struct LimitedLifetime {
	int frames;
	LimitedLifetime() : frames(0) {}
	LimitedLifetime(int f) : frames(f) {}
};

struct Sprite {
	sf::Texture* t;
	sf::Vertex va[4];
	sf::Vector2f origin;

	Sprite() : t(), va(), origin(0.5f, 0.5f) {}
	// The origin accepts a 0-1 where 0 is top/left and 1 is bottom/right.
	// The midpoint is 0.5,0.5.
	Sprite(sf::Texture* texture, sf::FloatRect texture_rect, sf::Vector2f o) : t(texture), va(), origin(-o.x * texture_rect.width, -o.y * texture_rect.height) {
		set_rect(texture_rect);
	}
	Sprite(sf::Texture* texture, SpriteSheetEntryConfig* sprite) : t(texture), va(), origin(-0.5f * (float)sprite->width, -0.5f * (float)sprite->height) {
		set_rect(sf::FloatRect((float)sprite->x, (float)sprite->y, (float)sprite->width, (float)sprite->height));
	}

	void set_rect(sf::FloatRect texture_rect) {
		va[0].position = sf::Vector2f(0.0f, 0.0f);
		va[1].position = sf::Vector2f(0.0f, texture_rect.height);
		va[2].position = sf::Vector2f(texture_rect.width, 0.0f);
		va[3].position = sf::Vector2f(texture_rect.width, texture_rect.height);

		float left = texture_rect.left;
		float right = left + texture_rect.width;
		float top = texture_rect.top;
		float bottom = top + texture_rect.height;

		va[0].texCoords = sf::Vector2f(left, top);
		va[1].texCoords = sf::Vector2f(left, bottom);
		va[2].texCoords = sf::Vector2f(right, top);
		va[3].texCoords = sf::Vector2f(right, bottom);
	}

	void render(sf::RenderTarget& target, sf::Transform transform) const {
		auto originated_transform = transform.translate(origin);
		sf::RenderStates state;
		state.texture = t;
		state.transform = transform;
		// target.draw(va, state);
		target.draw(va, 4, sf::PrimitiveType::TriangleStrip, state);
	}
};

struct Animation {
	int id;
	SpriteSheetEntryConfig* config;
	unsigned int current_frame;
	bool loop;
	bool destroyAfter;

	Animation() : id(-1), config(), current_frame(0), loop(false), destroyAfter(true) {}
	Animation(
		int _id,
		SpriteSheetEntryConfig* _config,
		bool _loop,
		bool _destroy
	) : id(_id), config(_config), current_frame(0), loop(_loop), destroyAfter(_destroy) {}
};

struct Transform {
	sf::Vector2f position;
	sf::Vector2f scale;
	Transform() : position(0.0f, 0.0f), scale(1.0f, 1.0f) {}
	Transform(float x, float y) : position(x, y), scale(1.0f, 1.0f) {}
	Transform(float x, float y, float sx, float sy) : position(x, y), scale(sx, sy) {}

	sf::Transform transform() const {
		return sf::Transform().translate(position).scale(scale);
	}
};

struct Movement {
	sf::Vector2f velocity;
	Movement() : velocity(0.0f, 0.0f) {}
	Movement(float x, float y) : velocity(x, y) {}
};

struct AABB {
	enum class Material {
		Permeable, // can move through
		Solid,
	};

	sf::Vector2f size;
	sf::Vector2f half_size;
	sf::Vector2f previous_position;
	sf::Vector2f previous_velocity;

	Material material;
	// per frame damage applied to the item
	int damage;
	// hardness and piercing is a way to allow some things to be immune to certain entities.
	int hardness;
	int piercing;
	// TODO: maybe some max speed / acceleration allowed in the zone

	// for rendering:
	// if a collision is currently happening
	bool collision = false;
	sf::RectangleShape render_box;

	AABB() : size(0.0f, 0.0f), previous_position(0.0f, 0.0f), previous_velocity(0.0f, 0.0f), render_box(), material(Material::Permeable), damage(0), hardness(0), piercing(0) {}
	AABB(sf::Vector2f s, Material m, int d = 0, int h = 0, int p = 0) : size(s), previous_position(0.0f, 0.0f), previous_velocity(0.0f, 0.0f), render_box(s), material(m), damage(d), hardness(h), piercing(p) {
		half_size = sf::Vector2f(s.x / 2.0f, s.y / 2.0f);
		render_box.setOrigin(half_size.x, half_size.y);
		render_box.setFillColor(sf::Color::Transparent);
		render_box.setOutlineThickness(1.0f);
	}
};

struct Sensors {
	bool left;
	bool right;
	bool top;
	bool bottom;
	Sensors() : left(false), right(false), top(false), bottom(false) {}
};

struct Mortal {
	int health;
	Mortal() : health(1) {}
	Mortal(int h) : health(h) {}
};
