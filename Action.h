#pragma once

enum class ActionType {
	UP,
	DOWN,
	LEFT,
	RIGHT,
	JUMP,
	SHOOT,

	GRID,
	MENU,
	SELECT,

	PAUSE
};

enum class ActionState {
	START,
	CONTINUE,
	END
};

struct Action {
	ActionType type;
	ActionState state;
};
