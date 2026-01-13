import React, { useState, useEffect, useRef, useCallback } from 'react';
import './Round2.css';
import { submitBid, playerReady } from '../../../services/round2Service';
import { waitForConnection } from '../../../network/socketClient';

/**
 * Round 2: The Bid
 * Players bid on product prices, closest without going over wins.
 */
const Round2 = ({ matchId = 1, playerId = 1, onRoundComplete, isWagerAvailable = false }) => {
    // Game phase: 'connecting' -> 'playing' -> 'result' -> 'summary'
    const [gamePhase, setGamePhase] = useState('connecting');

    // Connection state
    const [connectedPlayers, setConnectedPlayers] = useState([]);
    const [connectionCountdown, setConnectionCountdown] = useState(10);

    // Product state
    const [currentProductIdx, setCurrentProductIdx] = useState(0);
    const [totalProducts, setTotalProducts] = useState(5);
    const [productData, setProductData] = useState(null);
    const [timeLeft, setTimeLeft] = useState(15);
    const [score, setScore] = useState(0);
    
    // Bid state
    const [bidValue, setBidValue] = useState('');
    const [hasBid, setHasBid] = useState(false);
    const [isWagerActive, setIsWagerActive] = useState(false);
    
    // Turn result state
    const [turnResult, setTurnResult] = useState(null);
    const [showResult, setShowResult] = useState(false);
    
    // Summary state
    const [summaryData, setSummaryData] = useState(null);
    const [summaryCountdown, setSummaryCountdown] = useState(10);

    const startTime = useRef(null);
    const timerRef = useRef(null);
    const summaryTimerRef = useRef(null);
    const connectionTimerRef = useRef(null);
    const productIdxRef = useRef(0);
    const scoreRef = useRef(0);
    const gameStartedRef = useRef(false);

    //==========================================================================
    // AUTO-CONNECT: Join Round 2
    //==========================================================================
    useEffect(() => {
        console.log('[Round2] Connecting player ' + playerId + ' to match ' + matchId);

        let isMounted = true;
        let retryInterval = null;

        const connectAndReady = async () => {
            try {
                console.log('[Round2] Waiting for socket...');
                await waitForConnection(5000);
                console.log('[Round2] Connected! Sending playerReady...');

                if (!isMounted) return;
                playerReady(matchId, playerId);

                retryInterval = setInterval(() => {
                    if (!gameStartedRef.current && isMounted) {
                        console.log('[Round2] Retry playerReady...');
                        playerReady(matchId, playerId);
                    } else {
                        clearInterval(retryInterval);
                    }
                }, 1000);

            } catch (err) {
                console.error('[Round2] Failed to connect:', err);
            }
        };

        connectAndReady();

        setConnectionCountdown(10);
        connectionTimerRef.current = setInterval(() => {
            setConnectionCountdown(prev => {
                if (prev <= 1) {
                    clearInterval(connectionTimerRef.current);
                    return 0;
                }
                return prev - 1;
            });
        }, 1000);

        return () => {
            isMounted = false;
            if (connectionTimerRef.current) clearInterval(connectionTimerRef.current);
            if (retryInterval) clearInterval(retryInterval);
        };
    }, [matchId, playerId]);

    //==========================================================================
    // LISTEN: Ready status
    //==========================================================================
    useEffect(() => {
        const handleReadyStatus = (e) => {
            const data = e.detail;
            console.log('[Round2] Ready status:', data);
            setConnectedPlayers(data.players || []);
        };

        window.addEventListener('round2ReadyStatus', handleReadyStatus);
        return () => window.removeEventListener('round2ReadyStatus', handleReadyStatus);
    }, []);

    //==========================================================================
    // LISTEN: All ready - Start game
    //==========================================================================
    useEffect(() => {
        const handleAllReady = (e) => {
            const data = e.detail;
            console.log('[Round2] All ready, starting!', data);

            if (gameStartedRef.current) return;
            gameStartedRef.current = true;

            if (connectionTimerRef.current) clearInterval(connectionTimerRef.current);
            setGamePhase('playing');
            setTotalProducts(data.total_products || 5);
            
            productIdxRef.current = 0;
            setCurrentProductIdx(0);
        };

        window.addEventListener('round2AllReady', handleAllReady);
        return () => window.removeEventListener('round2AllReady', handleAllReady);
    }, []);

    //==========================================================================
    // HANDLER: Timeout
    //==========================================================================
    const handleTimeout = useCallback(() => {
        console.log('[Round2] Timeout! Submitting empty bid');
        submitBid(matchId, productIdxRef.current, 0);
    }, [matchId]);

    //==========================================================================
    // LISTEN: Product received
    //==========================================================================
    useEffect(() => {
        const handleProduct = (e) => {
            const data = e.detail;
            console.log('[Round2] Product received:', data);

            if (data.success === false) {
                console.error('[Round2] Failed:', data.error);
                if (data.error?.includes('eliminated')) {
                    setGamePhase('eliminated');
                }
                return;
            }

            if (gamePhase !== 'playing') {
                setGamePhase('playing');
            }

            const serverIdx = data.product_idx;
            if (serverIdx !== undefined) {
                productIdxRef.current = serverIdx;
                setCurrentProductIdx(serverIdx);
            }

            setProductData(data);
            setBidValue('');
            setHasBid(false);
            setShowResult(false);
            setTurnResult(null);
            startTime.current = Date.now();

            const timeLimit = Math.floor((data.time_limit_ms || 15000) / 1000);
            setTimeLeft(timeLimit);

            window.dispatchEvent(new CustomEvent('timerUpdate', { detail: { timeLeft: timeLimit } }));

            if (timerRef.current) clearInterval(timerRef.current);
            timerRef.current = setInterval(() => {
                setTimeLeft(prev => {
                    const newTime = prev <= 1 ? 0 : prev - 1;
                    window.dispatchEvent(new CustomEvent('timerUpdate', { detail: { timeLeft: newTime } }));

                    if (newTime <= 0) {
                        clearInterval(timerRef.current);
                        if (!hasBid) handleTimeout();
                    }
                    return newTime;
                });
            }, 1000);
        };

        window.addEventListener('round2Product', handleProduct);
        return () => window.removeEventListener('round2Product', handleProduct);
    }, [handleTimeout, gamePhase, hasBid]);

    //==========================================================================
    // LISTEN: Bid acknowledgment
    //==========================================================================
    useEffect(() => {
        const handleBidAck = (e) => {
            const data = e.detail;
            console.log('[Round2] Bid acknowledged:', data);
            setHasBid(true);
        };

        window.addEventListener('round2BidAck', handleBidAck);
        return () => window.removeEventListener('round2BidAck', handleBidAck);
    }, []);

    //==========================================================================
    // LISTEN: Turn result (all bids + scores)
    //==========================================================================
    useEffect(() => {
        const handleTurnResult = (e) => {
            const data = e.detail;
            console.log('[Round2] Turn result:', data);

            clearInterval(timerRef.current);
            setTurnResult(data);
            setShowResult(true);
            setGamePhase('result');

            // Update score from result
            const myBid = data.bids?.find(b => Number(b.id) === Number(playerId));
            if (myBid) {
                const newTotal = myBid.total_score || (scoreRef.current + myBid.score_delta);
                scoreRef.current = newTotal;
                setScore(newTotal);

                window.dispatchEvent(new CustomEvent('round1ScoreUpdate', {
                    detail: { score: myBid.score_delta, totalScore: newTotal }
                }));
            }

            // Result will be shown for 3 seconds, then next product arrives
            setTimeout(() => {
                setShowResult(false);
                productIdxRef.current++;
                setCurrentProductIdx(productIdxRef.current);
            }, 3000);
        };

        window.addEventListener('round2TurnResult', handleTurnResult);
        return () => window.removeEventListener('round2TurnResult', handleTurnResult);
    }, [playerId]);

    //==========================================================================
    // LISTEN: Round finished
    //==========================================================================
    useEffect(() => {
        const handleAllFinished = (e) => {
            const data = e.detail;
            console.log('[Round2] ALL_FINISHED:', data);

            if (timerRef.current) clearInterval(timerRef.current);

            setSummaryData(data);
            setGamePhase('summary');
            setSummaryCountdown(10);

            if (summaryTimerRef.current) clearInterval(summaryTimerRef.current);
            summaryTimerRef.current = setInterval(() => {
                setSummaryCountdown(prev => {
                    if (prev <= 1) {
                        clearInterval(summaryTimerRef.current);
                        return 0;
                    }
                    return prev - 1;
                });
            }, 1000);
        };

        window.addEventListener('round2AllFinished', handleAllFinished);
        return () => {
            window.removeEventListener('round2AllFinished', handleAllFinished);
            if (summaryTimerRef.current) clearInterval(summaryTimerRef.current);
        };
    }, []);

    //==========================================================================
    // Handle countdown end
    //==========================================================================
    useEffect(() => {
        if (gamePhase === 'summary' && summaryCountdown === 0) {
            if (onRoundComplete) {
                onRoundComplete(3, { score: scoreRef.current });
            }
        }
    }, [gamePhase, summaryCountdown, onRoundComplete]);

    //==========================================================================
    // Cleanup
    //==========================================================================
    useEffect(() => {
        return () => {
            if (timerRef.current) clearInterval(timerRef.current);
            if (summaryTimerRef.current) clearInterval(summaryTimerRef.current);
        };
    }, []);

    //==========================================================================
    // HANDLER: Submit bid
    //==========================================================================
    const handleSubmit = () => {
        if (hasBid) return;
        
        const bid = parseInt(bidValue) || 0;
        console.log('[Round2] Submitting bid:', bid);
        
        submitBid(matchId, productIdxRef.current, bid);
    };

    //==========================================================================
    // HANDLER: Wager toggle
    //==========================================================================
    const handleWagerToggle = () => {
        setIsWagerActive(!isWagerActive);
    };

    //==========================================================================
    // HANDLER: Skip to next round
    //==========================================================================
    const handleSkipToNextRound = () => {
        clearInterval(summaryTimerRef.current);
        if (onRoundComplete) {
            onRoundComplete(3, { score: scoreRef.current });
        }
    };

    //==========================================================================
    // RENDER: Connecting screen
    //==========================================================================
    if (gamePhase === 'connecting') {
        return (
            <div className="round2-wrapper-quiz">
                <div className="ready-content">
                    <h1 className="ready-title">💰 ROUND 2 - THE BID 💰</h1>
                    <p className="ready-subtitle">Connecting to game...</p>

                    <div className="player-status-list">
                        <h3>Players ({connectedPlayers.filter(p => p.ready).length}/{connectedPlayers.length || 3})</h3>
                        {connectedPlayers.length > 0 ? (
                            connectedPlayers.map((p, idx) => (
                                <div key={idx} className={'player-status-item ' + (p.ready ? 'ready' : 'not-ready')}>
                                    <span className="player-status-name">
                                        Player {p.account_id || idx + 1}
                                    </span>
                                    <span className="player-status-badge">
                                        {p.ready ? '✅ Ready' : '⏳ Connecting...'}
                                    </span>
                                </div>
                            ))
                        ) : (
                            <div className="waiting-for-players">
                                <div className="connecting-spinner">🔄</div>
                                Waiting for players...
                            </div>
                        )}
                    </div>

                    <div className="connecting-status">
                        <div className="connecting-spinner">🔄</div>
                        <p>Auto-starting when ready...</p>
                        {connectionCountdown > 0 && (
                            <p className="connection-countdown">Timeout in {connectionCountdown}s</p>
                        )}
                    </div>
                </div>
            </div>
        );
    }

    //==========================================================================
    // RENDER: Turn result (showing all bids and correct price)
    //==========================================================================
    if (gamePhase === 'result' && showResult && turnResult) {
        return (
            <div className="round2-wrapper-quiz">
                <div className="result-content">
                    <h2 className="result-title">📊 TURN {turnResult.product_idx + 1} RESULT</h2>
                    
                    <div className="correct-price-box">
                        <span className="correct-label">CORRECT PRICE</span>
                        <span className="correct-value">${turnResult.correct_price?.toLocaleString()}</span>
                    </div>

                    <div className="all-bids-list">
                        {turnResult.bids?.sort((a, b) => b.score_delta - a.score_delta).map((bid, idx) => {
                            const isMe = Number(bid.id) === Number(playerId);
                            const isWinner = bid.score_delta > 0;
                            return (
                                <div 
                                    key={idx} 
                                    className={'bid-item ' + (isWinner ? 'winner ' : '') + (isMe ? 'is-me ' : '') + (bid.overbid ? 'overbid' : '')}
                                >
                                    <span className="bid-rank">{isWinner ? '🏆' : (idx + 1)}</span>
                                    <span className="bid-name">{isMe ? `${bid.name} (YOU)` : bid.name}</span>
                                    <span className="bid-value">${bid.bid >= 0 ? bid.bid.toLocaleString() : '--'}</span>
                                    <span className={'bid-delta ' + (bid.score_delta > 0 ? 'positive' : '')}>
                                        {bid.score_delta > 0 ? `+${bid.score_delta}` : '0'}
                                    </span>
                                </div>
                            );
                        })}
                    </div>

                    <p className="waiting-next">Next product in 3 seconds...</p>
                </div>
            </div>
        );
    }

    //==========================================================================
    // RENDER: Summary screen
    //==========================================================================
    if (gamePhase === 'summary') {
        const allPlayers = summaryData?.players || [];
        const eliminatedPlayers = allPlayers.filter(p => p.eliminated === true);
        const activePlayers = allPlayers.filter(p => !p.eliminated).sort((a, b) => b.score - a.score);
        
        const amIEliminated = eliminatedPlayers.some(p => Number(p.id) === Number(playerId));

        return (
            <div className="round2-wrapper-quiz">
                <div className="summary-content">
                    <div className="summary-header">
                        <h1 className="summary-title">🏆 ROUND 2 COMPLETE! 🏆</h1>
                        <div className="summary-countdown">
                            Next round in: <span className="countdown-number">{summaryCountdown}s</span>
                        </div>
                    </div>

                    {amIEliminated && (
                        <div className="eliminated-banner">
                            ❌ YOU HAVE BEEN ELIMINATED ❌
                        </div>
                    )}

                    <div className="summary-scoreboard">
                        <h2 className="scoreboard-title">FINAL SCOREBOARD</h2>
                        <div className="player-list">
                            {activePlayers.map((player, index) => {
                                const isMe = Number(player.id) === Number(playerId);
                                return (
                                    <div
                                        key={player.id || index}
                                        className={'player-row ' + (index === 0 ? 'winner ' : '') + (isMe ? 'is-you ' : '')}
                                    >
                                        <div className="player-rank">
                                            {index === 0 ? '🥇' : index === 1 ? '🥈' : index === 2 ? '🥉' : '#' + (index + 1)}
                                        </div>
                                        <div className="player-name">
                                            {isMe ? (player.name + ' (YOU)') : player.name}
                                        </div>
                                        <div className="player-score">{player.score} pts</div>
                                    </div>
                                );
                            })}

                            {eliminatedPlayers.map((player) => {
                                const isMe = Number(player.id) === Number(playerId);
                                return (
                                    <div key={'elim-' + player.id} className="player-row eliminated">
                                        <div className="player-rank">🚫</div>
                                        <div className="player-name">
                                            {isMe ? (player.name + ' (YOU)') : player.name}
                                            <span className="eliminated-tag">ELIMINATED</span>
                                        </div>
                                        <div className="player-score">{player.score} pts</div>
                                    </div>
                                );
                            })}
                        </div>
                    </div>

                    <button className="skip-btn" onClick={handleSkipToNextRound}>
                        {amIEliminated ? 'SPECTATE ROUND 3 →' : 'CONTINUE TO ROUND 3 →'}
                    </button>
                </div>
            </div>
        );
    }

    //==========================================================================
    // RENDER: Eliminated screen
    //==========================================================================
    if (gamePhase === 'eliminated') {
        return (
            <div className="round2-wrapper-quiz">
                <div className="eliminated-content">
                    <h1 className="eliminated-title">❌ ELIMINATED ❌</h1>
                    <p className="eliminated-subtitle">You have been eliminated</p>
                    <div className="eliminated-info-box">
                        <div className="eliminated-icon">😢</div>
                        <h2>Better luck next time!</h2>
                        <p>Your final score: <strong>{score} pts</strong></p>
                    </div>
                </div>
            </div>
        );
    }

    //==========================================================================
    // RENDER: Loading product
    //==========================================================================
    if (!productData) {
        return (
            <div className="round2-wrapper-quiz">
                <div className="quiz-content">
                    <div className="loading">Loading product {currentProductIdx + 1}...</div>
                </div>
            </div>
        );
    }

    //==========================================================================
    // RENDER: Bid UI
    //==========================================================================
    return (
        <div className="round2-wrapper-quiz">
            <div className="quiz-content">
                <div className="question-header-row">
                    <div className="question-number-header">
                        PRODUCT {currentProductIdx + 1} / {totalProducts}
                    </div>
                    {isWagerAvailable && (
                        <button 
                            className={'wager-badge-btn ' + (isWagerActive ? 'active' : '')}
                            onClick={handleWagerToggle}
                        >
                            ⭐ WAGER
                        </button>
                    )}
                </div>

                <h2 className="quiz-question">{productData.question}</h2>

                <div className="product-image-container">
                    <img 
                        src={productData.product_image} 
                        alt="Product" 
                        className="product-image" 
                    />
                </div>

                <div className="input-section">
                    <div className="bid-input-wrapper">
                        <span className="currency-symbol">$</span>
                        <input 
                            type="number" 
                            className={'answer-input ' + (hasBid ? 'submitted' : '')}
                            placeholder="Enter your bid..."
                            value={bidValue}
                            onChange={(e) => setBidValue(e.target.value)}
                            disabled={hasBid}
                        />
                    </div>
                    <button 
                        className={'submit-answer-btn ' + (hasBid ? 'submitted' : '')}
                        onClick={handleSubmit}
                        disabled={hasBid || !bidValue}
                    >
                        {hasBid ? '✓ BID SENT' : 'SUBMIT'}
                    </button>
                </div>

                {hasBid && (
                    <div className="waiting-others">
                        <div className="connecting-spinner">⏳</div>
                        <span>Waiting for other players to bid...</span>
                    </div>
                )}

                <div className="score-display">
                    Total Score: {score}
                </div>
            </div>
        </div>
    );
};

export default Round2;
