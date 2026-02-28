/*
 * Goxel Script: Hostel Room Walls
 * Generates 4 voxel-style hostel walls:
 * - North wall: window gap
 * - East wall: door gap (adjacent to north wall)
 * - South/West walls: normal slightly dirty full walls
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

function key(x, y, z) {
    return x + '|' + y + '|' + z;
}

goxel.registerScript({
    name: 'CreateHostelWalls',
    description: 'Generates 4 hostel voxel walls with adjacent window and door walls',
    onExecute: function() {
        let volume = goxel.image.activeLayer.volume;

        // Match hostel floor footprint
        const xMin = -14;
        const xMax = 14;
        const yMin = -10;
        const yMax = 10;

        const zBase = 1;
        const zMax = 9;

        // Colors (RGBA)
        const C_WALL     = [180, 176, 165, 255];
        const C_WALL_DARK = [156, 150, 138, 255];
        const C_DIRT     = [105, 95, 80, 255];

        // Openings
        // Window on north wall (y = yMax)
        const winX1 = -3, winX2 = 3;
        const winZ1 = 4, winZ2 = 6;

        // Door on east wall (x = xMax), adjacent to north wall
        // Larger opening to match the detailed hostel door script
        const doorY1 = -3, doorY2 = 2;
        const doorZ1 = zBase, doorZ2 = 7;

        const skip = {};

        // Mark window and door empty voxels
        for (let x = winX1; x <= winX2; x++) {
            for (let z = winZ1; z <= winZ2; z++) {
                skip[key(x, yMax, z)] = true;
            }
        }
        for (let y = doorY1; y <= doorY2; y++) {
            for (let z = doorZ1; z <= doorZ2; z++) {
                skip[key(xMax, y, z)] = true;
            }
        }

        // Place a voxel with dirt/age variation
        function placeAgedVoxel(x, y, z) {
            if (skip[key(x, y, z)]) return;

            // Wall base tone variation
            const tonePick = (x * 17 + y * 11 + z * 5) % 9;
            const color = tonePick < 2 ? C_WALL_DARK : C_WALL;
            volume.setAt([x, y, z], color);

            // Light dirt patches mainly near floor level
            const dirtMask = Math.abs((x * 31 + y * 19 + z * 23) % 29);
            const nearBottom = z <= 2;
            const midPatch = z >= 4 && z <= 5 && ((x + y) % 11 === 0);
            if ((nearBottom && dirtMask < 3) || (midPatch && dirtMask < 1)) {
                volume.setAt([x, y, z], C_DIRT);
            }
        }

        // NORTH wall (window gap) y = yMax
        for (let x = xMin; x <= xMax; x++) {
            for (let z = zBase; z <= zMax; z++) {
                placeAgedVoxel(x, yMax, z);
            }
        }

        // SOUTH wall (normal full) y = yMin
        for (let x = xMin; x <= xMax; x++) {
            for (let z = zBase; z <= zMax; z++) {
                placeAgedVoxel(x, yMin, z);
            }
        }

        // WEST wall (normal slightly dirty full) x = xMin
        for (let y = yMin; y <= yMax; y++) {
            for (let z = zBase; z <= zMax; z++) {
                placeAgedVoxel(xMin, y, z);
            }
        }

        // EAST wall (door gap) x = xMax
        for (let y = yMin; y <= yMax; y++) {
            for (let z = zBase; z <= zMax; z++) {
                placeAgedVoxel(xMax, y, z);
            }
        }
    }
});
