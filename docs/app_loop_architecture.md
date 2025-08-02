# Game Loop Architecture

The Wisp Engine implements a modern, staged game loop similar to professional game engines. This provides predictable performance, clear separation of concerns, and robust physics/collision handling.

## Game Loop Stages

Each frame executes these stages in order:

1. **STAGE_INPUT_COLLECTION** - Gather input events, update controller state
2. **STAGE_HEARTBEAT** - Backend systems update, timers, state management  
3. **STAGE_LOGIC_UPDATE** - App/game logic execution (Lua scripts)
4. **STAGE_PHYSICS_PREDICTION** - Calculate intended movements and physics
5. **STAGE_COLLISION_DETECTION** - Check for overlaps and collisions
6. **STAGE_PHYSICS_RESOLUTION** - Resolve conflicts, apply final positions
7. **STAGE_TRIGGER_PROCESSING** - Process trigger events and interactions
8. **STAGE_AUDIO_UPDATE** - Update audio systems, apply effects
9. **STAGE_RENDER_PREPARE** - Prepare rendering data, depth sorting
10. **STAGE_RENDER_EXECUTE** - Draw to frame buffer
11. **STAGE_RENDER_PRESENT** - Present frame to display

## Entity System

### Game Entities
Entities represent game objects that can move, collide, and interact:

```cpp
struct GameEntity {
    uint16_t entityId;         // Unique identifier
    BoundingBox bounds;        // Position and size
    int16_t velocityX, velocityY; // Movement per frame
    uint8_t collisionMask;     // What this entity collides with
    uint8_t triggerMask;       // What triggers this entity responds to
    bool active;               // Whether entity is active
    bool kinematic;            // If true, not affected by collision resolution
    
    // Rendering
    uint16_t spriteId;         // Which sprite to display
    uint8_t spriteFrame;       // Frame within sprite sheet
    uint8_t depth;             // Rendering depth (0=front, 12=back)
}
```

### Physics Regions
Regions define areas with special collision or trigger behavior:

```cpp
struct PhysicsRegion {
    BoundingBox bounds;        // Area covered by region
    RegionType type;           // REGION_COLLISION or REGION_TRIGGER
    uint8_t collisionMask;     // What this region collides with
    uint8_t triggerMask;       // What this region triggers with
    uint16_t regionId;         // Unique identifier
    TriggerLogic triggerLogic; // When trigger fires
}
```

## Collision and Trigger System

### Mask Filtering
The system uses bit masks for efficient collision filtering:

- **MASK_PLAYER** (0x01) - Player entities
- **MASK_ENEMY** (0x02) - Enemy entities  
- **MASK_PROJECTILE** (0x04) - Bullets, projectiles
- **MASK_ITEM** (0x08) - Collectible items
- **MASK_ALL** (0x0F) - All mask types

### Collision Types

1. **Solid Collisions** - Block movement, prevent overlap
   - Entity vs Entity collisions
   - Entity vs REGION_COLLISION regions
   
2. **Triggers** - Detect overlap, allow movement
   - Entity vs REGION_TRIGGER regions
   - Generate events for game logic

### Trigger Logic Types

- **TRIGGER_ON_ENTER** - Fire when entity enters region
- **TRIGGER_ON_EXIT** - Fire when entity exits region  
- **TRIGGER_WHILE_INSIDE** - Fire every frame while inside
- **TRIGGER_ON_OVERLAP** - Fire when any overlap detected

## Physics Resolution

The physics system uses a prediction/validation approach:

1. **Prediction Phase** - Calculate where each entity wants to move
2. **Collision Detection** - Check all predicted positions for conflicts
3. **Resolution Phase** - Apply valid movements, block invalid ones

This prevents entities from ever overlapping and provides stable physics.

## Lua API

### Entity Management
```lua
-- Create entities
entityId = createEntity(x, y, width, height, collisionMask, triggerMask)
regionId = createRegion(x, y, width, height, type, mask, triggerLogic)

-- Modify entities
setEntityPosition(entityId, x, y)
setEntityVelocity(entityId, vx, vy)
setEntitySprite(entityId, spriteId, frame)
setEntityDepth(entityId, depth)
setEntityActive(entityId, active)
setEntityMasks(entityId, collisionMask, triggerMask)
setEntityKinematic(entityId, kinematic)

-- Query entities
x, y = getEntityPosition(entityId)
vx, vy = getEntityVelocity(entityId)
collision = checkEntityCollision(entityId1, entityId2)

-- Cleanup
destroyEntity(entityId)
```

### Game Loop Information
```lua
deltaTime = getDeltaTime()        -- Microseconds since last frame
frameCount = getFrameCount()      -- Current frame number  
stage = getCurrentStage()         -- Current stage enum
timings = getStageTimings()       -- Performance data for each stage
```

### Event Handlers
Games should implement these callback functions:

```lua
function gameUpdate()
    -- Called during STAGE_LOGIC_UPDATE
    -- Update game logic, AI, player input, etc.
end

function onCollision(entityId, otherEntityId, regionId)
    -- Called when solid collision occurs
    -- entityId collided with otherEntityId or regionId
end

function onTriggerEnter(entityId, regionId)
    -- Called when entity enters trigger region
end

function onTriggerExit(entityId, regionId)  
    -- Called when entity exits trigger region
end

function onTriggerStay(entityId, regionId)
    -- Called every frame while entity is in trigger
end
```

## Performance Characteristics

### Frame Time Budget (60 FPS = 16.67ms)
- Input Collection: < 0.1ms
- Logic Update: < 8.0ms (Lua execution)
- Physics: < 2.0ms
- Rendering: < 6.0ms
- Audio: < 0.5ms

### Memory Usage
- Max 256 entities
- Max 128 physics regions  
- Per-frame event queue cleared each frame
- Automatic depth sorting for rendering

### Optimization Features
- Mask-based collision filtering reduces checks
- Spatial partitioning for collision detection
- Run-length encoded sprite depth data
- Direct frame buffer rendering

## Best Practices

### Entity Design
- Use kinematic entities for static objects that never move
- Set appropriate collision/trigger masks to avoid unnecessary checks
- Keep entity counts reasonable (< 100 active entities)

### Physics Regions
- Use large trigger regions for area effects
- Create boundary collision regions around play area
- Minimize overlapping regions for better performance

### Performance Monitoring
- Check stage timings regularly during development
- Optimize Lua code in gameUpdate() - it's the biggest time consumer
- Use sprite sheets and depth sorting efficiently

### Memory Management  
- Destroy entities when no longer needed
- Reuse entity IDs when possible
- Keep Lua garbage collection in mind for frequent allocations

## Example Usage

See `examples/example_game.lua` for a complete working game that demonstrates:
- Player movement with collision
- Enemy AI and spawning
- Projectile physics
- Item collection
- Trigger-based events
- Performance monitoring
