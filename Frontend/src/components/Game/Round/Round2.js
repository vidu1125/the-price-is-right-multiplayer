import React, { useState, useEffect, useRef, useCallback } from 'react';
import './Round2.css';
// Import ensures handlers are registered
import '../../../services/round2Service';
import { getProduct, submitBid, playerReady } from '../../../services/round2Service';
import { waitForConnection } from '../../../network/socketClient';

const Round2 = ({ matchId = 1, playerId = 1, previousScore = 0, onRoundComplete }) => {
    // Game phase: 'connecting' -> 'playing' -> 'result' -> 'summary'
    const [gamePhase, setGamePhase] = useState('connecting');

    // Connection state
    const [connectedPlayers, setConnectedPlayers] = useState([]);
    const [connectionCountdown, setConnectionCountdown] = useState(10);

    // Product state
    const [currentProductIdx, setCurrentProductIdx] = useState(0);
    const [productData, setProductData] = useState(null);
    const [totalProducts, setTotalProducts] = useState(5);
    const [timeLeft, setTimeLeft] = useState(15);
    const [score, setScore] = useState(previousScore);

    // Bid state
    const [bidValue, setBidValue] = useState('');
    const [hasBid, setHasBid] = useState(false);
    const [isWagerActive, setIsWagerActive] = useState(false); 

    // Turn result state
    const [turnResult, setTurnResult] = useState(null);
    const [correctPrice, setCorrectPrice] = useState(null);

    // Summary screen state
    const [summaryData, setSummaryData] = useState(null);
    // ⭐ HARDCODE FOR TESTING: Reduced countdown (3 seconds)
    const [summaryCountdown, setSummaryCountdown] = useState(3);
    const [activePlayers, setActivePlayers] = useState([]);

    const startTime = useRef(null);
    const timerRef = useRef(null);
    const summaryTimerRef = useRef(null);
    const connectionTimerRef = useRef(null);
    const productReceivedRef = useRef(false);
    const currentIdxRef = useRef(0);
    const scoreRef = useRef(previousScore);
    const gameStartedRef = useRef(false);
    const showingResultRef = useRef(false);
    
    // Sync scoreRef when previousScore changes
    useEffect(() => {
        scoreRef.current = previousScore;
        setScore(previousScore);
    }, [previousScore]);

    //==========================================================================
    // AUTO-CONNECT: Khi vào round 2, chờ socket và gửi ready
    //==========================================================================
    useEffect(() => {
        console.log('[Round2] Auto-connecting player ' + playerId + ' to match ' + matchId);

        let isMounted = true;
        let retryInterval = null;

        const connectAndReady = async () => {
            try {
                console.log('[Round2] Waiting for socket connection...');
                await waitForConnection(5000);
                console.log('[Round2] Socket connected! Sending playerReady...');

                if (!isMounted) return;

                playerReady(matchId, playerId);

                retryInterval = setInterval(() => {
                    if (!gameStartedRef.current && isMounted) {
                        console.log('[Round2] Retry sending playerReady...');
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
    // LISTEN: Ready status updates
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
    // LISTEN: All ready - game starts
    //==========================================================================
    useEffect(() => {
        const handleAllReady = (e) => {
            const data = e.detail;
            console.log('[Round2] 🎮 All ready! Starting round 2:', data);

            gameStartedRef.current = true;
            setTotalProducts(data.total_products || 5);

            if (connectionTimerRef.current) clearInterval(connectionTimerRef.current);

            setGamePhase('playing');
            setCurrentProductIdx(0);
            currentIdxRef.current = 0;
            productReceivedRef.current = false;

            // First product will come via S2C_ROUND2_PRODUCT
            // Fallback: Request product if not received within 2 seconds
            setTimeout(() => {
                if (!productReceivedRef.current && gameStartedRef.current) {
                    console.log('[Round2] Product not received, requesting...');
                    getProduct(matchId, 0);
                }
            }, 2000);
        };

        window.addEventListener('round2AllReady', handleAllReady);
        return () => window.removeEventListener('round2AllReady', handleAllReady);
    }, [matchId]);

    //==========================================================================
    // LISTEN: Product data
    //==========================================================================
    useEffect(() => {
        const handleProduct = (e) => {
            const data = e.detail;
            console.log('[Round2] Product received:', data);

            if (showingResultRef.current) {
                console.log('[Round2] Ignoring product - still showing result');
                return;
            }

            if (data.product_idx !== undefined) {
                console.log('[Round2] Syncing product index from server:', data.product_idx);
                currentIdxRef.current = data.product_idx;
                setCurrentProductIdx(data.product_idx);
            }

            setProductData(data);
            setHasBid(false);
            setBidValue('');
            setTurnResult(null);
            setCorrectPrice(null);
            productReceivedRef.current = true;

            // Start timer
            startTime.current = Date.now();
            setTimeLeft(Math.ceil((data.time_limit_ms || 15000) / 1000));

            if (timerRef.current) clearInterval(timerRef.current);
            timerRef.current = setInterval(() => {
                setTimeLeft(prev => {
                    if (prev <= 1) {
                        clearInterval(timerRef.current);
                        return 0;
                    }
                    window.dispatchEvent(new CustomEvent('timerUpdate', { detail: { timeLeft: prev - 1 } }));
                    return prev - 1;
                });
            }, 1000);
        };

        window.addEventListener('round2Product', handleProduct);
        return () => window.removeEventListener('round2Product', handleProduct);
    }, [matchId]);

    //==========================================================================
    // LISTEN: Bid acknowledged
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
    // LISTEN: Turn result (all bids revealed)
    //==========================================================================
    useEffect(() => {
        const handleTurnResult = (e) => {
            const data = e.detail;
            console.log('[Round2] Turn result:', data);

            if (timerRef.current) clearInterval(timerRef.current);

            setTurnResult(data);
            setCorrectPrice(data.correct_price);
            showingResultRef.current = true;
            setGamePhase('result');

            // Find my score delta
            const myResult = data.bids?.find(b => b.id === playerId);
            if (myResult) {
                scoreRef.current += myResult.score_delta;
                setScore(scoreRef.current);

                console.log('[Round2] Score: +' + myResult.score_delta + ', Total: ' + scoreRef.current);

                // Dispatch score update with current total (use scoreRef.current for consistency)
                window.dispatchEvent(new CustomEvent('round2ScoreUpdate', {
                    detail: { score: myResult.score_delta, totalScore: scoreRef.current }
                }));
            }

            // After 2 seconds (less than backend's 3s), prepare for next
            setTimeout(() => {
                showingResultRef.current = false;
                const nextIdx = currentIdxRef.current + 1;
                if (nextIdx < (data.total_products || totalProducts)) {
                    setGamePhase('playing');
                    productReceivedRef.current = false;
                    // Server will send next product
                } else {
                    console.log('[Round2] All products done, waiting for final results...');
                    setGamePhase('waiting');
                }
            }, 2000);
        };

        window.addEventListener('round2TurnResult', handleTurnResult);
        return () => window.removeEventListener('round2TurnResult', handleTurnResult);
    }, [playerId, totalProducts]);

    //==========================================================================
    // LISTEN: All finished - Show summary
    //==========================================================================
    useEffect(() => {
        const handleAllFinished = (e) => {
            const data = e.detail;
            console.log('[Round2] 🏆 ALL_FINISHED received!', data);

            // Clear any pending timers
            if (timerRef.current) clearInterval(timerRef.current);
            showingResultRef.current = false;

            const allPlayers = data.players || data.rankings || [];
            setActivePlayers(allPlayers.filter(p => p.connected !== false));
            setSummaryData(data);
            setGamePhase('summary');
            setSummaryCountdown(5); // 5 seconds to view summary

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
    // Summary countdown end - move to next round
    //==========================================================================
    useEffect(() => {
        if (gamePhase === 'summary' && summaryCountdown === 0) {
            if (onRoundComplete) {
                // Use next_round from server, default to 3
                const nextRound = summaryData?.next_round || 3;
                console.log('[Round2] Calling onRoundComplete with nextRound:', nextRound);
                onRoundComplete(nextRound, {
                    score: scoreRef.current,
                    playerCount: activePlayers.length
                });
            }
        }
    }, [gamePhase, summaryCountdown, activePlayers, onRoundComplete, summaryData]);

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
    const handleSubmitBid = useCallback(() => {
        if (hasBid || !bidValue) return;

        const bidNum = parseInt(bidValue.replace(/[^0-9]/g, ''), 10);
        if (isNaN(bidNum) || bidNum <= 0) {
            console.log('[Round2] Invalid bid value');
            return;
        }

        console.log('[Round2] Submitting bid:', bidNum);
        submitBid(matchId, currentIdxRef.current, bidNum);
        setHasBid(true);
    }, [matchId, bidValue, hasBid]);

    //==========================================================================
    // HANDLER: Skip to next round
    //==========================================================================
    const handleSkipToNextRound = () => {
        clearInterval(summaryTimerRef.current);
        if (onRoundComplete) {
            const nextRound = summaryData?.next_round || 3;
            console.log('[Round2] Skip to next round:', nextRound);
            onRoundComplete(nextRound, { score: scoreRef.current });
        }
    };

    //==========================================================================
    // Format currency
    //==========================================================================
    const formatCurrency = (value) => {
        if (!value) return '';
        return new Intl.NumberFormat('vi-VN').format(value) + ' đ';
    };

    //==========================================================================
    // RENDER: Connecting screen
    //==========================================================================
    if (gamePhase === 'connecting') {
        return (
            <div className="round2-wrapper-quiz">
                <div className="connecting-content">
                    <h1 className="connecting-title">ROUND 2: THE BID</h1>
                    <div className="connecting-status">
                        <div className="connecting-spinner"></div>
                        <p>Connecting to game server...</p>
                        <p className="countdown-text">
                            {connectionCountdown > 0 ? `Starting in ${connectionCountdown}s` : 'Waiting for players...'}
                        </p>
                    </div>
                    <div className="player-list">
                        <h3>Connected Players ({connectedPlayers.length})</h3>
                        {connectedPlayers.map((p, i) => (
                            <div key={i} className={`player-item ${p.ready ? 'ready' : ''}`}>
                                {p.account_id === playerId ? '👤 YOU' : `Player ${p.account_id}`}
                                {p.ready && ' ✓'}
                            </div>
                        ))}
                    </div>
                </div>
            </div>
        );
    }

    //==========================================================================
    // RENDER: Turn result screen
    //==========================================================================
    if (gamePhase === 'result' && turnResult) {
        return (
            <div className="round2-wrapper-quiz">
                <div className="result-content">
                    <h2 className="result-title">TURN RESULT</h2>
                    <div className="correct-price">
                        <span>Correct Price:</span>
                        <strong>{formatCurrency(correctPrice)}</strong>
                    </div>
                    <div className="bids-list">
                        {turnResult.bids?.map((bid, idx) => {
                            const isMe = bid.id === playerId;
                            const isWinner = bid.score_delta > 0;
                            return (
                                <div key={idx} className={`bid-item ${isMe ? 'is-me' : ''} ${isWinner ? 'winner' : ''} ${bid.overbid ? 'overbid' : ''}`}>
                                    <span className="bid-player">{bid.name || `Player ${bid.id}`}</span>
                                    <span className="bid-value">{formatCurrency(bid.bid)}</span>
                                    <span className="bid-score">+{bid.score_delta}</span>
                                    {bid.overbid && <span className="overbid-badge">OVER</span>}
                                </div>
                            );
                        })}
                    </div>
                    <p className="result-next">Next product in 3 seconds...</p>
                </div>
            </div>
        );
    }

    //==========================================================================
    // RENDER: Waiting for final results
    //==========================================================================
    if (gamePhase === 'waiting') {
        return (
            <div className="round2-wrapper-quiz">
                <div className="connecting-content">
                    <h1 className="connecting-title">ROUND 2 COMPLETE!</h1>
                    <div className="connecting-status">
                        <div className="connecting-spinner"></div>
                        <p>Calculating final scores...</p>
                    </div>
                </div>
            </div>
        );
    }

    //==========================================================================
    // RENDER: Summary screen
    //==========================================================================
    if (gamePhase === 'summary') {
        let playerList = [...activePlayers].sort((a, b) => b.score - a.score);

        return (
            <div className="round2-wrapper-quiz">
                <div className="summary-content">
                    <div className="summary-header">
                        <h1 className="summary-title">🏆 ROUND 2 COMPLETE! 🏆</h1>
                        <div className="summary-countdown">
                            Next round in: <span className="countdown-number">{summaryCountdown}s</span>
                        </div>
                    </div>

                    <div className="summary-scoreboard">
                        <h2 className="scoreboard-title">FINAL SCOREBOARD</h2>
                        <div className="player-list">
                            {playerList.map((player, index) => {
                                const isMe = player.id === playerId;
                                return (
                                    <div key={player.id} className={`player-row ${isMe ? 'is-me' : ''} ${player.eliminated ? 'eliminated' : ''}`}>
                                        <span className="player-rank">#{index + 1}</span>
                                        <span className="player-name">{isMe ? `${player.name} (YOU)` : player.name}</span>
                                        <span className="player-score">{player.score}</span>
                                        {player.eliminated && <span className="eliminated-badge">ELIMINATED</span>}
                                    </div>
                                );
                            })}
                        </div>
                    </div>

                    <button className="skip-button" onClick={handleSkipToNextRound}>
                        SKIP → ROUND 3
                    </button>
                </div>
            </div>
        );
    }

    //==========================================================================
    // RENDER: Waiting for product (playing but no product data yet)
    //==========================================================================
    if (gamePhase === 'playing' && !productData) {
        return (
            <div className="round2-wrapper-quiz">
                <div className="connecting-content">
                    <h1 className="connecting-title">ROUND 2: THE BID</h1>
                    <div className="connecting-status">
                        <div className="connecting-spinner"></div>
                        <p>Loading product...</p>
                    </div>
                </div>
            </div>
        );
    }

    //==========================================================================
    // RENDER: Playing (bid input)
    //==========================================================================
    return (
        <div className="round2-wrapper-quiz">
            <div className="quiz-content">
                <div className="question-header-row">
                    <div className="question-number-header">
                        PRODUCT {currentProductIdx + 1} / {totalProducts}
                    </div>
                    {!hasBid && (
                        <button 
                        className={`wager-badge-btn ${isWagerActive ? 'active' : ''}`} 
                            onClick={() => setIsWagerActive(!isWagerActive)}
                        >
                        ⭐ WAGER
                        </button>
                    )}
                </div>

                <div className="question-box">
                    <h2 className="quiz-question">
                        {productData?.question || "What is the price of this product?"}
                    </h2>
                </div>

                {/* Only show image if product has actual image data */}
                {productData?.product_image && (
                    <div className="product-image-container">
                        <img 
                            src={productData.product_image} 
                            alt="Product" 
                            className="product-image" 
                        />
                    </div>
                )}

                <div className="input-section">
                    <input 
                        type="text" 
                        className={`answer-input ${hasBid ? 'submitted' : ''}`}
                        placeholder="Enter your bid..."
                        value={bidValue}
                        onChange={(e) => setBidValue(e.target.value)}
                        disabled={hasBid}
                        onKeyPress={(e) => e.key === 'Enter' && handleSubmitBid()}
                    />
                    <button 
                        className={`submit-answer-btn ${hasBid ? 'submitted' : ''}`}
                        onClick={handleSubmitBid}
                        disabled={hasBid || !bidValue}
                    >
                        {hasBid ? 'BID SUBMITTED ✓' : 'SUBMIT BID'}
                    </button>
                </div>

                {hasBid && (
                    <div className="waiting-message">
                        <div className="connecting-spinner"></div>
                        <p>Waiting for other players...</p>
                    </div>
                )}
            </div>
        </div>
    );
};

export default Round2;
