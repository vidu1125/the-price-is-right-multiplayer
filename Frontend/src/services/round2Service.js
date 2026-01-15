/* eslint-disable no-undef */
import { sendPacket } from "../network/dispatcher";
import { registerHandler } from "../network/receiver";

// Opcodes (MUST match C header opcode.h)
const OPCODE = {
    // Round 2 - Bid (0x0620 - 0x063F)
    C2S_ROUND2_READY: 0x0620,
    C2S_ROUND2_PLAYER_READY: 0x0621,
    S2C_ROUND2_READY_STATUS: 0x0630,
    S2C_ROUND2_ALL_READY: 0x0631,
    C2S_ROUND2_GET_PRODUCT: 0x0622,
    S2C_ROUND2_PRODUCT: 0x0632,
    C2S_ROUND2_BID: 0x0623,
    S2C_ROUND2_BID_ACK: 0x0633,
    S2C_ROUND2_TURN_RESULT: 0x0634,
    S2C_ROUND2_ALL_FINISHED: 0x0635,
    // Notification opcodes
    NTF_PLAYER_LEFT: 0x02BD
};

//==============================================================================
// 1. PLAYER READY
//==============================================================================
export function playerReady(matchId, playerId) {
    const buffer = new ArrayBuffer(8);
    const view = new DataView(buffer);
    
    view.setUint32(0, matchId, false);
    view.setUint32(4, playerId, false);
    
    console.log('[Round2] Player ready', { matchId, playerId });
    sendPacket(OPCODE.C2S_ROUND2_PLAYER_READY, buffer);
}

// Ready status update
registerHandler(OPCODE.S2C_ROUND2_READY_STATUS, (payload) => {
    const text = new TextDecoder().decode(payload);
    const data = JSON.parse(text);
    
    console.log('[Round2] Ready status update:', data);
    window.dispatchEvent(new CustomEvent('round2ReadyStatus', { detail: data }));
});

// All players ready - round starts
registerHandler(OPCODE.S2C_ROUND2_ALL_READY, (payload) => {
    const text = new TextDecoder().decode(payload);
    const data = JSON.parse(text);
    
    console.log('[Round2] All players ready, starting round:', data);
    window.dispatchEvent(new CustomEvent('round2AllReady', { detail: data }));
});

//==============================================================================
// 2. GET PRODUCT
//==============================================================================
export function getProduct(matchId, productIdx) {
    const buffer = new ArrayBuffer(8);
    const view = new DataView(buffer);

    view.setUint32(0, Number(matchId), false);
    view.setUint32(4, Number(productIdx), false);

    console.log('[Round2] Request product', { matchId, productIdx });
    sendPacket(OPCODE.C2S_ROUND2_GET_PRODUCT, buffer);
}

registerHandler(OPCODE.S2C_ROUND2_PRODUCT, (payload) => {
    console.log('[Round2] Product payload received');
    
    const text = new TextDecoder().decode(payload);
    console.log('[Round2] Decoded text:', text);
    
    try {
        const data = JSON.parse(text);
        console.log('[Round2] Parsed product:', data);
        
        window.dispatchEvent(new CustomEvent('round2Product', { detail: data }));
    } catch (e) {
        console.error('[Round2] JSON parse error:', e);
    }
});

//==============================================================================
// 3. SUBMIT BID
// Payload: match_id (4) + product_idx (4) + bid_value (8) = 16 bytes
//==============================================================================
export function submitBid(matchId, productIdx, bidValue) {
    const buffer = new ArrayBuffer(16);
    const view = new DataView(buffer);
    
    view.setUint32(0, matchId, false);
    view.setUint32(4, productIdx, false);
    // bid_value is int64_t, stored as BigInt64 big-endian
    view.setBigInt64(8, BigInt(bidValue), false);
    
    console.log('[Round2] Submit bid', { matchId, productIdx, bidValue });
    sendPacket(OPCODE.C2S_ROUND2_BID, buffer);
}

// Bid acknowledged
registerHandler(OPCODE.S2C_ROUND2_BID_ACK, (payload) => {
    const text = new TextDecoder().decode(payload);
    const data = JSON.parse(text);
    
    console.log('[Round2] Bid acknowledged:', data);
    window.dispatchEvent(new CustomEvent('round2BidAck', { detail: data }));
});

// Turn result (after all bids)
registerHandler(OPCODE.S2C_ROUND2_TURN_RESULT, (payload) => {
    const text = new TextDecoder().decode(payload);
    const data = JSON.parse(text);
    
    console.log('[Round2] Turn result:', data);
    window.dispatchEvent(new CustomEvent('round2TurnResult', { detail: data }));
});

//==============================================================================
// 4. ROUND FINISHED
//==============================================================================
registerHandler(OPCODE.S2C_ROUND2_ALL_FINISHED, (payload) => {
    const text = new TextDecoder().decode(payload);
    const data = JSON.parse(text);
    
    console.log('[Round2] All finished, final scoreboard:', data);
    window.dispatchEvent(new CustomEvent('round2AllFinished', { detail: data }));
});

console.log('[Round2Service] Loaded and handlers registered');
