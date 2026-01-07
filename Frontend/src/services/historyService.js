import { sendPacket } from "../network/dispatcher";
import { registerHandler } from "../network/receiver";
import { OPCODE } from "../network/opcode";

const decoder = new TextDecoder();
let historyPending = null;
const TIMEOUT_MS = 10000;

/**
 * Gửi request xem history
 * CMD_HIST = 0x0502
 * @returns {Promise<Array>} List of matches
 */
export function viewHistory() {
  console.log("[Service][History] viewHistory() called");

  return new Promise((resolve, reject) => {
    // Clear previous pending request if exists
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

    sendPacket(OPCODE.CMD_HIST);
  });
}

registerHandler(OPCODE.CMD_HIST, (payload) => {
  const text = decoder.decode(payload);
  console.log("[Service][History] response payload:", text);

  if (historyPending) {
    clearTimeout(historyPending.timeoutId);
    try {
      const data = JSON.parse(text);
      historyPending.resolve(data);
    } catch (e) {
      console.error("[Service][History] JSON parse error", e);
      historyPending.reject(e);
    } finally {
      historyPending = null;
    }
  }
});
