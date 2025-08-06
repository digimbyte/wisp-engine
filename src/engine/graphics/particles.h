// particle_canvas.h
#pragma once
#include "engine_common.h"

#define MAX_PARTICLES 64
#define TRAIL_LENGTH 4

// Flag bits
#define PARTICLE_GLOW  0x01
#define PARTICLE_TRAIL 0x02
#define PARTICLE_FADE  0x04

struct Vec2 {
  int16_t x, y;
};

struct Particle {
  int16_t x, y;
  int8_t vx, vy;
  uint32_t startTime;
  uint16_t lifespan;
  uint8_t spriteIndex;
  uint8_t flags;
  uint8_t trailLen;
  Vec2 trail[TRAIL_LENGTH];
  bool active;
};

class ParticleCanvas {
public:
  Particle particles[MAX_PARTICLES];

  void init() {
    memset(particles, 0, sizeof(particles));
  }

  void spawn(int16_t x, int16_t y, int8_t vx, int8_t vy, uint16_t lifespan,
             uint8_t sprite, uint8_t flags) {
    for (int i = 0; i < MAX_PARTICLES; ++i) {
      if (!particles[i].active) {
        Particle& p = particles[i];
        p.x = x;
        p.y = y;
        p.vx = vx;
        p.vy = vy;
        p.startTime = get_millis();
        p.lifespan = lifespan;
        p.spriteIndex = sprite;
        p.flags = flags;
        p.trailLen = (flags & PARTICLE_TRAIL) ? TRAIL_LENGTH : 0;
        memset(p.trail, 0, sizeof(p.trail));
        p.trail[0] = {x, y};
        p.active = true;
        break;
      }
    }
  }

  void updateAndRender(uint8_t* heightmap, void (*draw)(uint8_t sprite, int x, int y, uint8_t brightness)) {
    uint32_t now = get_millis();

    for (int i = 0; i < MAX_PARTICLES; ++i) {
      Particle& p = particles[i];
      if (!p.active) continue;

      uint32_t age = now - p.startTime;
      if (age >= p.lifespan) {
        p.active = false;
        continue;
      }

      // Movement
      p.x += p.vx;
      p.y += p.vy;

      // Trail tracking
      if (p.flags & PARTICLE_TRAIL) {
        memmove(&p.trail[1], &p.trail[0], sizeof(Vec2) * (TRAIL_LENGTH - 1));
        p.trail[0] = {p.x, p.y};
      }

      // Brightness based on fade
      uint8_t brightness = 255;
      if (p.flags & PARTICLE_FADE) {
        brightness = 255 - (age * 255 / p.lifespan);
      }

      // Glow effect (simulated)
      if (p.flags & PARTICLE_GLOW) {
        draw(p.spriteIndex, p.x - 1, p.y, brightness / 6);
        draw(p.spriteIndex, p.x + 1, p.y, brightness / 6);
        draw(p.spriteIndex, p.x, p.y - 1, brightness / 6);
        draw(p.spriteIndex, p.x, p.y + 1, brightness / 6);
      }

      // Trail render
      if (p.flags & PARTICLE_TRAIL) {
        for (int j = p.trailLen - 1; j >= 0; --j) {
          uint8_t tBright = brightness * (p.trailLen - j) / p.trailLen / 2;
          draw(p.spriteIndex, p.trail[j].x, p.trail[j].y, tBright);
        }
      }

      // Final draw with heightmap adjustment
      int renderY = p.y - heightmap[p.x];
      draw(p.spriteIndex, p.x, renderY, brightness);
    }
  }
};
