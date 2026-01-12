import { sendPacket } from "../network/dispatcher";
import { OPCODE } from "../network/opcode";

/**
 * Start a match in the specified room
 * @param {number} roomId - The room ID
 */
export function startGame(roomId) {
    if (!roomId || roomId <= 0) {
        console.error("[gameService] Invalid room ID:", roomId);
        return;
    }

    console.log(`[gameService] Starting game in room ${roomId}...`);

    // Build binary payload: uint32_t room_id (Big Endian)
    const payload = new ArrayBuffer(4);
    const view = new DataView(payload);
    view.setUint32(0, roomId, false); // Big Endian (network byte order)

    // Send packet (fire and forget)
    // Response (RES_GAME_STARTED) or Notification (NTF_GAME_START) 
    // should be handled by listeners in the UI or global handlers.
    sendPacket(OPCODE.CMD_START_GAME || 0x0300, payload);
}

/**
 * Leave/end the current match
 * @param {number} matchId - The match ID
 * @returns {Promise<{success: boolean}>}
 */
export async function endMatch(matchId) {
    console.log(`[gameService] Ending match ${matchId}...`);

    // TODO: Implement end match logic
    // For now, return placeholder
    return { success: false, error: "Not implemented" };
}
