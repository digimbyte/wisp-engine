// engine/sprite_layers.cpp
#include "sprite_layers.h"
#include <algorithm>

// Global layer system instance
WispSpriteLayerSystem* g_LayerSystem = nullptr;

// Layer names for debugging
const char* getLayerName(WispSpriteLayer layer) {
    switch (layer) {
        case LAYER_0_GRADIENTS: return "Gradients";
        case LAYER_1_BACKGROUNDS: return "Backgrounds";
        case LAYER_2_GAME_BACK: return "Game Back";
        case LAYER_3_GAME_MID: return "Game Mid";
        case LAYER_4_GAME_FRONT: return "Game Front";
        case LAYER_5_GAME_TOP: return "Game Top";
        case LAYER_6_EFFECTS: return "Effects";
        case LAYER_7_UI: return "UI";
        case LAYER_8_TEXT: return "Text";
        default: return "Unknown";
    }
}

const char* getSpriteTypeName(WispSpriteType type) {
    switch (type) {
        case SPRITE_TYPE_GRADIENT: return "Gradient";
        case SPRITE_TYPE_BACKGROUND: return "Background";
        case SPRITE_TYPE_STANDARD: return "Standard";
        case SPRITE_TYPE_UI: return "UI";
        case SPRITE_TYPE_TEXT: return "Text";
        default: return "Unknown";
    }
}

const char* getTilingModeName(WispTilingMode mode) {
    switch (mode) {
        case TILE_NONE: return "None";
        case TILE_REPEAT: return "Repeat";
        case TILE_REPEAT_X: return "Repeat X";
        case TILE_REPEAT_Y: return "Repeat Y";
        case TILE_MIRROR: return "Mirror";
        case TILE_MIRROR_X: return "Mirror X";
        case TILE_MIRROR_Y: return "Mirror Y";
        default: return "Unknown";
    }
}

// Sprite management
bool WispSpriteLayerSystem::addSprite(WispLayeredSprite* sprite) {
    if (!sprite) return false;
    
    validateSprite(sprite);
    
    // If sprite has depth mask, add to multiple layers
    if (sprite->depthMask.enabled) {
        for (int layer = 0; layer < WISP_LAYER_COUNT; layer++) {
            if (sprite->depthMask.isOnLayer((WispSpriteLayer)layer)) {
                layers[layer].push_back(sprite);
            }
        }
    } else {
        // Add to primary layer only
        if (VALIDATE_LAYER(sprite->primaryLayer)) {
            layers[sprite->primaryLayer].push_back(sprite);
        }
    }
    
    return true;
}

bool WispSpriteLayerSystem::removeSprite(WispLayeredSprite* sprite) {
    if (!sprite) return false;
    
    bool removed = false;
    
    // Remove from all layers (in case it's multi-layer)
    for (int layer = 0; layer < WISP_LAYER_COUNT; layer++) {
        auto& layerSprites = layers[layer];
        auto it = std::find(layerSprites.begin(), layerSprites.end(), sprite);
        if (it != layerSprites.end()) {
            layerSprites.erase(it);
            removed = true;
        }
    }
    
    return removed;
}

void WispSpriteLayerSystem::clearLayer(WispSpriteLayer layer) {
    if (VALIDATE_LAYER(layer)) {
        layers[layer].clear();
    }
}

void WispSpriteLayerSystem::clearAllSprites() {
    for (int i = 0; i < WISP_LAYER_COUNT; i++) {
        layers[i].clear();
    }
}

// Main rendering loop
void WispSpriteLayerSystem::renderAllLayers() {
    spritesRendered = 0;
    layersRendered = 0;
    
    // Render layers in order (0 = bottom, 8 = top)
    for (int layer = 0; layer < WISP_LAYER_COUNT; layer++) {
        if (layerEnabled[layer] && !layers[layer].empty()) {
            renderLayer((WispSpriteLayer)layer);
            layersRendered++;
        }
    }
}

