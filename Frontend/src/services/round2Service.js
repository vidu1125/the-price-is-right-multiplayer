/**
 * round2Service.js - Round 2: The Bid
 * 
 * Frontend service for Round 2 bid gameplay.
 * Handles sending commands and receiving events for bidding round.
 */

import { sendBinaryCommand } from '../network/socketClient';
import { OPCODE } from '../network/opcode';
import { registerHandler } from '../network/receiver';

/* global BigInt */

/**
 * Send player ready signal for Round 2
 * @param {number} matchId - Current match ID
 * @param {number} playerId - Player's account ID
 */
export function playerReady(matchId, playerId) {
    const buffer = new ArrayBuffer(8);
    const view = new DataView(buffer);
    view.setUint32(0, matchId, false);  // big-endian
    view.setUint32(4, playerId, false);
    
    console.log('[Round2] Player ready:', { matchId, playerId });
    sendBinaryCommand(OPCODE.OP_C2S_ROUND2_PLAYER_READY, buffer);
}

/**
 * Request current product data
 * @param {number} matchId - Current match ID
 * @param {number} productIdx - Product index (0-based)
 */
export function getProduct(matchId, productIdx) {
    const buffer = new ArrayBuffer(8);
    const view = new DataView(buffer);
    view.setUint32(0, matchId, false);
    view.setUint32(4, productIdx, false);
    
    console.log('[Round2] Get product:', { matchId, productIdx });
    sendBinaryCommand(OPCODE.OP_C2S_ROUND2_GET_PRODUCT, buffer);
}

/**
 * Submit bid for current product
 * @param {number} matchId - Current match ID
 * @param {number} productIdx - Product index (0-based)
 * @param {number} bidValue - Player's bid (price guess)
 */
export function submitBid(matchId, productIdx, bidValue) {
    const buffer = new ArrayBuffer(16);
    const view = new DataView(buffer);
    view.setUint32(0, matchId, false);
    view.setUint32(4, productIdx, false);
    
    // 64-bit big-endian for bid value
    // JavaScript BigInt for 64-bit numbers
    const bigBid = BigInt(bidValue);
    view.setBigInt64(8, bigBid, false);  // big-endian
    
    console.log('[Round2] Submit bid:', { matchId, productIdx, bidValue });
    sendBinaryCommand(OPCODE.OP_C2S_ROUND2_BID, buffer);
}

/**
 * Event handlers mapping for Round 2
 * Use with dispatcher to handle server responses
 */
export const round2Events = {
    [OPCODE.OP_S2C_ROUND2_READY_STATUS]: 'round2ReadyStatus',
    [OPCODE.OP_S2C_ROUND2_ALL_READY]: 'round2AllReady',
    [OPCODE.OP_S2C_ROUND2_START]: 'round2Start',
    [OPCODE.OP_S2C_ROUND2_PRODUCT]: 'round2Product',
    [OPCODE.OP_S2C_ROUND2_BID_ACK]: 'round2BidAck',
    [OPCODE.OP_S2C_ROUND2_TURN_RESULT]: 'round2TurnResult',
    [OPCODE.OP_S2C_ROUND2_ALL_FINISHED]: 'round2AllFinished',
};

/**
 * Test login for UI testing (bypass auth)
 * @param {number} playerId - Test player ID (1, 2, or 3)
 */
export function testLogin(playerId) {
    const buffer = new ArrayBuffer(4);
    const view = new DataView(buffer);
    view.setUint32(0, playerId, false);
    
    console.log('[Round2] Test login for player', playerId);
    sendBinaryCommand(OPCODE.CMD_TEST_LOGIN, buffer);
}

//==============================================================================
// REGISTER HANDLERS
// Parse server responses and dispatch as window events
//==============================================================================

function parseJsonPayload(payload) {
    try {
        const text = new TextDecoder().decode(payload);
        return JSON.parse(text);
    } catch (e) {
        console.error('[Round2] Failed to parse JSON:', e);
        return null;
    }
}

// Ready status
registerHandler(OPCODE.OP_S2C_ROUND2_READY_STATUS, (payload) => {
    const data = parseJsonPayload(payload);
    if (data) {
        console.log('[Round2] Event: round2ReadyStatus', data);
        window.dispatchEvent(new CustomEvent('round2ReadyStatus', { detail: data }));
    }
});

// All ready - game starts
registerHandler(OPCODE.OP_S2C_ROUND2_ALL_READY, (payload) => {
    const data = parseJsonPayload(payload);
    if (data) {
        console.log('[Round2] Event: round2AllReady', data);
        window.dispatchEvent(new CustomEvent('round2AllReady', { detail: data }));
    }
});

// Product data
registerHandler(OPCODE.OP_S2C_ROUND2_PRODUCT, (payload) => {
    const data = parseJsonPayload(payload);
    if (data) {
        console.log('[Round2] Event: round2Product', data);
        window.dispatchEvent(new CustomEvent('round2Product', { detail: data }));
    }
});

// Bid acknowledgment
registerHandler(OPCODE.OP_S2C_ROUND2_BID_ACK, (payload) => {
    const data = parseJsonPayload(payload);
    if (data) {
        console.log('[Round2] Event: round2BidAck', data);
        window.dispatchEvent(new CustomEvent('round2BidAck', { detail: data }));
    }
});

// Turn result (all bids + scores)
registerHandler(OPCODE.OP_S2C_ROUND2_TURN_RESULT, (payload) => {
    const data = parseJsonPayload(payload);
    if (data) {
        console.log('[Round2] Event: round2TurnResult', data);
        window.dispatchEvent(new CustomEvent('round2TurnResult', { detail: data }));
    }
});

// Round finished
registerHandler(OPCODE.OP_S2C_ROUND2_ALL_FINISHED, (payload) => {
    const data = parseJsonPayload(payload);
    if (data) {
        console.log('[Round2] Event: round2AllFinished', data);
        window.dispatchEvent(new CustomEvent('round2AllFinished', { detail: data }));
    }
});

console.log('[Round2Service] Handlers registered');

export default {
    playerReady,
    getProduct,
    submitBid,
    testLogin,
    round2Events,
};
