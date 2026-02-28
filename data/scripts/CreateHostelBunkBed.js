/*
 * Goxel Script: Hostel Bunk Bed
 * Generates a detailed hostel-style bunk bed with frame, slats, mattresses,
 * pillows, blanket fold, guard rails, and a side ladder.
 */

function setBlock(volume, start, end, color) {
    for (let x = start[0]; x <= end[0]; x++) {
        for (let y = start[1]; y <= end[1]; y++) {
            for (let z = start[2]; z <= end[2]; z++) {
                volume.setAt([x, y, z], color);
            }
        }
    }
}

goxel.registerScript({
    name: 'CreateHostelBunkBed',
    description: 'Generates a detailed hostel bunk bed with ladder and rails',
    onExecute: function() {
        let volume = goxel.image.activeLayer.volume;

        // --- Colors (RGBA) ---
        const C_WOOD       = [92, 62, 40, 255];
        const C_WOOD_DARK  = [72, 47, 30, 255];
        const C_MATTRESS   = [170, 174, 182, 255];
        const C_SHEET      = [70, 110, 185, 255];
        const C_PILLOW     = [238, 238, 236, 255];
        const C_METAL      = [176, 176, 182, 255];
        const C_METAL_DARK = [130, 130, 138, 255];

        // Bed footprint and height
        const x1 = -5, x2 = 5;
        const y1 = -9, y2 = 9;

        const zFloor = 0;
        const zBottomDeck = 4;
        const zTopDeck = 14;
        const zTopRail = 18;

        // 1) Main corner posts
        const posts = [[x1, y1], [x1, y2], [x2, y1], [x2, y2]];
        for (let i = 0; i < posts.length; i++) {
            const p = posts[i];
            setBlock(volume, [p[0], p[1], zFloor], [p[0], p[1], zTopRail], C_WOOD_DARK);
        }

        // 2) Lower and upper frame rings
        [zBottomDeck, zTopDeck].forEach(zDeck => {
            setBlock(volume, [x1, y1, zDeck], [x2, y1, zDeck], C_WOOD);
            setBlock(volume, [x1, y2, zDeck], [x2, y2, zDeck], C_WOOD);
            setBlock(volume, [x1, y1, zDeck], [x1, y2, zDeck], C_WOOD);
            setBlock(volume, [x2, y1, zDeck], [x2, y2, zDeck], C_WOOD);
        });

        // 3) Deck slats (inside each frame)
        [zBottomDeck, zTopDeck].forEach(zDeck => {
            for (let y = y1 + 1; y <= y2 - 1; y += 2) {
                setBlock(volume, [x1 + 1, y, zDeck], [x2 - 1, y, zDeck], C_WOOD_DARK);
            }
        });

        // 4) Mattresses + sheet strip + pillows
        // Bottom bunk mattress
        setBlock(volume, [x1 + 1, y1 + 1, zBottomDeck + 1], [x2 - 1, y2 - 1, zBottomDeck + 2], C_MATTRESS);
        setBlock(volume, [x1 + 1, y1 + 1, zBottomDeck + 2], [x2 - 1, y1 + 4, zBottomDeck + 2], C_SHEET);
        setBlock(volume, [x1 + 1, y2 - 3, zBottomDeck + 3], [x2 - 1, y2 - 1, zBottomDeck + 3], C_PILLOW);

        // Top bunk mattress
        setBlock(volume, [x1 + 1, y1 + 1, zTopDeck + 1], [x2 - 1, y2 - 1, zTopDeck + 2], C_MATTRESS);
        setBlock(volume, [x1 + 1, y1 + 1, zTopDeck + 2], [x2 - 1, y1 + 5, zTopDeck + 2], C_SHEET);
        setBlock(volume, [x1 + 1, y2 - 3, zTopDeck + 3], [x2 - 1, y2 - 1, zTopDeck + 3], C_PILLOW);

        // 5) Top guard rails (opening near ladder side)
        setBlock(volume, [x1, y1, zTopRail], [x1, y2, zTopRail], C_WOOD);      // Left side rail
        setBlock(volume, [x2, y1, zTopRail], [x2, y2 - 5, zTopRail], C_WOOD);  // Right side rail with gap
        setBlock(volume, [x1, y1, zTopRail], [x2, y1, zTopRail], C_WOOD);      // Front rail
        setBlock(volume, [x1, y2, zTopRail], [x2, y2, zTopRail], C_WOOD);      // Back rail

        // Vertical mini posts for rail detail
        for (let y = y1 + 2; y <= y2 - 2; y += 3) {
            volume.setAt([x1, y, zTopRail - 1], C_WOOD_DARK);
        }
        for (let y = y1 + 2; y <= y2 - 7; y += 3) {
            volume.setAt([x2, y, zTopRail - 1], C_WOOD_DARK);
        }

        // 6) Side ladder on right/front corner
        const ladderX = x2 + 1;
        const ladderY1 = y1 + 2;
        const ladderY2 = y1 + 5;

        for (let z = zFloor; z <= zTopRail; z++) {
            volume.setAt([ladderX, ladderY1, z], C_METAL);
            volume.setAt([ladderX, ladderY2, z], C_METAL);
            if (z % 3 === 0 && z <= zTopRail - 1) {
                setBlock(volume, [ladderX, ladderY1, z], [ladderX, ladderY2, z], C_METAL_DARK);
            }
        }

        // 7) Small under-bed support bar (bottom bunk)
        setBlock(volume, [x1 + 1, 0, zBottomDeck - 1], [x2 - 1, 0, zBottomDeck - 1], C_WOOD_DARK);
    }
});