void WispSpriteLayerSystem::renderLayer(WispSpriteLayer layer) {
    if (!VALIDATE_LAYER(layer) || !layerEnabled[layer]) return;
    
    auto& layerSprites = layers[layer];
    if (layerSprites.empty()) return;
    
    // Sort by render priority if needed
    sortLayer(layer);
    
    // Set layer alpha
    graphics->setGlobalAlpha(layerAlpha[layer]);
    
    // Render all sprites in this layer
    for (WispLayeredSprite* sprite : layerSprites) {
        if (sprite && sprite->visible) {
            renderSprite(sprite, layer);
            spritesRendered++;
        }
    }
    
    // Reset global alpha
    graphics->setGlobalAlpha(255);
}

void WispSpriteLayerSystem::renderSprite(WispLayeredSprite* sprite, WispSpriteLayer layer) {
    if (!sprite || !sprite->visible) return;
    
    // Apply layer-specific depth for multi-layer sprites
    uint8_t effectiveAlpha = sprite->alpha;
    if (sprite->depthMask.enabled && sprite->depthMask.isOnLayer(layer)) {
        // Modify alpha based on depth value for this layer
        uint8_t depth = sprite->depthMask.depthValues[layer];
        effectiveAlpha = (sprite->alpha * depth) / 10; // depth 0-10 scale
    }
    
    graphics->setAlpha(effectiveAlpha);
    
    // Render based on sprite type
    switch (sprite->type) {
        case SPRITE_TYPE_GRADIENT:
            if (layer == LAYER_0_GRADIENTS) {
                renderGradient(sprite);
            }
            break;
            
        case SPRITE_TYPE_BACKGROUND:
            if (layer == LAYER_1_BACKGROUNDS) {
                renderBackgroundSprite(sprite);
            }
            break;
            
        case SPRITE_TYPE_STANDARD:
            if (layer >= LAYER_2_GAME_BACK && layer <= LAYER_6_EFFECTS) {
                renderStandardSprite(sprite, layer);
            }
            break;
            
        case SPRITE_TYPE_UI:
            if (layer == LAYER_7_UI) {
                renderUISprite(sprite);
            }
            break;
            
        case SPRITE_TYPE_TEXT:
            if (layer == LAYER_8_TEXT) {
                renderTextSprite(sprite);
            }
            break;
    }
}

// Background rendering (Layer 1)
void WispSpriteLayerSystem::renderBackgroundSprite(WispLayeredSprite* sprite) {
    if (sprite->tilingMode == TILE_NONE) {
        // Simple background - render once, scaled to viewport
        WispVec2 pos = applyParallax(sprite, sprite->x, sprite->y);
        WispVec2 screenPos = worldToScreen(pos.x, pos.y);
        
        graphics->drawSprite(sprite->spriteId, screenPos.x, screenPos.y, 
                           sprite->scaleX, sprite->scaleY, sprite->rotation);
    } else {
        // Tiled background
        renderTiledBackground(sprite);
    }
}

void WispSpriteLayerSystem::renderTiledBackground(WispLayeredSprite* sprite) {
    // Get sprite dimensions
    // Note: This would need to be implemented to get actual sprite size
    int spriteWidth = 64;  // Placeholder - get from sprite data
    int spriteHeight = 64; // Placeholder - get from sprite data
    
    int startTileX, startTileY, endTileX, endTileY, tileWidth, tileHeight;
    calculateTileRegion(sprite, startTileX, startTileY, endTileX, endTileY, tileWidth, tileHeight);
    
    // Render tiles
    for (int tileY = startTileY; tileY <= endTileY; tileY++) {
        for (int tileX = startTileX; tileX <= endTileX; tileX++) {
            float worldX = tileX * tileWidth + sprite->scrollX;
            float worldY = tileY * tileHeight + sprite->scrollY;
            
            // Apply mirroring if needed
            bool mirrorX = false, mirrorY = false;
            if (sprite->tilingMode == TILE_MIRROR || sprite->tilingMode == TILE_MIRROR_X) {
                mirrorX = (tileX % 2) != 0;
            }
            if (sprite->tilingMode == TILE_MIRROR || sprite->tilingMode == TILE_MIRROR_Y) {
                mirrorY = (tileY % 2) != 0;
            }
            
            WispVec2 pos = applyParallax(sprite, worldX, worldY);
            WispVec2 screenPos = worldToScreen(pos.x, pos.y);
            
            if (isInViewport(screenPos.x, screenPos.y, tileWidth, tileHeight)) {
                float scaleX = sprite->scaleX * (mirrorX ? -1.0f : 1.0f);
                float scaleY = sprite->scaleY * (mirrorY ? -1.0f : 1.0f);
                
                graphics->drawSprite(sprite->spriteId, screenPos.x, screenPos.y, 
                                   scaleX, scaleY, sprite->rotation);
            }
        }
    }
}

