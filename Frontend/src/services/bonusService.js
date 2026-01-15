/* eslint-disable no-undef */
/**
 * bonusService.js - Bonus Round (Tiebreaker) Service
 * 
 * PURPOSE:
 * Handles communication for the Bonus Round which is triggered when:
 * - After Round 1/2: Multiple players tied at LOWEST score (elimination)
 * - After Round 3: Multiple players tied at HIGHEST score (winner selection)
 * 
 * MECHANISM:
 * - Server creates N cards (1 ELIMINATED, N-1 SAFE)
 * - Each tied player draws one card
 * - After all draw, cards are revealed simultaneously
 * - Player who drew ELIMINATED card is eliminated/loses
 */

import { sendPacket } from "../network/dispatcher";
import { registerHandler } from "../network/receiver";

// Opcodes (MUST match C header opcode.h)
const OPCODE = {
    // Bonus Round - Tiebreaker (0x0660 - 0x067F)
    C2S_BONUS_READY: 0x0660,
    C2S_BONUS_DRAW_CARD: 0x0661,
    S2C_BONUS_INIT: 0x0670,
    S2C_BONUS_PARTICIPANT: 0x0671,
    S2C_BONUS_SPECTATOR: 0x0672,
    S2C_BONUS_CARD_DRAWN: 0x0673,
    S2C_BONUS_PLAYER_DREW: 0x0674,
    S2C_BONUS_REVEAL: 0x0675,
    S2C_BONUS_RESULT: 0x0676,
    S2C_BONUS_TRANSITION: 0x0677
};

// Bonus state enum (matches C enum)
export const BONUS_STATE = {
    NONE: 0,
    INITIALIZING: 1,
    DRAWING: 2,
    REVEALING: 3,
    APPLYING: 4,
    COMPLETED: 5
};

// Player bonus state enum
export const PLAYER_BONUS_STATE = {
    WAITING_TO_DRAW: 0,
    CARD_DRAWN: 1,
    REVEALED_SAFE: 2,
    REVEALED_ELIMINATED: 3,
    SPECTATING: 4
};

//==============================================================================
// 1. READY - Request current bonus state
//==============================================================================
export function bonusReady(matchId) {
    const buffer = new ArrayBuffer(4);
    const view = new DataView(buffer);
    
    view.setUint32(0, matchId, false);
    
    console.log('[SERVICE] <bonus> bonusReady called', { matchId });
    sendPacket(OPCODE.C2S_BONUS_READY, buffer);
}

// Bonus init response (current state)
registerHandler(OPCODE.S2C_BONUS_INIT, (payload) => {
    const text = new TextDecoder().decode(payload);
    const data = JSON.parse(text);
    
    console.log('[SERVICE] <bonus> Bonus init:', data);
    window.dispatchEvent(new CustomEvent('bonusInit', { detail: data }));
});

//==============================================================================
// 2. DRAW CARD
//==============================================================================
export function drawCard(matchId) {
    const buffer = new ArrayBuffer(4);
    const view = new DataView(buffer);
    
    view.setUint32(0, matchId, false);
    
    console.log('[SERVICE] <bonus> drawCard called', { matchId });
    sendPacket(OPCODE.C2S_BONUS_DRAW_CARD, buffer);
}

// Card drawn confirmation (to drawer)
registerHandler(OPCODE.S2C_BONUS_CARD_DRAWN, (payload) => {
    const text = new TextDecoder().decode(payload);
    const data = JSON.parse(text);
    
    console.log('[SERVICE] <bonus> Card drawn confirmation:', data);
    window.dispatchEvent(new CustomEvent('bonusCardDrawn', { detail: data }));
});

// Player drew broadcast (to all)
registerHandler(OPCODE.S2C_BONUS_PLAYER_DREW, (payload) => {
    const text = new TextDecoder().decode(payload);
    const data = JSON.parse(text);
    
    console.log('[SERVICE] <bonus> Player drew card:', data);
    window.dispatchEvent(new CustomEvent('bonusPlayerDrew', { detail: data }));
});

//==============================================================================
// 3. NOTIFICATIONS
//==============================================================================

// Participant notification (you are in the bonus round)
registerHandler(OPCODE.S2C_BONUS_PARTICIPANT, (payload) => {
    const text = new TextDecoder().decode(payload);
    const data = JSON.parse(text);
    
    console.log('[SERVICE] <bonus> You are a PARTICIPANT:', data);
    window.dispatchEvent(new CustomEvent('bonusParticipant', { detail: data }));
});

// Spectator notification (you are watching)
registerHandler(OPCODE.S2C_BONUS_SPECTATOR, (payload) => {
    const text = new TextDecoder().decode(payload);
    const data = JSON.parse(text);
    
    console.log('[SERVICE] <bonus> You are a SPECTATOR:', data);
    window.dispatchEvent(new CustomEvent('bonusSpectator', { detail: data }));
});

//==============================================================================
// 4. REVEAL & RESULTS
//==============================================================================

// Reveal all cards
registerHandler(OPCODE.S2C_BONUS_REVEAL, (payload) => {
    const text = new TextDecoder().decode(payload);
    const data = JSON.parse(text);
    
    console.log('[SERVICE] <bonus> REVEAL:', data);
    window.dispatchEvent(new CustomEvent('bonusReveal', { detail: data }));
});

// Final result
registerHandler(OPCODE.S2C_BONUS_RESULT, (payload) => {
    const text = new TextDecoder().decode(payload);
    const data = JSON.parse(text);
    
    console.log('[SERVICE] <bonus> RESULT:', data);
    window.dispatchEvent(new CustomEvent('bonusResult', { detail: data }));
});

//==============================================================================
// 5. TRANSITION
//==============================================================================

// Transition to next phase
registerHandler(OPCODE.S2C_BONUS_TRANSITION, (payload) => {
    const text = new TextDecoder().decode(payload);
    const data = JSON.parse(text);
    
    console.log('[SERVICE] <bonus> TRANSITION:', data);
    window.dispatchEvent(new CustomEvent('bonusTransition', { detail: data }));
    
    // Also dispatch specific events based on next phase
    if (data.next_phase === 'NEXT_ROUND') {
        window.dispatchEvent(new CustomEvent('bonusNextRound', { 
            detail: {
                nextRound: data.next_round,
                scoreboard: data.scoreboard,
                countdown: data.countdown,
                message: data.message
            }
        }));
    } else if (data.next_phase === 'MATCH_ENDED') {
        window.dispatchEvent(new CustomEvent('bonusMatchEnded', {
            detail: {
                finalScores: data.final_scores,
                winner: data.winner,
                bonusWinner: data.bonus_winner,
                message: data.message
            }
        }));
    }
});

console.log('[SERVICE] <bonus> BonusService loaded and handlers registered');
