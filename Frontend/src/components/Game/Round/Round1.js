import React, { useState, useEffect, useRef, useCallback } from 'react';
import './Round1.css';
// Import ensures handlers are registered
import '../../../services/round1Service';
import { getQuestion, submitAnswer, endRound1, playerReady } from '../../../services/round1Service';
import { waitForConnection } from '../../../network/socketClient';

const Round1 = ({ matchId = 1, playerId = 1, previousScore = 0, onRoundComplete }) => {
    // Game phase: 'connecting' -> 'playing' -> 'waiting' -> 'summary'
    const [gamePhase, setGamePhase] = useState('connecting');

    // Connection state
    const [connectedPlayers, setConnectedPlayers] = useState([]);
    // ⭐ HARDCODE FOR TESTING: Reduced countdown (3 seconds)
    const [connectionCountdown, setConnectionCountdown] = useState(3);

    // Question state
    const [currentQuestionIdx, setCurrentQuestionIdx] = useState(0);
    const [questionData, setQuestionData] = useState(null);
    const [selectedAnswer, setSelectedAnswer] = useState(null);
    const [timeLeft, setTimeLeft] = useState(15);
    const [score, setScore] = useState(previousScore);
    const [isAnswered, setIsAnswered] = useState(false);
    const [correctIndex, setCorrectIndex] = useState(null);

    // Waiting state
    const [finishedCount, setFinishedCount] = useState(0);
    const [playerCount, setPlayerCount] = useState(0);

    // Summary screen state
    const [summaryData, setSummaryData] = useState(null);
    // ⭐ HARDCODE FOR TESTING: Reduced countdown (3 seconds)
    const [summaryCountdown, setSummaryCountdown] = useState(3);
    const [activePlayers, setActivePlayers] = useState([]); // Track connected players
    const [disconnectedPlayers, setDisconnectedPlayers] = useState([]); // Track who disconnected

    const startTime = useRef(null);
    const timerRef = useRef(null);
    const summaryTimerRef = useRef(null);
    const connectionTimerRef = useRef(null);
    const questionReceivedRef = useRef(false);
    const currentIdxRef = useRef(0);
    const scoreRef = useRef(previousScore);
    const gameStartedRef = useRef(false);
    const showingResultRef = useRef(false); // Track if we're showing answer result
    
    // Sync scoreRef when previousScore changes
    useEffect(() => {
        scoreRef.current = previousScore;
        setScore(previousScore);
    }, [previousScore]);

    //==========================================================================
    // AUTO-CONNECT: Khi vào game, chờ socket kết nối rồi gửi signal
    //==========================================================================
    useEffect(() => {
        console.log('[Round1] Auto-connecting player ' + playerId + ' to match ' + matchId);

        let isMounted = true;
        let retryInterval = null;

        const connectAndReady = async () => {
            try {
                console.log('[Round1] Waiting for socket connection...');
                await waitForConnection(5000);
                console.log('[Round1] Socket connected! Sending playerReady...');

                if (!isMounted) return;

                // Gửi playerReady
                playerReady(matchId, playerId);

                // Retry mỗi 1s cho đến khi game start
                retryInterval = setInterval(() => {
                    if (!gameStartedRef.current && isMounted) {
                        console.log('[Round1] Retry sending playerReady...');
                        playerReady(matchId, playerId);
                    } else {
                        clearInterval(retryInterval);
                    }
                }, 1000);

            } catch (err) {
                console.error('[Round1] Failed to connect:', err);
            }
        };

        connectAndReady();

        // ⭐ HARDCODE FOR TESTING: Reduced countdown (3s)
        setConnectionCountdown(3);
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
    // LISTEN: Connection status update (thay thế ready status)
    //==========================================================================
    useEffect(() => {
        const handleReadyStatus = (e) => {
            const data = e.detail;
            console.log('[Round1] Connection status:', data);
            setConnectedPlayers(data.players || []);
            setPlayerCount(data.player_count || 0);
        };

        window.addEventListener('round1ReadyStatus', handleReadyStatus);
        return () => window.removeEventListener('round1ReadyStatus', handleReadyStatus);
    }, []);

    //==========================================================================
    // LISTEN: All players connected - Start game automatically
    //==========================================================================
    useEffect(() => {
        const handleAllReady = (e) => {
            const data = e.detail;
            console.log('[Round1] All connected, starting game!', data);

            if (gameStartedRef.current) return;
            gameStartedRef.current = true;

            if (connectionTimerRef.current) clearInterval(connectionTimerRef.current);
            setGamePhase('playing');
            setPlayerCount(data.player_count || 4);

            // Start loading first question
            setTimeout(() => {
                questionReceivedRef.current = false;
                currentIdxRef.current = 0;
                setCurrentQuestionIdx(0);
                getQuestion(matchId, 0);
            }, 500);
        };

        window.addEventListener('round1AllReady', handleAllReady);
        return () => window.removeEventListener('round1AllReady', handleAllReady);
    }, [matchId]);

    //==========================================================================
    // HANDLER: Timeout
    //==========================================================================
    const handleTimeout = useCallback(() => {
        console.log('[Round1] Timeout! idx=' + currentIdxRef.current);
        submitAnswer(matchId, currentIdxRef.current, 255, 15000);
    }, [matchId]);

    //==========================================================================
    // LISTEN: Question received
    //==========================================================================
    useEffect(() => {
        const handleQuestion = (e) => {
            const data = e.detail;
            console.log('[Round1] Question received:', data);

            if (!data.success) {
                console.error('[Round1] Failed to get question:', data.error);
                return;
            }

            // IGNORE new questions while showing result (wait for 2s delay)
            if (showingResultRef.current) {
                console.log('[Round1] Ignoring question - still showing result');
                return;
            }

            questionReceivedRef.current = true;

            // SYNC question index from server (important for timeout auto-advance)
            if (data.question_idx !== undefined) {
                console.log('[Round1] Syncing question index from server:', data.question_idx);
                currentIdxRef.current = data.question_idx;
                setCurrentQuestionIdx(data.question_idx);
            }

            setQuestionData(data);
            setSelectedAnswer(null);
            setIsAnswered(false);
            setTimeLeft(15);
            setCorrectIndex(null);
            startTime.current = Date.now();

            // Dispatch initial timer
            window.dispatchEvent(new CustomEvent('timerUpdate', { detail: { timeLeft: 15 } }));

            if (timerRef.current) clearInterval(timerRef.current);
            timerRef.current = setInterval(() => {
                setTimeLeft(prev => {
                    const newTime = prev <= 1 ? 0 : prev - 1;

                    // Dispatch timer update to GameContainer
                    window.dispatchEvent(new CustomEvent('timerUpdate', { detail: { timeLeft: newTime } }));

                    if (newTime <= 0) {
                        clearInterval(timerRef.current);
                        handleTimeout();
                    }
                    return newTime;
                });
            }, 1000);
        };

        window.addEventListener('round1Question', handleQuestion);
        return () => window.removeEventListener('round1Question', handleQuestion);
    }, [handleTimeout]);

    //==========================================================================
    // LISTEN: Answer result - Load next question
    //==========================================================================
    useEffect(() => {
        const handleResult = (e) => {
            const data = e.detail;
            console.log('[Round1] Answer result:', data);

            clearInterval(timerRef.current);
            setIsAnswered(true);
            setCorrectIndex(data.correct_index);
            showingResultRef.current = true; // Block new questions while showing result

            console.log('[Round1] Correct index:', data.correct_index);

            // Add score
            const addedScore = data.score || 0;
            scoreRef.current += addedScore;
            setScore(scoreRef.current);

            console.log('[Round1] Score: +' + addedScore + ', Total: ' + scoreRef.current);

            // Dispatch score update
            window.dispatchEvent(new CustomEvent('round1ScoreUpdate', {
                detail: { score: addedScore, totalScore: scoreRef.current }
            }));

            // Wait 2s then load next question
            setTimeout(() => {
                showingResultRef.current = false; // Allow new questions now
                const nextIdx = currentIdxRef.current + 1;
                const totalQ = data.total_questions || questionData?.total_questions || 10;

                console.log('[Round1] Next question check: nextIdx=' + nextIdx + ', total=' + totalQ);

                if (nextIdx < totalQ) {
                    currentIdxRef.current = nextIdx;
                    setCurrentQuestionIdx(nextIdx);
                    questionReceivedRef.current = false;
                    getQuestion(matchId, nextIdx);
                } else {
                    console.log('[Round1] All ' + totalQ + ' questions done! Final score: ' + scoreRef.current);
                    setGamePhase('waiting');
                    endRound1(matchId, playerId);
                }
            }, 2000);
        };

        window.addEventListener('round1Result', handleResult);
        return () => window.removeEventListener('round1Result', handleResult);
    }, [matchId, playerId]);

    //==========================================================================
    // LISTEN: Waiting status (handles both normal waiting and reconnection)
    //==========================================================================
    useEffect(() => {
        const handleWaiting = (e) => {
            const data = e.detail;
            console.log('[Round1] ⏳ WAITING status received:', data);
            setFinishedCount(data.finished_count || 0);
            setPlayerCount(data.player_count || 0);

            // Check if this is a reconnection response
            if (data.reconnected) {
                console.log('[Round1] 🔄🔄🔄 RECONNECTED! Showing waiting screen...');
                console.log('[Round1] Your saved score:', data.your_score);
                console.log('[Round1] finished_count:', data.finished_count, 'player_count:', data.player_count);
                setGamePhase('reconnect_waiting');
                // Store reconnection score if available
                if (data.your_score !== undefined) {
                    setScore(data.your_score);
                    scoreRef.current = data.your_score;
                }
            }
        };

        window.addEventListener('round1Waiting', handleWaiting);
        console.log('[Round1] 👂 Registered round1Waiting listener');
        return () => window.removeEventListener('round1Waiting', handleWaiting);
    }, []);

    //==========================================================================
    // LISTEN: All finished - Show summary (10 giây)
    //==========================================================================
    useEffect(() => {
        const handleAllFinished = (e) => {
            const data = e.detail;
            console.log('[Round1] 🏆🏆🏆 ALL_FINISHED received! Current phase:', gamePhase);
            console.log('[Round1] Scoreboard data:', data);

            setSummaryData(data);

            // Separate connected and disconnected players from server response
            const allPlayers = data.players || [];
            const connected = allPlayers.filter(p => p.connected !== false);
            const disconnected = allPlayers.filter(p => p.connected === false);

            setActivePlayers(connected);
            setDisconnectedPlayers(disconnected);
            setGamePhase('summary');
            // ⭐ HARDCODE FOR TESTING: Reduced countdown (3 seconds)
            setSummaryCountdown(3);

            console.log('[Round1] Active players:', connected.length, 'Disconnected:', disconnected.length);

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

        window.addEventListener('round1AllFinished', handleAllFinished);
        return () => {
            window.removeEventListener('round1AllFinished', handleAllFinished);
            if (summaryTimerRef.current) clearInterval(summaryTimerRef.current);
        };
    }, []);

    //==========================================================================
    // LISTEN: Player disconnected during game (real-time update)
    //==========================================================================
    useEffect(() => {
        const handlePlayerDisconnected = (e) => {
            const { playerId: disconnectedId, playerName } = e.detail;
            // Convert to number for comparison (server sends number, but could be string)
            const dcId = Number(disconnectedId);
            console.log('[Round1] 🔴 PLAYER DISCONNECT EVENT! ID:', dcId, 'Name:', playerName);

            // Find and move player from active to disconnected
            setActivePlayers(prev => {
                console.log('[Round1] Before disconnect - activePlayers:', prev.map(p => `${p.id}:${p.name}`));
                const disconnectedPlayer = prev.find(p => Number(p.id) === dcId);

                if (disconnectedPlayer) {
                    console.log('[Round1] ✅ Found player to disconnect:', disconnectedPlayer.name);

                    // Create player info for disconnected list
                    const dcPlayerInfo = {
                        ...disconnectedPlayer,
                        connected: false,
                        name: playerName || disconnectedPlayer.name || `Player${dcId}`
                    };

                    // Update disconnected list
                    setDisconnectedPlayers(prevDisc => {
                        const alreadyInList = prevDisc.some(p => Number(p.id) === dcId);
                        if (!alreadyInList) {
                            console.log('[Round1] Adding to disconnected list:', dcPlayerInfo.name);
                            return [...prevDisc, dcPlayerInfo];
                        }
                        return prevDisc;
                    });

                    // Remove from active list
                    const newActive = prev.filter(p => Number(p.id) !== dcId);
                    console.log('[Round1] After disconnect - activePlayers count:', newActive.length);
                    return newActive;
                } else {
                    console.log('[Round1] ⚠️ Player not found in active list, already disconnected?');
                }
                return prev;
            });
        };

        window.addEventListener('playerDisconnected', handlePlayerDisconnected);
        console.log('[Round1] 👂 Registered playerDisconnected listener');

        return () => {
            window.removeEventListener('playerDisconnected', handlePlayerDisconnected);
            console.log('[Round1] 🔇 Unregistered playerDisconnected listener');
        };
    }, []); // Empty deps - listener stays for component lifetime

    //==========================================================================
    // LISTEN: Player reconnected (move from disconnected back to active)
    //==========================================================================
    useEffect(() => {
        const handlePlayerReconnected = (e) => {
            const { playerId: reconnectedId, playerName } = e.detail;
            const rcId = Number(reconnectedId);
            console.log('[Round1] 🔄 Player RECONNECTED! ID:', rcId, 'Name:', playerName);

            // Move player from disconnected back to active
            setDisconnectedPlayers(prev => {
                const reconnectedPlayer = prev.find(p => Number(p.id) === rcId);

                if (reconnectedPlayer) {
                    console.log('[Round1] Moving player from disconnected to active:', reconnectedPlayer.name);

                    // Add back to active players
                    setActivePlayers(prevActive => {
                        const alreadyActive = prevActive.some(p => Number(p.id) === rcId);
                        if (!alreadyActive) {
                            return [...prevActive, {
                                ...reconnectedPlayer,
                                connected: true,
                                name: playerName || reconnectedPlayer.name
                            }];
                        }
                        return prevActive;
                    });

                    // Remove from disconnected
                    return prev.filter(p => Number(p.id) !== rcId);
                }
                return prev;
            });
        };

        window.addEventListener('playerReconnected', handlePlayerReconnected);
        return () => window.removeEventListener('playerReconnected', handlePlayerReconnected);
    }, []);

    //==========================================================================
    // Handle countdown end - Check player count and proceed
    //==========================================================================
    useEffect(() => {
        if (gamePhase === 'summary' && summaryCountdown === 0) {
            const activeCount = activePlayers.length;
            console.log('[Round1] Summary countdown ended. Active players:', activeCount);

            if (activeCount < 2) {
                console.log('[Round1] Not enough players! Only ' + activeCount + ' player(s)');
                window.dispatchEvent(new CustomEvent('notEnoughPlayers', {
                    detail: { playerCount: activeCount, required: 2 }
                }));
            }

            if (onRoundComplete) {
                // Use next_round from server, default to 2
                const nextRound = summaryData?.next_round || 2;
                console.log('[Round1] Calling onRoundComplete with nextRound:', nextRound);
                onRoundComplete(nextRound, {
                    score: scoreRef.current,
                    playerCount: activeCount,
                    disconnectedPlayers: disconnectedPlayers
                });
            }
        }
    }, [gamePhase, summaryCountdown, activePlayers, disconnectedPlayers, onRoundComplete, summaryData]);

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
    // HANDLER: Answer clicked
    //==========================================================================
    const handleAnswerClick = (index) => {
        if (isAnswered) return;

        const timeTaken = Date.now() - startTime.current;
        setSelectedAnswer(index);

        submitAnswer(matchId, currentIdxRef.current, index, timeTaken);
    };

    //==========================================================================
    // HANDLER: Skip to next round
    //==========================================================================
    const handleSkipToNextRound = () => {
        clearInterval(summaryTimerRef.current);
        if (onRoundComplete) {
            const nextRound = summaryData?.next_round || 2;
            console.log('[Round1] Skip to next round:', nextRound);
            onRoundComplete(nextRound, { score: scoreRef.current });
        }
    };

    //==========================================================================
    // RENDER: Connecting screen (tự động kết nối, không cần ấn Ready)
    //==========================================================================
    if (gamePhase === 'connecting') {
        return (
            <div className="round1-wrapper-quiz">
                <div className="ready-content">
                    <h1 className="ready-title">🎮 ROUND 1 - QUIZ 🎮</h1>
                    <p className="ready-subtitle">Connecting to game...</p>

                    <div className="player-status-list">
                        <h3>Players Connected ({connectedPlayers.filter(p => p.is_ready).length}/{connectedPlayers.length || 4})</h3>
                        {connectedPlayers.length > 0 ? (
                            connectedPlayers.map((p, idx) => (
                                <div key={idx} className={'player-status-item ' + (p.is_ready ? 'ready' : 'not-ready')}>
                                    <span className="player-status-name">
                                        {p.id === playerId ? '👤 ' + p.name + ' (YOU)' : p.name}
                                    </span>
                                    <span className="player-status-badge">
                                        {p.is_ready ? ' Connected' : '⏳ Connecting...'}
                                    </span>
                                </div>
                            ))
                        ) : (
                            <div className="waiting-for-players">
                                <div className="connecting-spinner"></div>
                                Waiting for players to connect...
                            </div>
                        )}
                    </div>

                    <div className="connecting-status">
                        <div className="connecting-spinner"></div>
                        <p>Auto-starting when all players connect...</p>
                        {connectionCountdown > 0 && (
                            <p className="connection-countdown">Timeout in {connectionCountdown}s</p>
                        )}
                    </div>

                    <div className="ready-info">
                        <p>Match ID: {matchId} | Player: {playerId}</p>
                    </div>
                </div>
            </div>
        );
    }

    //==========================================================================
    // RENDER: Waiting screen (finished, waiting for others)
    //==========================================================================
    if (gamePhase === 'waiting') {
        return (
            <div className="round1-wrapper-quiz">
                <div className="waiting-content">
                    <h1 className="waiting-title">⏳ COMPLETED! ⏳</h1>
                    <p className="waiting-subtitle">Your Score: {score} pts</p>

                    <div className="waiting-progress">
                        <div className="progress-text">
                            Waiting for other players... ({finishedCount}/{playerCount})
                        </div>
                        <div className="progress-bar">
                            <div
                                className="progress-fill"
                                style={{ width: (playerCount > 0 ? (finishedCount / playerCount) * 100 : 0) + '%' }}
                            />
                        </div>
                    </div>

                    <div className="waiting-spinner"></div>
                </div>
            </div>
        );
    }

    //==========================================================================
    // RENDER: Reconnected - Waiting for others to finish Round 1
    //==========================================================================
    if (gamePhase === 'reconnect_waiting') {
        return (
            <div className="round1-wrapper-quiz">
                <div className="reconnect-waiting-content">
                    <h1 className="reconnect-title"> WELCOME BACK! </h1>
                    <p className="reconnect-subtitle">You've reconnected to the game</p>

                    <div className="reconnect-info-box">
                        <div className="reconnect-icon">🎮</div>
                        <h2>Round 1 is in progress</h2>
                        <p>Other players are still answering questions.</p>
                        <p>Please wait for them to finish...</p>
                    </div>

                    <div className="reconnect-progress">
                        <div className="progress-text">
                            Players finished: {finishedCount}/{playerCount}
                        </div>
                        <div className="progress-bar">
                            <div
                                className="progress-fill reconnect-fill"
                                style={{ width: (playerCount > 0 ? (finishedCount / playerCount) * 100 : 0) + '%' }}
                            />
                        </div>
                    </div>

                    {score > 0 && (
                        <div className="reconnect-score">
                            Your saved score: <strong>{score} pts</strong>
                        </div>
                    )}

                    <div className="reconnect-status">
                        <div className="connecting-spinner">⏳</div>
                        <p>You'll automatically continue to Round 2 when everyone finishes!</p>
                    </div>
                </div>
            </div>
        );
    }

    //==========================================================================
    // RENDER: Summary screen (10 giây + check số người chơi)
    //==========================================================================
    if (gamePhase === 'summary') {
        // Use activePlayers (real-time) instead of static summaryData
        let playerList = [...activePlayers];

        // Update YOUR score from local state
        playerList = playerList.map(p => {
            if (p.id === playerId) {
                return { ...p, score: scoreRef.current };
            }
            return p;
        });

        // Sort by score
        playerList = playerList.sort((a, b) => b.score - a.score);

        const activePlayerCount = activePlayers.length;
        const hasEnoughPlayers = activePlayerCount >= 2;
        const hasDisconnected = disconnectedPlayers.length > 0;

        return (
            <div className="round1-wrapper-quiz">
                <div className="summary-content">
                    <div className="summary-header">
                        <h1 className="summary-title">🏆 ROUND 1 COMPLETE! 🏆</h1>
                        <div className="summary-countdown">
                            Next round in: <span className="countdown-number">{summaryCountdown}s</span>
                        </div>
                    </div>

                    {/* Hiển thị số người chơi còn kết nối */}
                    <div className="player-count-info" style={{
                        textAlign: 'center',
                        padding: '10px',
                        marginBottom: '10px',
                        background: hasEnoughPlayers ? 'rgba(0, 255, 0, 0.2)' : 'rgba(255, 0, 0, 0.2)',
                        borderRadius: '10px'
                    }}>
                        <span style={{ fontSize: '1.1rem' }}>
                            {hasEnoughPlayers
                                ? `✅ ${activePlayerCount} players connected - Ready for Round 2!`
                                : `⚠️ Only ${activePlayerCount} player(s) - Need at least 2 to continue`
                            }
                        </span>
                        {hasDisconnected && (
                            <div style={{ color: '#ff6b6b', fontSize: '0.9rem', marginTop: '5px' }}>
                                ❌ {disconnectedPlayers.length} player(s) disconnected: {disconnectedPlayers.map(p => p.name).join(', ')}
                            </div>
                        )}
                    </div>

                    <div className="summary-scoreboard">
                        <h2 className="scoreboard-title">FINAL SCOREBOARD</h2>
                        <div className="player-list">
                            {playerList.map((player, index) => {
                                const isMe = player.id === playerId;
                                const isDisconnected = disconnectedPlayers.some(p => p.id === player.id);
                                return (
                                    <div
                                        key={player.id || index}
                                        className={'player-row ' + (index === 0 ? 'winner ' : '') + (isMe ? 'is-you ' : '') + (isDisconnected ? 'disconnected' : '')}
                                        style={isDisconnected ? { opacity: 0.5, textDecoration: 'line-through' } : {}}
                                    >
                                        <div className="player-rank">
                                            {index === 0 ? '🥇' : index === 1 ? '🥈' : index === 2 ? '🥉' : '#' + (index + 1)}
                                        </div>
                                        <div className="player-name">
                                            {isMe ? (player.name + ' (YOU)') : player.name}
                                            {isDisconnected && ' ❌'}
                                        </div>
                                        <div className="player-score">{player.score} pts</div>
                                    </div>
                                );
                            })}

                            {/* Show disconnected players at the bottom */}
                            {disconnectedPlayers.filter(dp => !playerList.some(p => p.id === dp.id)).map((player, index) => (
                                <div
                                    key={'dc-' + player.id}
                                    className="player-row disconnected"
                                    style={{ opacity: 0.5, background: '#ffcccc' }}
                                >
                                    <div className="player-rank">❌</div>
                                    <div className="player-name">{player.name} (DISCONNECTED)</div>
                                    <div className="player-score">--</div>
                                </div>
                            ))}
                        </div>
                    </div>

                    <button className="skip-btn" onClick={handleSkipToNextRound}>
                        {hasEnoughPlayers ? 'CONTINUE TO ROUND 2 →' : 'END GAME'}
                    </button>
                </div>
            </div>
        );
    }

    //==========================================================================
    // RENDER: Loading question
    //==========================================================================
    if (!questionData) {
        return (
            <div className="round1-wrapper-quiz">
                <div className="quiz-content">
                    <div className="loading">Loading question {currentQuestionIdx + 1}...</div>
                </div>
            </div>
        );
    }

    //==========================================================================
    // RENDER: Question UI
    //==========================================================================
    return (
        <div className="round1-wrapper-quiz">
            <div className="quiz-content">
                <div className="question-header-row">
                    <div className="question-number-header">
                        QUESTION {currentQuestionIdx + 1} / {questionData.total_questions || 10}
                    </div>
                </div>

                <div className="question-box">
                    <h2 className="quiz-question">{questionData.question}</h2>
                </div>

                {questionData.product_image && (
                    <div className="product-image-container">
                        <img src={questionData.product_image} alt="Product" className="product-image" />
                    </div>
                )}

                <div className="answers-grid">
                    {questionData.choices && questionData.choices.map((text, index) => {
                        let btnClass = 'quiz-answer-btn';
                        if (isAnswered) {
                            if (index === correctIndex) btnClass += ' correct';
                            else if (index === selectedAnswer) btnClass += ' wrong';
                            else btnClass += ' disabled';
                        }
                        return (
                            <button
                                key={index}
                                className={btnClass}
                                onClick={() => handleAnswerClick(index)}
                                disabled={isAnswered}
                            >
                                {text}
                            </button>
                        );
                    })}
                </div>

                <div className="score-display">
                    Total Score: {score}
                </div>
            </div>
        </div>
    );
};

export default Round1;
