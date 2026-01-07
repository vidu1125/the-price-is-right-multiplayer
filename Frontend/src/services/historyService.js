import { sendPacket } from "../network/dispatcher";
import { registerHandler } from "../network/receiver";
import { OPCODE } from "../network/opcode";

const decoder = new TextDecoder();
let historyPending = null;
const TIMEOUT_MS = 10000;

/**
 * Gửi request xem history
 * CMD_HIST = 0x0502
 */
let cachedHistory = null;

/**
 * Get cached history synchronously
 */
export function getCachedHistory() {
  return cachedHistory;
}


/**
 * Gửi request xem history
 * CMD_HIST = 0x0502
 * @param {object} params
 * @param {number} params.limit Number of items (uint8)
 * @param {number} params.offset Offset (uint8)
 * @param {boolean} params.forceUpdate Force fetch from server
 * @returns {Promise<Array>} List of matches
 */
export function viewHistory({ limit = 10, offset = 0, forceUpdate = false } = {}) {
  console.log("[SERVICE] <viewHistory> called", { limit, offset, forceUpdate });

  return new Promise((resolve, reject) => {
    // Check cache
    if (!forceUpdate && cachedHistory !== null) {
      console.log("[SERVICE] <viewHistory> Returning cached data");
      resolve(cachedHistory);
      return;
    }

    if (historyPending) {
      clearTimeout(historyPending.timeoutId);
    }

    historyPending = {
      resolve: (data) => {
        cachedHistory = data; // Update cache
        resolve(data);
      },
      reject,
      timeoutId: setTimeout(() => {
        if (historyPending) {
          historyPending.reject(new Error("Request timed out"));
          historyPending = null;
        }
      }, TIMEOUT_MS)
    };

    // Construct 2-byte payload: [limit, offset]
    const buffer = new ArrayBuffer(2);
    const view = new DataView(buffer);
    view.setUint8(0, limit);
    view.setUint8(1, offset);

    sendPacket(OPCODE.CMD_HIST, buffer);
  });
}

registerHandler(OPCODE.CMD_HIST, (payload) => {
  console.log("[SERVICE] <viewHistory> response payload len:", payload.byteLength);

  if (historyPending) {
    clearTimeout(historyPending.timeoutId);
    try {
      if (payload.byteLength < 2) {
        throw new Error("Payload too short");
      }

      const view = new DataView(payload);
      const count = view.getUint8(0);
      const reserved = view.getUint8(1);

      console.log(`[SERVICE] <viewHistory> count=${count}`);

      const records = [];
      let offset = 2;
      const RECORD_SIZE = 32; // Calculated from C struct with padding

      for (let i = 0; i < count; i++) {
        if (offset + RECORD_SIZE > payload.byteLength) {
          console.warn("[SERVICE] <viewHistory> Payload truncated");
          break;
        }

        const matchId = view.getUint32(offset, true);
        const modeVal = view.getUint8(offset + 4);
        const isWinner = view.getUint8(offset + 5) !== 0;
        const score = view.getInt32(offset + 8, true); // Little Endian

        // Decode ranking (char[8])
        const rankingBytes = new Uint8Array(payload, offset + 12, 8);
        // Remove null terminators
        let ranking = decoder.decode(rankingBytes).replace(/\0/g, '');

        // ended_at
        const endedAt = view.getBigInt64(offset + 24, true);

        // Map mode enum (1=Scoring, 2=Eliminated)
        let modeStr = "Unknown";
        if (modeVal === 1) modeStr = "Scoring";
        if (modeVal === 2) modeStr = "Eliminated";

        records.push({
          matchId,
          score,
          mode: modeStr,
          ranking,
          isWinner,
          endedAt: Number(endedAt)
        });

        offset += RECORD_SIZE;
      }

      historyPending.resolve(records);
    } catch (e) {
      console.error("[SERVICE] <viewHistory> Parse error", e);
      historyPending.reject(e);
    } finally {
      historyPending = null;
    }
  }
});

let pendingMatchDetail = null;

/**
 * Get match details
 * Uses CMD_REPLAY (0x0503)
 * @param {number} matchId
 * @returns {Promise<Object>} Match details
 */
export function viewMatchDetails(matchId) {
  console.log("[SERVICE] <viewMatchDetails> called", { matchId });

  return new Promise((resolve, reject) => {
    if (pendingMatchDetail) {
      clearTimeout(pendingMatchDetail.timeoutId);
    }

    pendingMatchDetail = {
      resolve,
      reject,
      timeoutId: setTimeout(() => {
        if (pendingMatchDetail) {
          pendingMatchDetail.reject(new Error("Request timed out"));
          pendingMatchDetail = null;
        }
      }, TIMEOUT_MS)
    };

    // Payload: match_id (4 bytes)
    const buffer = new ArrayBuffer(4);
    const view = new DataView(buffer);
    view.setUint32(0, Number(matchId), true); // Little endian

    sendPacket(OPCODE.CMD_REPLAY, buffer);
  });
}

// Handle CMD_REPLAY response
registerHandler(OPCODE.CMD_REPLAY, (payload) => {
  console.log("[SERVICE] <viewMatchDetails> response payload len:", payload.byteLength);

  if (pendingMatchDetail) {
    clearTimeout(pendingMatchDetail.timeoutId);
    try {
      // TODO: Parse actual binary detail data here
      // For now, logging and resolving empty object with raw payload
      console.log("[SERVICE] <viewMatchDetails> Received data");

      pendingMatchDetail.resolve({ matchId: 0, rawData: payload });

    } catch (e) {
      console.error("[SERVICE] <viewMatchDetails> Parse error", e);
      pendingMatchDetail.reject(e);
    } finally {
      pendingMatchDetail = null;
    }
  }
});
