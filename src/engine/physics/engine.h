// physics_engine.h
#pragma once
#include <stdint.h>

#define MAX_PHYSICS_QUEUE 128
#define MAX_SHAPES 4

struct BoundingBox {
  int16_t left, top, right, bottom;
};

inline bool intersects(const BoundingBox& a, const BoundingBox& b) {
  return !(a.right < b.left || a.left > b.right ||
           a.bottom < b.top || a.top > b.bottom);
}

struct PhysicsShape {
  BoundingBox physical[MAX_SHAPES];
  BoundingBox trigger[MAX_SHAPES];
};

struct PhysicsIntent {
  int entityId;
  int16_t projectedX, projectedY;
  PhysicsShape projectedShape;
  bool isValid;
};

struct EntityPhysics {
  int id;
  int16_t x, y;
  int8_t dx, dy;
  PhysicsShape shape;
  bool active;
};

class PhysicsEngine {
public:
  PhysicsIntent queue[MAX_PHYSICS_QUEUE];
  int queuedCount = 0;

  void resetQueue() {
    queuedCount = 0;
  }

  bool enqueuePrediction(const EntityPhysics& e) {
    if (queuedCount >= MAX_PHYSICS_QUEUE) return false;

    PhysicsIntent& intent = queue[queuedCount++];
    intent.entityId = e.id;
    intent.projectedX = e.x + e.dx;
    intent.projectedY = e.y + e.dy;
    intent.isValid = true;

    for (int i = 0; i < MAX_SHAPES; ++i) {
      const auto& p = e.shape.physical[i];
      intent.projectedShape.physical[i] = {
        p.left + e.dx, p.top + e.dy,
        p.right + e.dx, p.bottom + e.dy
      };
      const auto& t = e.shape.trigger[i];
      intent.projectedShape.trigger[i] = {
        t.left + e.dx, t.top + e.dy,
        t.right + e.dx, t.bottom + e.dy
      };
    }
    return true;
  }

  void resolveConflicts() {
    for (int i = 0; i < queuedCount; ++i) {
      PhysicsIntent& a = queue[i];
      for (int j = 0; j < queuedCount; ++j) {
        if (i == j) continue;
        PhysicsIntent& b = queue[j];

        for (int k = 0; k < MAX_SHAPES; ++k) {
          if (intersects(a.projectedShape.physical[k], b.projectedShape.physical[k])) {
            a.isValid = false;
          }
          if (intersects(a.projectedShape.trigger[k], b.projectedShape.physical[k])) {
            // trigger logic hook here
          }
        }
      }
    }
  }

  void applyIntent(EntityPhysics& e, const PhysicsIntent& intent) {
    if (intent.isValid) {
      e.x = intent.projectedX;
      e.y = intent.projectedY;
    } else {
      e.dx = 0;
      e.dy = 0;
    }
  }
};