// Standard sprite rendering (Layers 2-6)
void WispSpriteLayerSystem::renderStandardSprite(WispLayeredSprite* sprite, WispSpriteLayer layer) {
    WispVec2 screenPos = worldToScreen(sprite->x, sprite->y);
    
    // Get sprite dimensions for viewport culling
    // Note: This would need actual sprite dimension lookup
    float spriteWidth = 32 * sprite->scaleX;   // Placeholder
    float spriteHeight = 32 * sprite->scaleY;  // Placeholder
    
    if (isInViewport(screenPos.x, screenPos.y, spriteWidth, spriteHeight)) {
        // Get current animation frame if animated
        uint16_t frameIndex = sprite->spriteId;
        if (sprite->hasAnimation) {
            WispAnimationFrame* frame = getCurrentFrame(sprite);
            if (frame) {
                frameIndex = frame->frameIndex;
                screenPos.x += frame->offsetX;
                screenPos.y += frame->offsetY;
                graphics->setAlpha((sprite->alpha * frame->alpha) / 255);
            }
        }
        
        graphics->drawSprite(frameIndex, screenPos.x, screenPos.y, 
                           sprite->scaleX, sprite->scaleY, sprite->rotation);
    }
}

// UI sprite rendering (Layer 7)
void WispSpriteLayerSystem::renderUISprite(WispLayeredSprite* sprite) {
    if (sprite->slice.enabled) {
        renderSlicedSprite(sprite);
    } else {
        // Standard UI sprite - no camera transform
        graphics->drawSprite(sprite->spriteId, sprite->x, sprite->y, 
                           sprite->scaleX, sprite->scaleY, sprite->rotation);
    }
}

void WispSpriteLayerSystem::renderSlicedSprite(WispLayeredSprite* sprite) {
    // 9-patch sprite rendering
    const WispSpriteSlice& slice = sprite->slice;
    
    // Get original sprite dimensions
    float originalWidth = 64;   // Placeholder - get from sprite data
    float originalHeight = 64;  // Placeholder - get from sprite data
    
    float targetWidth = sprite->targetWidth > 0 ? sprite->targetWidth : originalWidth;
    float targetHeight = sprite->targetHeight > 0 ? sprite->targetHeight : originalHeight;
    
    // Calculate slice regions
    float leftWidth = slice.left;
    float rightWidth = originalWidth - slice.right;
    float topHeight = slice.top;
    float bottomHeight = originalHeight - slice.bottom;
    float centerWidth = targetWidth - leftWidth - rightWidth;
    float centerHeight = targetHeight - topHeight - bottomHeight;
    
    // Render 9 patches
    float x = sprite->x;
    float y = sprite->y;
    
    // Top row
    graphics->drawSpriteRegion(sprite->spriteId, x, y, 0, 0, leftWidth, topHeight);
    graphics->drawSpriteRegion(sprite->spriteId, x + leftWidth, y, slice.left, 0, 
                              slice.right - slice.left, topHeight, centerWidth, topHeight);
    graphics->drawSpriteRegion(sprite->spriteId, x + leftWidth + centerWidth, y, 
                              slice.right, 0, rightWidth, topHeight);
    
    // Middle row  
    graphics->drawSpriteRegion(sprite->spriteId, x, y + topHeight, 0, slice.top, 
                              leftWidth, slice.bottom - slice.top, leftWidth, centerHeight);
    graphics->drawSpriteRegion(sprite->spriteId, x + leftWidth, y + topHeight, 
                              slice.left, slice.top, slice.right - slice.left, 
                              slice.bottom - slice.top, centerWidth, centerHeight);
    graphics->drawSpriteRegion(sprite->spriteId, x + leftWidth + centerWidth, y + topHeight, 
                              slice.right, slice.top, rightWidth, slice.bottom - slice.top, 
                              rightWidth, centerHeight);
    
    // Bottom row
    graphics->drawSpriteRegion(sprite->spriteId, x, y + topHeight + centerHeight, 
                              0, slice.bottom, leftWidth, bottomHeight);
    graphics->drawSpriteRegion(sprite->spriteId, x + leftWidth, y + topHeight + centerHeight, 
                              slice.left, slice.bottom, slice.right - slice.left, 
                              bottomHeight, centerWidth, bottomHeight);
    graphics->drawSpriteRegion(sprite->spriteId, x + leftWidth + centerWidth, 
                              y + topHeight + centerHeight, slice.right, slice.bottom, 
                              rightWidth, bottomHeight);
}

