import { sendPacket } from "../network/dispatcher";
import { OPCODE } from "../network/opcode";

/**
 * Sends a forfeit request to the server.
 * @param {number} matchId - The ID of the match to forfeit.
 */
export const sendForfeit = (matchId) => {
    if (!matchId) {
        console.warn("[ForfeitService] Missing matchId");
        return;
    }

    console.log("[ForfeitService] Sending forfeit request for match:", matchId);

    // Payload: match_id (4 bytes) - uint32 Big Endian
    const payload = new ArrayBuffer(4);
    const view = new DataView(payload);
    view.setUint32(0, matchId, false);

    sendPacket(OPCODE.CMD_FORFEIT, payload);
};
