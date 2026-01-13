import React, { useState, useEffect, useRef, useCallback } from 'react';
import './Round1.css';
// Import ensures handlers are registered
import '../../../services/round1Service';
import { submitAnswer, endRound1, playerReady } from '../../../services/round1Service';
import { waitForConnection } from '../../../network/socketClient';

// Note: getQuestion is no longer used - server pushes questions in synchronized mode

const Round1 = ({ matchId = 1, playerId = 1, onRoundComplete }) => {
    // Game phase: 'connecting' -> 'playing' -> 'waiting' -> 'summary'
    const [gamePhase, setGamePhase] = useState('connecting');

    // Connection state
    const [connectedPlayers, setConnectedPlayers] = useState([]);
    const [connectionCountdown, setConnectionCountdown] = useState(5);

    // Question state
    const [currentQuestionIdx, setCurrentQuestionIdx] = useState(0);
    const [questionData, setQuestionData] = useState(null);
    const [selectedAnswer, setSelectedAnswer] = useState(null);
    const [timeLeft, setTimeLeft] = useState(15);
    const [score, setScore] = useState(0);
    const [isAnswered, setIsAnswered] = useState(false);
    const [correctIndex, setCorrectIndex] = useState(null);

    // Waiting state
    const [finishedCount, setFinishedCount] = useState(0);
    const [playerCount, setPlayerCount] = useState(0);

    // Summary screen state
    const [summaryData, setSummaryData] = useState(null);
    const [summaryCountdown, setSummaryCountdown] = useState(10);
    const [activePlayers, setActivePlayers] = useState([]); // Track connected players
    const [disconnectedPlayers, setDisconnectedPlayers] = useState([]); // Track who disconnected

    const startTime = useRef(null);
    const timerRef = useRef(null);
    const summaryTimerRef = useRef(null);
    const connectionTimerRef = useRef(null);
    const questionReceivedRef = useRef(false);
    const currentIdxRef = useRef(0);
    const scoreRef = useRef(0);
    const gameStartedRef = useRef(false);

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

        // Countdown để hiển thị trạng thái kết nối (tăng lên 10s)
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
    // SYNCHRONIZED FLOW: Server broadcasts first question after this
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

            // Reset question tracking
            questionReceivedRef.current = false;
            currentIdxRef.current = 0;
            setCurrentQuestionIdx(0);
            
            // SYNCHRONIZED FLOW:
            // Server will broadcast first question automatically after ALL_READY
            // If first_question is embedded in response, round1Service dispatches it
            // Otherwise, wait for server to push S2C_ROUND1_QUESTION
            console.log('[Round1] Waiting for server to broadcast first question...');
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
    // LISTEN: Question received (SERVER PUSHES QUESTIONS - SYNCHRONIZED)
    // Server broadcasts question to all players at the same time
    //==========================================================================
    useEffect(() => {
        const handleQuestion = (e) => {
            const data = e.detail;
            console.log('[Round1] Question received:', data);

            // Check for error (e.g., player eliminated)
            if (data.success === false) {
                console.error('[Round1] Failed to get question:', data.error);
                
                // Handle eliminated player
                if (data.error && data.error.includes('eliminated')) {
                    console.log('[Round1] Player is eliminated, showing waiting screen');
                    setGamePhase('eliminated');
                }
                return;
            }

            // Make sure we're in playing phase
            if (gamePhase !== 'playing') {
                setGamePhase('playing');
            }

            questionReceivedRef.current = true;

            // Update question index from server (authoritative)
            const serverQIdx = data.question_idx;
            if (serverQIdx !== undefined) {
                currentIdxRef.current = serverQIdx;
                setCurrentQuestionIdx(serverQIdx);
            }

            setQuestionData(data);
            setSelectedAnswer(null);
            setIsAnswered(false);
            setCorrectIndex(null);
            startTime.current = Date.now();

            // Get time limit from server or default to 15s
            const timeLimit = Math.floor((data.time_limit_ms || 15000) / 1000);
            setTimeLeft(timeLimit);

            // Dispatch initial timer
            window.dispatchEvent(new CustomEvent('timerUpdate', { detail: { timeLeft: timeLimit } }));

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
    }, [handleTimeout, gamePhase]);

    //==========================================================================
    // LISTEN: Answer result - SYNCHRONIZED FLOW
    // Server broadcasts next question when all players answered
    // Client just waits for server to push the next question
    //==========================================================================
    useEffect(() => {
        const handleResult = (e) => {
            const data = e.detail;
            console.log('[Round1] Answer result:', data);

            clearInterval(timerRef.current);
            setIsAnswered(true);
            setCorrectIndex(data.correct_index);

            // Add score (normalized from score_delta)
            const addedScore = data.score || 0;
            // Use totalScore from server (authoritative) or calculate locally
            const newTotal = data.totalScore || (scoreRef.current + addedScore);
            scoreRef.current = newTotal;
            setScore(newTotal);

            console.log('[Round1] Score: +' + addedScore + ', Total: ' + newTotal);

            // Dispatch score update
            window.dispatchEvent(new CustomEvent('round1ScoreUpdate', {
                detail: { score: addedScore, totalScore: newTotal }
            }));

            // Show waiting status while waiting for all players
            const answered = data.answered_count || 0;
            const total = data.player_count || 0;
            console.log('[Round1] Waiting for others: ' + answered + '/' + total);
            
            // SYNCHRONIZED FLOW: 
            // - Server will broadcast next question when all_answered is true
            // - Client doesn't request next question, it waits for server push
            // - If has_next is false and round is finished, wait for ALL_FINISHED
            
            if (data.all_answered) {
                console.log('[Round1] All answered! Server will broadcast next question...');
                currentIdxRef.current++;
                setCurrentQuestionIdx(currentIdxRef.current);
                // Server will push next question or round end result
            }
            
            // Check if round finished (no more questions)
            if (data.has_next === false) {
                console.log('[Round1] Round finished, waiting for final results...');
                setGamePhase('waiting');
            }
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
    // Handles both connected/disconnected and eliminated status
    //==========================================================================
    useEffect(() => {
        const handleAllFinished = (e) => {
            const data = e.detail;
            console.log('[Round1] 🏆🏆🏆 ALL_FINISHED received! Current phase:', gamePhase);
            console.log('[Round1] Scoreboard data:', data);

            // Clear any running timers
            if (timerRef.current) clearInterval(timerRef.current);

            setSummaryData(data);

            // Separate connected and disconnected players from server response
            const allPlayers = data.players || [];
            
            // Filter: connected AND not eliminated = active
            const connected = allPlayers.filter(p => p.connected !== false && !p.eliminated);
            const disconnected = allPlayers.filter(p => p.connected === false);
            const eliminated = allPlayers.filter(p => p.eliminated === true);

            setActivePlayers(connected);
            setDisconnectedPlayers(disconnected);
            setGamePhase('summary');
            setSummaryCountdown(10);

            console.log('[Round1] Active:', connected.length, 
                       'Disconnected:', disconnected.length, 
                       'Eliminated:', eliminated.length);

            // Log elimination info
            if (eliminated.length > 0) {
                console.log('[Round1] 🚫 Eliminated players:', eliminated.map(p => `${p.name}(${p.id})`));
            }

            // Check if current player is eliminated
            const meEliminated = eliminated.some(p => Number(p.id) === Number(playerId));
            if (meEliminated) {
                console.log('[Round1] YOU have been eliminated!');
            }

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
    }, [playerId]);

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
                onRoundComplete(2, {
                    score: scoreRef.current,
                    playerCount: activeCount,
                    disconnectedPlayers: disconnectedPlayers
                });
            }
        }
    }, [gamePhase, summaryCountdown, activePlayers, disconnectedPlayers, onRoundComplete]);

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
            onRoundComplete(2, { score: scoreRef.current });
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
                        <h3>Players Connected ({connectedPlayers.filter(p => p.ready).length}/{connectedPlayers.length || playerCount || 3})</h3>
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
                                Waiting for players to connect...
                            </div>
                        )}
                    </div>

                    <div className="connecting-status">
                        <div className="connecting-spinner">🔄</div>
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

                    <div className="waiting-spinner">🔄</div>
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
                    <h1 className="reconnect-title">🔄 WELCOME BACK! 🔄</h1>
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
    // RENDER: Summary screen (10 giây + check số người chơi + eliminated)
    //==========================================================================
    if (gamePhase === 'summary') {
        // Get all players from summaryData (server data is authoritative)
        const allPlayers = summaryData?.players || [];
        
        // Separate by status
        const eliminatedPlayers = allPlayers.filter(p => p.eliminated === true);
        const activeAndConnected = allPlayers.filter(p => !p.eliminated && p.connected !== false);
        
        // Merge with local tracking for disconnected
        let playerList = [...activeAndConnected];

        // Update YOUR score from local state
        playerList = playerList.map(p => {
            if (Number(p.id) === Number(playerId)) {
                return { ...p, score: scoreRef.current };
            }
            return p;
        });

        // Sort by score descending
        playerList = playerList.sort((a, b) => b.score - a.score);

        const activePlayerCount = activeAndConnected.length;
        const hasEnoughPlayers = activePlayerCount >= 2;
        const hasDisconnected = disconnectedPlayers.length > 0;
        const hasEliminated = eliminatedPlayers.length > 0;
        
        // Check if current player is eliminated
        const amIEliminated = eliminatedPlayers.some(p => Number(p.id) === Number(playerId));

        return (
            <div className="round1-wrapper-quiz">
                <div className="summary-content">
                    <div className="summary-header">
                        <h1 className="summary-title">🏆 ROUND 1 COMPLETE! 🏆</h1>
                        <div className="summary-countdown">
                            Next round in: <span className="countdown-number">{summaryCountdown}s</span>
                        </div>
                    </div>

                    {/* Show if player is eliminated */}
                    {amIEliminated && (
                        <div className="eliminated-banner" style={{
                            textAlign: 'center',
                            padding: '15px',
                            marginBottom: '10px',
                            background: 'rgba(255, 0, 0, 0.3)',
                            borderRadius: '10px',
                            border: '2px solid #ff4444'
                        }}>
                            <span style={{ fontSize: '1.3rem', fontWeight: 'bold', color: '#ff4444' }}>
                                ❌ YOU HAVE BEEN ELIMINATED ❌
                            </span>
                            <p style={{ margin: '5px 0 0', color: '#ffaaaa' }}>
                                You had the lowest score this round.
                            </p>
                        </div>
                    )}

                    {/* Show player count and status */}
                    <div className="player-count-info" style={{
                        textAlign: 'center',
                        padding: '10px',
                        marginBottom: '10px',
                        background: hasEnoughPlayers ? 'rgba(0, 255, 0, 0.2)' : 'rgba(255, 0, 0, 0.2)',
                        borderRadius: '10px'
                    }}>
                        <span style={{ fontSize: '1.1rem' }}>
                            {hasEnoughPlayers
                                ? `✅ ${activePlayerCount} players remaining - Ready for Round 2!`
                                : `⚠️ Only ${activePlayerCount} player(s) - Need at least 2 to continue`
                            }
                        </span>
                        {hasDisconnected && (
                            <div style={{ color: '#ff6b6b', fontSize: '0.9rem', marginTop: '5px' }}>
                                🔌 {disconnectedPlayers.length} player(s) disconnected
                            </div>
                        )}
                        {hasEliminated && (
                            <div style={{ color: '#ff9999', fontSize: '0.9rem', marginTop: '5px' }}>
                                🚫 {eliminatedPlayers.length} player(s) eliminated: {eliminatedPlayers.map(p => p.name).join(', ')}
                            </div>
                        )}
                    </div>

                    <div className="summary-scoreboard">
                        <h2 className="scoreboard-title">FINAL SCOREBOARD</h2>
                        <div className="player-list">
                            {/* Active players */}
                            {playerList.map((player, index) => {
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

                            {/* Eliminated players */}
                            {eliminatedPlayers.map((player) => {
                                const isMe = Number(player.id) === Number(playerId);
                                return (
                                    <div
                                        key={'elim-' + player.id}
                                        className="player-row eliminated"
                                        style={{ 
                                            opacity: 0.6, 
                                            background: 'rgba(255, 100, 100, 0.3)',
                                            borderLeft: '4px solid #ff4444'
                                        }}
                                    >
                                        <div className="player-rank">🚫</div>
                                        <div className="player-name">
                                            {isMe ? (player.name + ' (YOU)') : player.name}
                                            <span style={{ color: '#ff6666', marginLeft: '8px' }}>ELIMINATED</span>
                                        </div>
                                        <div className="player-score">{player.score} pts</div>
                                    </div>
                                );
                            })}

                            {/* Disconnected players */}
                            {disconnectedPlayers.filter(dp => 
                                !playerList.some(p => Number(p.id) === Number(dp.id)) &&
                                !eliminatedPlayers.some(p => Number(p.id) === Number(dp.id))
                            ).map((player) => (
                                <div
                                    key={'dc-' + player.id}
                                    className="player-row disconnected"
                                    style={{ opacity: 0.5, background: '#ffcccc' }}
                                >
                                    <div className="player-rank">🔌</div>
                                    <div className="player-name">{player.name} (DISCONNECTED)</div>
                                    <div className="player-score">--</div>
                                </div>
                            ))}
                        </div>
                    </div>

                    <button className="skip-btn" onClick={handleSkipToNextRound}>
                        {amIEliminated 
                            ? 'SPECTATE NEXT ROUND →' 
                            : hasEnoughPlayers 
                                ? 'CONTINUE TO ROUND 2 →' 
                                : 'END GAME'
                        }
                    </button>
                </div>
            </div>
        );
    }

    //==========================================================================
    // RENDER: Eliminated player screen
    //==========================================================================
    if (gamePhase === 'eliminated') {
        return (
            <div className="round1-wrapper-quiz">
                <div className="eliminated-content">
                    <h1 className="eliminated-title">❌ ELIMINATED ❌</h1>
                    <p className="eliminated-subtitle">You have been eliminated from the game</p>
                    
                    <div className="eliminated-info-box">
                        <div className="eliminated-icon">😢</div>
                        <h2>Better luck next time!</h2>
                        <p>Your final score: <strong>{score} pts</strong></p>
                    </div>

                    <div className="eliminated-status">
                        <p>Waiting for the game to finish...</p>
                        <div className="connecting-spinner">⏳</div>
                    </div>
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
                        QUESTION {currentQuestionIdx + 1} / 10
                    </div>
                    {/* Timer đã có ở GameContainer header, không cần ở đây */}
                </div>

                <h2 className="quiz-question">{questionData.question}</h2>

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