// Text rendering (Layer 8)
void WispSpriteLayerSystem::renderTextSprite(WispLayeredSprite* sprite) {
    // Text rendering would be implemented here
    // For now, just draw a placeholder
    graphics->drawRect(sprite->x, sprite->y, 100, 16, 0xFFFF, 1);
}

// Gradient rendering (Layer 0)
void WispSpriteLayerSystem::renderGradient(WispLayeredSprite* sprite) {
    // Simple gradient rendering
    graphics->drawGradient(sprite->x, sprite->y, sprite->scaleX, sprite->scaleY, 
                          0x0000, 0xFFFF); // Black to white placeholder
}

// Animation system
void WispSpriteLayerSystem::updateAnimations(uint32_t deltaTime) {
    for (int layer = 0; layer < WISP_LAYER_COUNT; layer++) {
        for (WispLayeredSprite* sprite : layers[layer]) {
            if (sprite && sprite->hasAnimation && !sprite->animation.paused) {
                updateSpriteAnimation(sprite, deltaTime);
            }
        }
    }
}

void WispSpriteLayerSystem::updateSpriteAnimation(WispLayeredSprite* sprite, uint32_t deltaTime) {
    if (!sprite->hasAnimation || sprite->animation.frames.empty()) return;
    
    WispAnimation& anim = sprite->animation;
    uint32_t currentTime = millis();
    
    if (anim.frameStartTime == 0) {
        anim.frameStartTime = currentTime;
    }
    
    WispAnimationFrame& currentFrame = anim.frames[anim.currentFrame];
    uint32_t frameElapsed = currentTime - anim.frameStartTime;
    
    if (frameElapsed >= currentFrame.duration) {
        advanceAnimation(sprite);
        sprite->isDirty = true;
    }
}

void WispSpriteLayerSystem::advanceAnimation(WispLayeredSprite* sprite) {
    WispAnimation& anim = sprite->animation;
    
    if (anim.frames.empty()) return;
    
    if (anim.pingpong) {
        if (!anim.reverse) {
            anim.currentFrame++;
            if (anim.currentFrame >= anim.frames.size()) {
                anim.currentFrame = anim.frames.size() - 2;
                anim.reverse = true;
                if (!anim.loop) {
                    anim.paused = true;
                    return;
                }
            }
        } else {
            if (anim.currentFrame > 0) {
                anim.currentFrame--;
            } else {
                anim.currentFrame = 1;
                anim.reverse = false;
                if (!anim.loop) {
                    anim.paused = true;
                    return;
                }
            }
        }
    } else {
        anim.currentFrame++;
        if (anim.currentFrame >= anim.frames.size()) {
            if (anim.loop) {
                anim.currentFrame = 0;
            } else {
                anim.currentFrame = anim.frames.size() - 1;
                anim.paused = true;
                return;
            }
        }
    }
    
    anim.frameStartTime = millis();
}

WispAnimationFrame* WispSpriteLayerSystem::getCurrentFrame(WispLayeredSprite* sprite) {
    if (!sprite->hasAnimation || sprite->animation.frames.empty()) return nullptr;
    
    if (sprite->animation.currentFrame < sprite->animation.frames.size()) {
        return &sprite->animation.frames[sprite->animation.currentFrame];
    }
    
    return nullptr;
}

// Camera and viewport
void WispSpriteLayerSystem::setCamera(float x, float y) {
    cameraX = x;
    cameraY = y;
}

