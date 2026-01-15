/**
 * RoundBonus.js - Bonus Round (Tiebreaker) Component
 * 
 * PURPOSE:
 * Displays the bonus round UI when there's a tie at critical positions.
 * Players draw cards to determine who gets eliminated or who wins.
 * 
 * UI LAYOUT (Card Game Style):
 * - Left side: Player 1 box with their drawn card
 * - Center: Deck of cards to draw from
 * - Right side: Player 2 box with their drawn card
 * 
 * CARDS:
 * - Back card: /bg/back_card.jpg
 * - Safe card: /bg/safe_card.jpg  
 * - Eliminated card: /bg/eliminated_card.jpg
 */

import React, { useState, useEffect, useCallback, useRef } from 'react';
import './RoundBonus.css';
import { drawCard, bonusReady, BONUS_STATE, PLAYER_BONUS_STATE } from '../../../services/bonusService';

// Card images
const CARD_BACK = '/bg/back_card.jpg';
const CARD_SAFE = '/bg/safe_card.jpg';
const CARD_ELIMINATED = '/bg/eliminated_card.jpg';

const RoundBonus = ({ matchId, playerId, initialData, onRoundComplete }) => {
    // ============ STATE ============
    const [role, setRole] = useState(initialData?.role || 'participant');
    const [bonusType, setBonusType] = useState(initialData?.bonus_type || 'elimination');
    const [bonusState, setBonusState] = useState(BONUS_STATE.DRAWING);
    const [playerState, setPlayerState] = useState(PLAYER_BONUS_STATE.WAITING_TO_DRAW);
    
    const [participants, setParticipants] = useState(initialData?.participants || []);
    const [drawnPlayers, setDrawnPlayers] = useState([]);
    const [cardsRemaining, setCardsRemaining] = useState(initialData?.total_cards || initialData?.participants?.length || 2);
    const [totalCards, setTotalCards] = useState(initialData?.total_cards || initialData?.participants?.length || 2);
    
    const [revealedCards, setRevealedCards] = useState([]);
    const [eliminatedPlayer, setEliminatedPlayer] = useState(null);
    const [winner, setWinner] = useState(null);
    
    const [message, setMessage] = useState(initialData?.instruction || 'Click DRAW to draw a card');
    const [isDrawing, setIsDrawing] = useState(false);
    const [showResult, setShowResult] = useState(false);
    const [dealingToPlayer, setDealingToPlayer] = useState(null);
    
    const hasInitialized = useRef(!!initialData);
    
    // ============ EVENT HANDLERS ============
    
    // Participant notification
    const handleParticipant = useCallback((e) => {
        const data = e.detail;
        console.log('[RoundBonus] I am a PARTICIPANT:', data);
        
        setRole('participant');
        setBonusType(data.bonus_type || 'elimination');
        setParticipants(data.participants || []);
        setCardsRemaining(data.total_cards || data.participants?.length || 2);
        setTotalCards(data.total_cards || data.participants?.length || 2);
        setMessage(data.instruction || 'Click DRAW to draw a card');
        setBonusState(BONUS_STATE.DRAWING);
        setPlayerState(PLAYER_BONUS_STATE.WAITING_TO_DRAW);
        hasInitialized.current = true;
    }, []);
    
    // Spectator notification
    const handleSpectator = useCallback((e) => {
        const data = e.detail;
        console.log('[RoundBonus] I am a SPECTATOR:', data);
        
        setRole('spectator');
        setBonusType(data.bonus_type || 'elimination');
        setParticipants(data.participants || []);
        setTotalCards(data.participants?.length || 2);
        setCardsRemaining(data.participants?.length || 2);
        setMessage(data.message || 'Watching bonus round...');
        setBonusState(BONUS_STATE.DRAWING);
        hasInitialized.current = true;
    }, []);
    
    // Bonus init response (from bonusReady)
    const handleBonusInit = useCallback((e) => {
        const data = e.detail;
        console.log('[RoundBonus] Bonus init:', data);
        
        if (!data.success) return;
        
        setRole(data.role || 'participant');
        setBonusType(data.bonus_type || 'elimination');
        setBonusState(data.state || BONUS_STATE.DRAWING);
        setCardsRemaining(data.cards_remaining ?? data.participant_count ?? 2);
        setTotalCards(data.participant_count ?? 2);
        
        if (data.role === 'participant') {
            setPlayerState(data.player_state ?? PLAYER_BONUS_STATE.WAITING_TO_DRAW);
        }
        hasInitialized.current = true;
    }, []);
    
    // Card drawn confirmation (to me)
    const handleCardDrawn = useCallback((e) => {
        const data = e.detail;
        console.log('[RoundBonus] My card drawn:', data);
        
        setIsDrawing(false);
        setPlayerState(PLAYER_BONUS_STATE.CARD_DRAWN);
        setMessage('Card drawn! Waiting for others...');
    }, []);
    
    // Someone drew a card (broadcast)
    const handlePlayerDrew = useCallback((e) => {
        const data = e.detail;
        console.log('[RoundBonus] Player drew:', data);
        
        // Animate card dealing to player
        setDealingToPlayer(data.player_id);
        
        setTimeout(() => {
            setDealingToPlayer(null);
            setDrawnPlayers(prev => [...prev, {
                id: data.player_id,
                name: data.player_name,
                drawnAt: data.drawn_at
            }]);
            setCardsRemaining(prev => Math.max(0, prev - 1));
            
            // Update participants list to show who drew
            setParticipants(prev => prev.map(p => 
                p.account_id === data.player_id 
                    ? { ...p, hasDrawn: true }
                    : p
            ));
        }, 600);
    }, []);
    
    // Reveal all cards
    const handleReveal = useCallback((e) => {
        const data = e.detail;
        console.log('[RoundBonus] REVEAL:', data);
        
        setBonusState(BONUS_STATE.REVEALING);
        setRevealedCards(data.results || []);
        setEliminatedPlayer(data.eliminated_player);
        
        if (data.winner) {
            setWinner(data.winner);
        }
        
        // Show reveal animation after short delay
        setTimeout(() => {
            setShowResult(true);
        }, 500);
    }, []);
    
    // Transition to next phase
    const handleTransition = useCallback((e) => {
        const data = e.detail;
        console.log('[RoundBonus] TRANSITION:', data);
        
        setBonusState(BONUS_STATE.COMPLETED);
        
        // Delay then call onRoundComplete
        setTimeout(() => {
            if (data.next_phase === 'NEXT_ROUND') {
                onRoundComplete && onRoundComplete(data.next_round, {
                    scoreboard: data.scoreboard,
                    message: data.message
                });
            } else if (data.next_phase === 'MATCH_ENDED') {
                onRoundComplete && onRoundComplete('end', {
                    finalScores: data.final_scores,
                    winner: data.winner,
                    bonusWinner: data.bonus_winner
                });
            }
        }, 3000);
    }, [onRoundComplete]);
    
    // ============ SETUP EVENT LISTENERS ============
    useEffect(() => {
        console.log('[RoundBonus] Setting up event listeners');
        
        window.addEventListener('bonusParticipant', handleParticipant);
        window.addEventListener('bonusSpectator', handleSpectator);
        window.addEventListener('bonusInit', handleBonusInit);
        window.addEventListener('bonusCardDrawn', handleCardDrawn);
        window.addEventListener('bonusPlayerDrew', handlePlayerDrew);
        window.addEventListener('bonusReveal', handleReveal);
        window.addEventListener('bonusTransition', handleTransition);
        
        // Request current state on mount (with small delay to ensure listeners are ready)
        const timer = setTimeout(() => {
            if (matchId && !hasInitialized.current) {
                console.log('[RoundBonus] Requesting bonus state for match:', matchId);
                bonusReady(matchId);
            }
        }, 100);
        
        return () => {
            clearTimeout(timer);
            window.removeEventListener('bonusParticipant', handleParticipant);
            window.removeEventListener('bonusSpectator', handleSpectator);
            window.removeEventListener('bonusInit', handleBonusInit);
            window.removeEventListener('bonusCardDrawn', handleCardDrawn);
            window.removeEventListener('bonusPlayerDrew', handlePlayerDrew);
            window.removeEventListener('bonusReveal', handleReveal);
            window.removeEventListener('bonusTransition', handleTransition);
        };
    }, [matchId, handleParticipant, handleSpectator, handleBonusInit, handleCardDrawn, 
        handlePlayerDrew, handleReveal, handleTransition]);
    
    // ============ ACTIONS ============
    const handleDrawCard = () => {
        if (isDrawing || playerState !== PLAYER_BONUS_STATE.WAITING_TO_DRAW) return;
        
        console.log('[RoundBonus] Drawing card...');
        setIsDrawing(true);
        drawCard(matchId);
    };
    
    // ============ RENDER HELPERS ============
    
    const renderHeader = () => {
        const typeLabel = bonusType === 'elimination' 
            ? 'ELIMINATION TIEBREAKER' 
            : 'WINNER TIEBREAKER';
            
        return (
            <div className="bonus-header">
                <div className="bonus-label">{typeLabel}</div>
                <p className="bonus-subtitle">
                    {bonusType === 'elimination' 
                        ? 'Multiple players tied at lowest score. One will be eliminated!'
                        : 'Multiple players tied at highest score. The winner will be decided!'}
                </p>
            </div>
        );
    };
    
    // Render player box (left or right side)
    const renderPlayerBox = (player, position, index) => {
        if (!player) {
            return (
                <div className={`player-box ${position} empty`} key={`empty-${position}`}>
                    <div className="player-info">
                        <div className="player-avatar">?</div>
                        <span className="player-name">Waiting...</span>
                    </div>
                    <div className="player-card-area">
                        <div className="card-placeholder">
                            <span>No player</span>
                        </div>
                    </div>
                </div>
            );
        }
        
        const hasDrawn = drawnPlayers.some(d => d.id === player.account_id) || player.hasDrawn;
        const isMe = player.account_id === playerId;
        const isDealing = dealingToPlayer === player.account_id;
        const revealedCard = revealedCards.find(r => r.player_id === player.account_id);
        const isEliminated = revealedCard?.card === 'eliminated';
        
        return (
            <div 
                key={player.account_id || index}
                className={`player-box ${position} ${hasDrawn ? 'has-card' : ''} ${isMe ? 'is-me' : ''}`}
            >
                {/* Player Info */}
                <div className="player-info">
                    <div className="player-avatar">
                        {player.name?.charAt(0) || '?'}
                    </div>
                    <span className="player-name">
                        {player.name || `Player ${player.account_id}`}
                        {isMe && ' (YOU)'}
                    </span>
                </div>
                
                {/* Card Area */}
                <div className="player-card-area">
                    {/* Card dealing animation */}
                    {isDealing && (
                        <div className="dealing-card">
                            <img src={CARD_BACK} alt="Card" className="card-image" />
                        </div>
                    )}
                    
                    {/* Face down card after drawing */}
                    {hasDrawn && !showResult && !isDealing && (
                        <div className="player-card face-down">
                            <img src={CARD_BACK} alt="Card Back" className="card-image" />
                        </div>
                    )}
                    
                    {/* Revealed card */}
                    {showResult && revealedCard && (
                        <div className={`player-card face-up ${isEliminated ? 'eliminated' : 'safe'} flip-reveal`}>
                            <img 
                                src={isEliminated ? CARD_ELIMINATED : CARD_SAFE} 
                                alt={isEliminated ? 'Eliminated' : 'Safe'} 
                                className="card-image"
                            />
                        </div>
                    )}
                    
                    {/* Placeholder when waiting */}
                    {!hasDrawn && !showResult && !isDealing && (
                        <div className="card-placeholder">
                            <span>?</span>
                        </div>
                    )}
                </div>
                
                {/* Status */}
                <div className={`player-status ${hasDrawn ? 'drawn' : 'waiting'} ${showResult && isEliminated ? 'eliminated' : ''}`}>
                    {showResult 
                        ? (isEliminated ? '💀 ELIMINATED' : '✓ SAFE')
                        : (hasDrawn ? '✓ Card Drawn' : 'Waiting...')}
                </div>
            </div>
        );
    };
    
    // Render center deck
    const renderCenterDeck = () => {
        const canDraw = role === 'participant' && playerState === PLAYER_BONUS_STATE.WAITING_TO_DRAW && !isDrawing;
        const deckCards = Math.max(cardsRemaining, 0);
        
        return (
            <div className="center-deck-area">
                {/* Card Stack */}
                <div className="deck-stack">
                    {deckCards > 0 ? (
                        [...Array(deckCards)].map((_, i) => (
                            <div 
                                key={i} 
                                className={`deck-card ${dealingToPlayer ? 'dealing-out' : ''}`}
                                style={{ 
                                    transform: `translateY(${-i * 4}px) translateX(${i * 2}px) rotate(${i * 0.5}deg)`,
                                    zIndex: totalCards - i
                                }}
                            >
                                <img src={CARD_BACK} alt="Card Back" className="card-image" />
                            </div>
                        ))
                    ) : (
                        <div className="deck-empty">
                            <span>All cards drawn!</span>
                        </div>
                    )}
                </div>
                
                {/* Draw Button */}
                {canDraw && !showResult && (
                    <button 
                        className={`draw-btn ${isDrawing ? 'drawing' : ''}`}
                        onClick={handleDrawCard}
                        disabled={isDrawing}
                    >
                        {isDrawing ? (
                            <>
                                <span className="draw-spinner"></span>
                                Drawing...
                            </>
                        ) : (
                            <>
                                <span className="draw-icon">🎴</span>
                                DRAW CARD
                            </>
                        )}
                    </button>
                )}
                
                {/* Waiting message for participant who already drew */}
                {role === 'participant' && playerState === PLAYER_BONUS_STATE.CARD_DRAWN && !showResult && (
                    <div className="waiting-draw-message">
                        ⏳ Waiting for other players...
                    </div>
                )}
                
                {/* Cards remaining label */}
                {!showResult && (
                    <div className="deck-info">
                        {cardsRemaining} / {totalCards} cards remaining
                    </div>
                )}
            </div>
        );
    };
    
    // Render result overlay
    const renderResultOverlay = () => {
        if (!showResult) return null;
        
        return (
            <div className="result-overlay">
                {bonusType === 'elimination' && eliminatedPlayer && (
                    <div className="result-box elimination">
                        <span className="result-icon">💀</span>
                        <span className="result-text">
                            <span className="eliminated-name">{eliminatedPlayer.name}</span>
                            <br />has been ELIMINATED!
                        </span>
                    </div>
                )}
                {bonusType === 'winner_selection' && winner && (
                    <div className="result-box winner">
                        <span className="result-icon">🏆</span>
                        <span className="result-text">
                            <span className="winner-name">{winner.name}</span>
                            <br />is the WINNER!
                        </span>
                    </div>
                )}
            </div>
        );
    };
    
    // ============ MAIN RENDER ============
    const player1 = participants[0] || null;
    const player2 = participants[1] || null;
    
    return (
        <div className="round-bonus-wrapper">
            {renderHeader()}
            
            <div className="bonus-game-area">
                {/* Player 1 (Left) */}
                {renderPlayerBox(player1, 'left', 0)}
                
                {/* Center Deck */}
                {renderCenterDeck()}
                
                {/* Player 2 (Right) */}
                {renderPlayerBox(player2, 'right', 1)}
            </div>
            
            {/* Additional players for 3+ */}
            {participants.length > 2 && (
                <div className="extra-players-row">
                    {participants.slice(2).map((p, i) => renderPlayerBox(p, 'bottom', i + 2))}
                </div>
            )}
            
            {renderResultOverlay()}
            
            {role === 'spectator' && (
                <div className="spectator-banner">
                    👀 You are watching the bonus round
                </div>
            )}
            
            {message && !showResult && (
                <div className="bonus-message">
                    {message}
                </div>
            )}
        </div>
    );
};

export default RoundBonus;
