#pragma once

// maintains a tile map, but as chunks.
// a chunk is NxN tiles in size (yes square)
// We also need to know how many chunks to display (X*Y)
// and potentially some area (view size + 1 on each side)
//
// 1 chunk will be 1 entity
// AABBs will be merged
// 
// On update we need the current center of the view is

// This file provides a Component for tilemap entities
// and Systems to manage the tilemap.
// This assumes there is an AABB and Camera entities.

#include "Components.h"

struct ChunkedTileMap {
};