void WispSpriteLayerSystem::setCameraSmooth(float x, float y, float smoothing) {
    cameraX = cameraX + (x - cameraX) * smoothing;
    cameraY = cameraY + (y - cameraY) * smoothing;
}

void WispSpriteLayerSystem::setViewport(float width, float height) {
    viewportWidth = width;
    viewportHeight = height;
}

// Sprite creation helpers
WispLayeredSprite* WispSpriteLayerSystem::createGradientSprite(float x, float y, float width, float height, 
                                                               uint32_t colorTop, uint32_t colorBottom) {
    WispLayeredSprite* sprite = new WispLayeredSprite();
    sprite->type = SPRITE_TYPE_GRADIENT;
    sprite->primaryLayer = LAYER_0_GRADIENTS;
    sprite->x = x;
    sprite->y = y;
    sprite->scaleX = width;
    sprite->scaleY = height;
    
    addSprite(sprite);
    return sprite;
}

WispLayeredSprite* WispSpriteLayerSystem::createBackgroundSprite(uint16_t spriteId, WispTilingMode tiling) {
    WispLayeredSprite* sprite = new WispLayeredSprite();
    sprite->spriteId = spriteId;
    sprite->type = SPRITE_TYPE_BACKGROUND;
    sprite->primaryLayer = LAYER_1_BACKGROUNDS;
    sprite->tilingMode = tiling;
    
    addSprite(sprite);
    return sprite;
}

WispLayeredSprite* WispSpriteLayerSystem::createGameSprite(uint16_t spriteId, WispSpriteLayer layer) {
    WispLayeredSprite* sprite = new WispLayeredSprite();
    sprite->spriteId = spriteId;
    sprite->type = SPRITE_TYPE_STANDARD;
    sprite->primaryLayer = layer;
    
    addSprite(sprite);
    return sprite;
}

// Helper functions
WispVec2 WispSpriteLayerSystem::applyParallax(WispLayeredSprite* sprite, float worldX, float worldY) {
    return WispVec2(
        worldX + (cameraX * (1.0f - sprite->parallaxX)),
        worldY + (cameraY * (1.0f - sprite->parallaxY))
    );
}

WispVec2 WispSpriteLayerSystem::worldToScreen(float worldX, float worldY) {
    return WispVec2(worldX - cameraX, worldY - cameraY);
}

bool WispSpriteLayerSystem::isInViewport(float x, float y, float width, float height) {
    return !(x + width < 0 || x > viewportWidth || y + height < 0 || y > viewportHeight);
}

void WispSpriteLayerSystem::sortLayer(WispSpriteLayer layer) {
    if (!VALIDATE_LAYER(layer)) return;
    
    std::sort(layers[layer].begin(), layers[layer].end(), 
              [](const WispLayeredSprite* a, const WispLayeredSprite* b) {
                  return a->renderPriority < b->renderPriority;
              });
}

void WispSpriteLayerSystem::validateSprite(WispLayeredSprite* sprite) {
    if (!sprite) return;
    
    // Validate sprite type matches intended layer
    if (!VALIDATE_SPRITE_TYPE(sprite, sprite->primaryLayer)) {
        printf("Warning: Sprite type %s not valid for layer %s\n",
               getSpriteTypeName(sprite->type),
               getLayerName(sprite->primaryLayer));
    }
}

void WispSpriteLayerSystem::calculateTileRegion(WispLayeredSprite* sprite, int& startTileX, int& startTileY, 
                                               int& endTileX, int& endTileY, int& tileWidth, int& tileHeight) {
    // Get tile dimensions (placeholder)
    tileWidth = 64;
    tileHeight = 64;
    
    WispVec2 pos = applyParallax(sprite, sprite->scrollX, sprite->scrollY);
    WispVec2 screenPos = worldToScreen(pos.x, pos.y);
    
    startTileX = (int)floor((cameraX - screenPos.x) / tileWidth) - 1;
    startTileY = (int)floor((cameraY - screenPos.y) / tileHeight) - 1;
    endTileX = (int)ceil((cameraX + viewportWidth - screenPos.x) / tileWidth) + 1;
    endTileY = (int)ceil((cameraY + viewportHeight - screenPos.y) / tileHeight) + 1;
}
