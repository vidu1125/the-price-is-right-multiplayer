/**
 * EndGame.js - End Game Screen Component
 * 
 * PURPOSE:
 * Displays the final game results with rankings, winner celebration,
 * and option to return to lobby.
 * 
 * SCORING MODE:
 * - Case 1: No bonus → Rank by score, show tie message if applicable
 * - Case 2: With bonus → Bonus winner = TOP 1, others by score
 * 
 * ELIMINATION MODE:
 * - TOP 1: Winner (last standing) with celebration
 * - TOP 2-4: Eliminated players with round info
 */

import React, { useState, useEffect, useCallback } from 'react';
import './EndGame.css';
import { endGameReady, backToLobby } from '../../../services/endGameService';
import { useNavigate } from 'react-router-dom';

// Crown image for winner
const CROWN_IMAGE = '/bg/crown.png';

const EndGame = ({ matchId, playerId, initialData, onBackToLobby }) => {
    const navigate = useNavigate();
    
    // ============ STATE ============
    const [gameResult, setGameResult] = useState(initialData || null);
    const [isLoading, setIsLoading] = useState(!initialData);
    const [showConfetti, setShowConfetti] = useState(false);
    const [animationPhase, setAnimationPhase] = useState(0);
    
    // ============ DERIVED STATE ============
    const mode = gameResult?.mode || 'scoring';
    const hasTie = gameResult?.has_tie || false;
    const bonusPlayed = gameResult?.bonus_round_played || false;
    const winner = gameResult?.winner || null;
    const rankings = gameResult?.rankings || [];
    const message = gameResult?.message || '';
    
    // Check if current player is winner
    const isWinner = winner?.id === playerId && !hasTie;
    
    // ============ EVENT HANDLERS ============
    const handleEndGameResult = useCallback((e) => {
        const data = e.detail;
        console.log('[EndGame] Received end game result:', data);
        
        setGameResult(data);
        setIsLoading(false);
        
        // Start animations
        setTimeout(() => setAnimationPhase(1), 100);
        setTimeout(() => setAnimationPhase(2), 600);
        setTimeout(() => setAnimationPhase(3), 1200);
        
        // Show confetti for winner
        if (data.winner?.id === playerId && !data.has_tie) {
            setTimeout(() => setShowConfetti(true), 1500);
        }
    }, [playerId]);
    
    const handleBackAck = useCallback((e) => {
        const data = e.detail;
        console.log('[EndGame] Back to lobby acknowledged:', data);
        
        if (data.success) {
            navigate('/lobby');
            onBackToLobby && onBackToLobby();
        }
    }, [navigate, onBackToLobby]);
    
    // ============ SETUP EVENT LISTENERS ============
    useEffect(() => {
        console.log('[EndGame] Setting up event listeners');
        
        window.addEventListener('endGameResult', handleEndGameResult);
        window.addEventListener('endGameBackAck', handleBackAck);
        
        // Request current state if no initial data
        if (!initialData && matchId) {
            console.log('[EndGame] Requesting end game state for match:', matchId);
            endGameReady(matchId);
        } else if (initialData) {
            // Trigger animations for initial data
            setTimeout(() => setAnimationPhase(1), 100);
            setTimeout(() => setAnimationPhase(2), 600);
            setTimeout(() => setAnimationPhase(3), 1200);
            
            if (initialData.winner?.id === playerId && !initialData.has_tie) {
                setTimeout(() => setShowConfetti(true), 1500);
            }
        }
        
        return () => {
            window.removeEventListener('endGameResult', handleEndGameResult);
            window.removeEventListener('endGameBackAck', handleBackAck);
        };
    }, [matchId, initialData, playerId, handleEndGameResult, handleBackAck]);
    
    // ============ ACTIONS ============
    const handleBackToLobby = () => {
        console.log('[EndGame] Back to lobby clicked');
        backToLobby(matchId);
    };
    
    // ============ RENDER: CONFETTI ============
    const renderConfetti = () => {
        if (!showConfetti) return null;
        
        const confettiPieces = [];
        const colors = ['#FFD700', '#FF6B6B', '#4ECDC4', '#45B7D1', '#96CEB4', '#FFEAA7', '#DDA0DD', '#98D8C8'];
        
        for (let i = 0; i < 100; i++) {
            const style = {
                left: `${Math.random() * 100}%`,
                animationDelay: `${Math.random() * 3}s`,
                backgroundColor: colors[Math.floor(Math.random() * colors.length)],
                transform: `rotate(${Math.random() * 360}deg)`
            };
            confettiPieces.push(
                <div key={i} className="confetti-piece" style={style} />
            );
        }
        
        return <div className="confetti-container">{confettiPieces}</div>;
    };
    
    // ============ RENDER: WINNER BANNER ============
    const renderWinnerBanner = () => {
        if (!winner || animationPhase < 1) return null;
        
        if (hasTie) {
            return (
                <div className="winner-banner tie-banner animate-in">
                    <div className="tie-icon">🤝</div>
                    <h1 className="tie-title">IT'S A TIE!</h1>
                    <p className="tie-message">Multiple players tied at highest score!</p>
                </div>
            );
        }
        
        return (
            <div className={`winner-banner ${isWinner ? 'you-won' : ''} animate-in`}>
                {bonusPlayed && (
                    <div className="bonus-badge">🎴 Bonus Round Winner</div>
                )}
                <div className="crown-container">
                    <img src={CROWN_IMAGE} alt="Crown" className="winner-crown" />
                </div>
                <div className="winner-avatar">
                    <span className="avatar-letter">{winner.name?.charAt(0) || '?'}</span>
                    {isWinner && <div className="winner-glow"></div>}
                </div>
                <h1 className="winner-name">
                    {isWinner ? 'YOU WIN!' : `${winner.name} WINS!`}
                </h1>
                {winner.score !== undefined && (
                    <div className="winner-score">
                        <span className="score-label">Final Score</span>
                        <span className="score-value">{winner.score?.toLocaleString()}</span>
                    </div>
                )}
                {mode === 'elimination' && (
                    <p className="winner-subtitle">Last Player Standing!</p>
                )}
            </div>
        );
    };
    
    // ============ RENDER: RANKINGS TABLE ============
    const renderRankings = () => {
        if (animationPhase < 2) return null;
        
        return (
            <div className="rankings-container animate-in">
                <h2 className="rankings-title">
                    {mode === 'elimination' ? 'FINAL STANDINGS' : 'LEADERBOARD'}
                </h2>
                
                <div className="rankings-table">
                    {rankings.map((player, index) => {
                        const isMe = player.account_id === playerId;
                        const isTop = player.rank === 1;
                        const isTied = hasTie && player.rank === 1;
                        
                        return (
                            <div 
                                key={player.account_id}
                                className={`ranking-row 
                                    ${isMe ? 'is-me' : ''} 
                                    ${isTop ? 'is-top' : ''} 
                                    ${isTied ? 'is-tied' : ''}
                                    ${player.eliminated ? 'is-eliminated' : ''}`}
                                style={{ animationDelay: `${index * 0.1}s` }}
                            >
                                {/* Rank Badge */}
                                <div className={`rank-badge rank-${player.rank}`}>
                                    {isTop && !hasTie ? '👑' : 
                                     isTied ? '🤝' :
                                     player.rank === 2 ? '🥈' :
                                     player.rank === 3 ? '🥉' :
                                     `#${player.rank}`}
                                </div>
                                
                                {/* Player Info */}
                                <div className="player-info">
                                    <div className="player-avatar-small">
                                        {player.name?.charAt(0) || '?'}
                                    </div>
                                    <div className="player-details">
                                        <span className="player-name">
                                            {player.name}
                                            {isMe && <span className="you-tag">(YOU)</span>}
                                        </span>
                                        {player.status && (
                                            <span className={`player-status ${player.is_winner ? 'winner' : player.eliminated ? 'eliminated' : ''}`}>
                                                {player.status}
                                            </span>
                                        )}
                                    </div>
                                </div>
                                
                                {/* Score */}
                                <div className="player-score">
                                    <span className="score-number">{player.score?.toLocaleString()}</span>
                                    <span className="score-label">pts</span>
                                </div>
                            </div>
                        );
                    })}
                </div>
            </div>
        );
    };
    
    // ============ RENDER: BACK TO LOBBY BUTTON ============
    const renderBackButton = () => {
        if (animationPhase < 3) return null;
        
        return (
            <div className="back-to-lobby-container animate-in">
                <button 
                    className="back-to-lobby-btn"
                    onClick={handleBackToLobby}
                >
                    <span className="btn-icon">🏠</span>
                    <span className="btn-text">BACK TO LOBBY</span>
                </button>
            </div>
        );
    };
    
    // ============ RENDER: LOADING ============
    if (isLoading) {
        return (
            <div className="end-game-wrapper loading">
                <div className="loading-spinner"></div>
                <p className="loading-text">Loading results...</p>
            </div>
        );
    }
    
    // ============ MAIN RENDER ============
    return (
        <div className={`end-game-wrapper ${mode} ${isWinner ? 'winner-view' : ''}`}>
            {renderConfetti()}
            
            <div className="end-game-content">
                {/* Header */}
                <div className="end-game-header">
                    <h3 className="game-over-label">GAME OVER</h3>
                    <div className="mode-badge">
                        {mode === 'elimination' ? '☠️ ELIMINATION' : '🏆 SCORING'}
                    </div>
                </div>
                
                {/* Winner Section */}
                {renderWinnerBanner()}
                
                {/* Rankings */}
                {renderRankings()}
                
                {/* Message */}
                {message && animationPhase >= 2 && (
                    <div className="result-message animate-in">
                        {message}
                    </div>
                )}
                
                {/* Back Button */}
                {renderBackButton()}
            </div>
        </div>
    );
};

export default EndGame;
