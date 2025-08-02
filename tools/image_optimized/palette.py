#!/usr/bin/env python3
"""
kdtree_remap_palette.py

Remap any-size palette via cKDTree (no 256 limit).
Source images are pulled from ./original/
Remapped images and palette.png are saved to ./optimized/
"""

import os
import shutil
import numpy as np
from PIL import Image
from tqdm import tqdm
from scipy.spatial import cKDTree

PALETTE_FILE = 'palette.png'
SOURCE_DIR   = 'original'
OUTPUT_DIR   = 'optimized'
INPUT_EXTS   = ('.png', '.jpg', '.jpeg')
CHUNK_SIZE   = 200_000

# 1) Build KD-Tree
pal_arr = np.array(Image.open(PALETTE_FILE).convert('RGB')).reshape(-1, 3)
palette = np.unique(pal_arr, axis=0)
tree    = cKDTree(palette)

# 2) Make output dir
os.makedirs(OUTPUT_DIR, exist_ok=True)

# 3) Remap all images from source dir
for fn in tqdm(os.listdir(SOURCE_DIR), desc='Remapping'):
    if not fn.lower().endswith(INPUT_EXTS):
        continue

    src_path = os.path.join(SOURCE_DIR, fn)
    img = Image.open(src_path).convert('RGB')
    arr = np.array(img)
    h, w, _ = arr.shape
    flat = arr.reshape(-1, 3)

    out = np.empty_like(flat)
    for i in range(0, flat.shape[0], CHUNK_SIZE):
        block = flat[i:i+CHUNK_SIZE]
        try:
            _, idx = tree.query(block, workers=-1)
        except TypeError:
            _, idx = tree.query(block)
        out[i:i+CHUNK_SIZE] = palette[idx]

    remapped = out.reshape((h, w, 3)).astype(np.uint8)
    Image.fromarray(remapped).save(os.path.join(OUTPUT_DIR, fn))

# 4) Copy palette into output folder
shutil.copy2(PALETTE_FILE, os.path.join(OUTPUT_DIR, PALETTE_FILE))

print("✅ Done. See ./optimized/")
