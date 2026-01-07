import { sendPacket } from "../network/dispatcher";
import { registerHandler } from "../network/receiver";
import { OPCODE } from "../network/opcode";

const decoder = new TextDecoder();
let historyPending = null;
const TIMEOUT_MS = 10000;

/**
 * Gửi request xem history
 * CMD_HIST = 0x0502
 * @param {object} params
 * @param {number} params.limit Number of items (uint8)
 * @param {number} params.offset Offset (uint8)
 * @returns {Promise<Array>} List of matches
 */
export function viewHistory({ limit = 10, offset = 0 } = {}) {
  console.log("[Service][History] viewHistory() called", { limit, offset });

  return new Promise((resolve, reject) => {
    if (historyPending) {
      clearTimeout(historyPending.timeoutId);
    }

    historyPending = {
      resolve,
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
  console.log("[Service][History] received binary response, len:", payload.byteLength);

  if (historyPending) {
    clearTimeout(historyPending.timeoutId);
    try {
      const view = new DataView(payload);
      const count = view.getUint8(0);
      const reserved = view.getUint8(1); // alignment

      console.log(`[Service][History] count=${count}`);

      const records = [];
      const RECORD_SIZE = 24; // 4 + 1 + 1 + 4 + 8 + 8 (padding/ranking?)
      // Check struct alignment in C:
      // uint32 match_id (4)
      // uint8 mode (1)
      // uint8 is_winner (1)
      // int32 final_score (4) <-- padding likely added before this? 
      //    offset 0: match_id (4)
      //    offset 4: mode (1)
      //    offset 5: is_winner (1)
      //    offset 6: padding (2) <-- compiler usually aligns int32 to 4 bytes
      //    offset 8: final_score (4)
      //    offset 12: ranking (8)
      //    offset 20: ended_at (8)
      // Total size = 28 bytes? No, wait. 
      // Let's re-verify C struct packing. 
      // typedef struct {
      //    uint32_t match_id;      
      //    uint8_t  mode;          
      //    uint8_t  is_winner;      
      //    int32_t  final_score;
      //    char     ranking[8];
      //    uint64_t ended_at;    
      // } HistoryRecord;
      // 
      // PACKED is likely missing from HistoryRecord definition in user's last snippet for header file?
      // User snippet: 
      // typedef struct { ... } HistoryRecord;
      // If NOT packed:
      // 0-3: match_id
      // 4: mode
      // 5: is_winner
      // 6-7: padding
      // 8-11: final_score
      // 12-19: ranking
      // 20-23: padding (for 8-byte alignment of uint64)
      // 24-31: ended_at
      // Total = 32 bytes

      // However, usually network structs are attributted with PACKED.
      // Let's assume user defined `typedef struct PACKED` or we need to treat it as packed manually.
      // But wait! User's snippet showed: `typedef struct { ... } HistoryRecord`. NO PACKED.
      // I should double check if `PACKED` macro is used or if `__attribute__((packed))` is implicit.
      //
      // Assuming NO PACKED (default alignment):
      // offset = 2 (header: count + reserved)

      let offset = 2;

      for (let i = 0; i < count; i++) {
        const matchId = view.getUint32(offset, true); // Little Endian
        const modeVal = view.getUint8(offset + 4);
        const isWinner = view.getUint8(offset + 5) !== 0;
        // Skip 2 padding bytes (6, 7)
        const score = view.getInt32(offset + 8, true);

        const rankingBytes = new Uint8Array(payload, offset + 12, 8);
        // Decode string usually stops at null terminator
        let ranking = decoder.decode(rankingBytes).replace(/\0/g, '');

        // Skip 4 padding bytes (20, 21, 22, 23) ??
        // ended_at is uint64. 
        // BigInt64 get
        const endedAt = view.getBigInt64(offset + 24, true);

        records.push({
          matchId,
          score,
          mode: modeVal === 1 ? "Scoring" : "Eliminated", // Mapping enum
          ranking,
          isWinner,
          endedAt: Number(endedAt) // Convert to number for JS (lossy if huge, but date usually fits)
        });

        offset += 32; // sizeof(HistoryRecord) aligned
      }

      historyPending.resolve(records);
    } catch (e) {
      console.error("[Service][History] Parse error", e);
      historyPending.reject(e);
    } finally {
      historyPending = null;
    }
  }
});
