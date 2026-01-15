import React, { useState, useEffect, useRef } from 'react';
import './Round3.css';
// Import ensures handlers are registered
import '../../../services/round3Service';
import { playerReady, requestSpin, submitDecision } from '../../../services/round3Service';
import { waitForConnection } from '../../../network/socketClient';

const Round3 = ({ matchId = 1, playerId = 1, previousScore = 0, onRoundComplete }) => {
    // Game phase: 'connecting' -> 'playing' -> 'decision' -> 'result' -> 'summary'
    const [gamePhase, setGamePhase] = useState('connecting');
    
    // Connection state
    const [connectedPlayers, setConnectedPlayers] = useState([]);
    const [connectionCountdown, setConnectionCountdown] = useState(10);
    
    // Spin state
    const [spinCount, setSpinCount] = useState(0);
    const [firstSpin, setFirstSpin] = useState(0);
    const [secondSpin, setSecondSpin] = useState(0);
    const [firstSpinValue, setFirstSpinValue] = useState(null); // Wheel value string
    const [secondSpinValue, setSecondSpinValue] = useState(null); // Wheel value string
    const [isSpinning, setIsSpinning] = useState(false);
    const [rotation, setRotation] = useState(0);
    const [decisionPending, setDecisionPending] = useState(false);
    
    // Result state
    const [finalResult, setFinalResult] = useState(null);
    const [bonus, setBonus] = useState(0);
    
    // Summary screen state
    const [summaryData, setSummaryData] = useState(null);
    const [summaryCountdown, setSummaryCountdown] = useState(5);
    const [activePlayers, setActivePlayers] = useState([]);
    
    const connectionTimerRef = useRef(null);
    const summaryTimerRef = useRef(null);
    const gameStartedRef = useRef(false);
    const scoreRef = useRef(previousScore);
    
    // Wheel values as specified
    const wheelValues = ["100", "200", "500", "0", "1000", "LOSE"];

    // Map server spin result (5-100) to wheel value index
    const mapSpinToWheelValue = (spinResult) => {
        // Map 5-100 to 0-5 index
        // 5-20 -> 0 (100)
        // 21-40 -> 1 (200)
        // 41-60 -> 2 (500)
        // 61-80 -> 3 (0)
        // 81-95 -> 4 (1000)
        // 96-100 -> 5 (LOSE)
        if (spinResult <= 20) return 0;      // 100
        if (spinResult <= 40) return 1;      // 200
        if (spinResult <= 60) return 2;      // 500
        if (spinResult <= 80) return 3;      // 0
        if (spinResult <= 95) return 4;      // 1000
        return 5;                             // LOSE
    };
    
    //==========================================================================
    // AUTO-CONNECT: When entering round 3, wait for socket and send ready
    //==========================================================================
    useEffect(() => {
        console.log('[Round3] Auto-connecting player ' + playerId + ' to match ' + matchId);
        
        let isMounted = true;
        let retryInterval = null;
        
        const connectAndReady = async () => {
            try {
                console.log('[Round3] Waiting for socket connection...');
                await waitForConnection(5000);
                console.log('[Round3] Socket connected! Sending playerReady...');
                
                if (!isMounted) return;
                
                playerReady(matchId, playerId);
                
                retryInterval = setInterval(() => {
                    if (!gameStartedRef.current && isMounted) {
                        console.log('[Round3] Retry sending playerReady...');
                        playerReady(matchId, playerId);
                    } else {
                        clearInterval(retryInterval);
                    }
                }, 1000);
            } catch (err) {
                console.error('[Round3] Failed to connect:', err);
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
    // LISTEN: Ready status update
    //==========================================================================
    useEffect(() => {
        const handleReadyStatus = (e) => {
            const data = e.detail;
            console.log('[Round3] Ready status:', data);
            setConnectedPlayers(data.players || []);
        };
        
        window.addEventListener('round3ReadyStatus', handleReadyStatus);
        return () => window.removeEventListener('round3ReadyStatus', handleReadyStatus);
    }, []);
    
    //==========================================================================
    // LISTEN: All players ready - Start game automatically
    //==========================================================================
    useEffect(() => {
        const handleAllReady = (e) => {
            const data = e.detail;
            console.log('[Round3] All players ready, starting game!', data);
            
            if (gameStartedRef.current) return;
            gameStartedRef.current = true;
            
            if (connectionTimerRef.current) clearInterval(connectionTimerRef.current);
            setGamePhase('playing');
        };
        
        window.addEventListener('round3AllReady', handleAllReady);
        return () => window.removeEventListener('round3AllReady', handleAllReady);
    }, [matchId]);
    
    //==========================================================================
    // LISTEN: Decision prompt (after first spin or second spin prompt)
    //==========================================================================
    useEffect(() => {
        const handleDecisionPrompt = (e) => {
            const data = e.detail;
            console.log('[Round3] Decision prompt:', data);
            
            if (data.spin_number === 2) {
                // Second spin prompt (after choosing continue)
                setGamePhase('playing');
                setDecisionPending(false);
            } else if (data.first_spin && (data.message?.includes('Continue') || data.message?.includes('Stop') || data.message?.includes('decision'))) {
                // Decision prompt after first spin
                setDecisionPending(true);
                setGamePhase('decision');
            } else if (data.spin_number === 1 || (data.message && data.message.includes('Spin'))) {
                // First spin prompt (round start)
                setGamePhase('playing');
            }
        };
        
        window.addEventListener('round3DecisionPrompt', handleDecisionPrompt);
        return () => window.removeEventListener('round3DecisionPrompt', handleDecisionPrompt);
    }, []);
    
    // Note: Frontend now handles spin calculation locally
    // Server spin result handler removed - wheel spins and calculates result on frontend
    
    //==========================================================================
    // LISTEN: Final result (bonus calculated)
    //==========================================================================
    useEffect(() => {
        const handleFinalResult = (e) => {
            const data = e.detail;
            console.log('[Round3] Final result:', data);
            
            setFinalResult(data);
            setBonus(data.bonus);
            scoreRef.current = data.total_score;
            setGamePhase('result');
        };
        
        window.addEventListener('round3FinalResult', handleFinalResult);
        return () => window.removeEventListener('round3FinalResult', handleFinalResult);
    }, []);
    
    //==========================================================================
    // LISTEN: All finished - Show summary
    //==========================================================================
    useEffect(() => {
        const handleAllFinished = (e) => {
            const data = e.detail;
            console.log('[Round3] 🏆 ALL_FINISHED received!', data);
            
            const allPlayers = data.players || data.rankings || [];
            setActivePlayers(allPlayers.filter(p => p.connected !== false));
            setSummaryData(data);
            setGamePhase('summary');
            setSummaryCountdown(5);
            
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
        
        window.addEventListener('round3AllFinished', handleAllFinished);
        return () => {
            window.removeEventListener('round3AllFinished', handleAllFinished);
            if (summaryTimerRef.current) clearInterval(summaryTimerRef.current);
        };
    }, []);
    
    //==========================================================================
    // Summary countdown end - game complete
    //==========================================================================
    useEffect(() => {
        if (gamePhase === 'summary' && summaryCountdown === 0) {
            if (onRoundComplete) {
                console.log('[Round3] Game complete!');
                onRoundComplete(0, { score: scoreRef.current }); // 0 = no next round
            }
        }
    }, [gamePhase, summaryCountdown, onRoundComplete]);
    
    //==========================================================================
    // Helper: Calculate which segment the pointer is pointing to after rotation
    //==========================================================================
    const getSegmentFromRotation = (finalRotation) => {
        const segmentAngle = 360 / wheelValues.length; // 60 degrees per segment
        // Normalize rotation to 0-360 range
        const normalizedRotation = finalRotation % 360;
        
        // Pointer is at 270 degrees (left side)
        // When wheel rotates clockwise, segments move clockwise
        // To find which segment is at pointer position (270deg):
        // If wheel rotated R degrees clockwise, segment at angle A is now at (A + R) mod 360
        // We want to find A such that (A + R) mod 360 = 270
        // So: A = (270 - R + 360) mod 360
        const segmentAngleAtPointer = (270 - normalizedRotation + 360) % 360;
        
        // Find which segment index this angle belongs to
        // Segment 0 (100) center is at 30deg, segment 1 (200) is at 90deg, etc.
        let segmentIndex = Math.floor(segmentAngleAtPointer / segmentAngle);
        
        // Handle edge case
        if (segmentIndex >= wheelValues.length) {
            segmentIndex = 0;
        }
        
        return segmentIndex;
    };
    
    //==========================================================================
    // Helper: Get points from wheel value
    //==========================================================================
    const getPointsFromWheelValue = (value) => {
        switch(value) {
            case "100": return 100;
            case "200": return 200;
            case "500": return 500;
            case "0": return 0;
            case "1000": return 1000;
            case "LOSE": return -1; // Special: điểm về 0
            default: return 0;
        }
    };
    
    //==========================================================================
    // HANDLER: Spin button click
    //==========================================================================
    const handleSpin = () => {
        if (isSpinning || spinCount >= 2) return;

        setIsSpinning(true);
        
        // Quay ít nhất 5 vòng (1800deg) + góc ngẫu nhiên (như code mẫu)
        const newRotation = rotation + 1800 + Math.floor(Math.random() * 360);
        setRotation(newRotation);

        // Sau 4 giây (khi animation hoàn thành), tính toán kết quả
        setTimeout(() => {
            setIsSpinning(false);
            
            // Tính segment được chỉ vào
            const segmentIndex = getSegmentFromRotation(newRotation);
            const wheelValue = wheelValues[segmentIndex];
            const points = getPointsFromWheelValue(wheelValue);
            
            console.log('[Round3] Spin complete! Segment:', segmentIndex, 'Value:', wheelValue, 'Points:', points);
            
            // Map wheel value to server result (5-100) for backend compatibility
            let serverResult = 0;
            switch(segmentIndex) {
                case 0: serverResult = 10; break;  // 100 -> 5-20 range, use 10
                case 1: serverResult = 30; break; // 200 -> 21-40 range, use 30
                case 2: serverResult = 50; break; // 500 -> 41-60 range, use 50
                case 3: serverResult = 70; break; // 0 -> 61-80 range, use 70
                case 4: serverResult = 90; break; // 1000 -> 81-95 range, use 90
                case 5: serverResult = 100; break; // LOSE -> 96-100 range, use 100
            }
            
            // Update state
            if (spinCount === 0) {
                // First spin
                setFirstSpin(serverResult);
                setFirstSpinValue(wheelValue);
                setSpinCount(1);
                
                // Show decision screen after first spin
                setDecisionPending(true);
                setGamePhase('decision');
            } else if (spinCount === 1) {
                // Second spin
                setSecondSpin(serverResult);
                setSecondSpinValue(wheelValue);
                setSpinCount(2);
                
                // Calculate final bonus - ONLY use second spin result (replaces first spin)
                const secondPoints = points;
                
                // Calculate total bonus - CHỈ LẤY ĐIỂM LẦN QUAY 2
                let totalBonus = 0;
                if (secondPoints === -1) {
                    // LOSE in second spin: điểm về 0
                    totalBonus = -scoreRef.current;
                    scoreRef.current = 0;
                } else {
                    totalBonus = secondPoints;  // Chỉ lấy điểm lần 2, không cộng lần 1
                    scoreRef.current += totalBonus;
                }
                
                setBonus(totalBonus);
                
                // Dispatch score update
                window.dispatchEvent(new CustomEvent('round3ScoreUpdate', {
                    detail: { score: totalBonus, totalScore: scoreRef.current }
                }));
                
                // Create final result object
                const result = {
                    first_spin: firstSpin,
                    second_spin: serverResult,
                    bonus: totalBonus,
                    total_score: scoreRef.current,
                    first_spin_value: firstSpinValue,
                    second_spin_value: wheelValue
                };
                setFinalResult(result);
                
                // Show result screen first
                setGamePhase('result');
                
                // After 3 seconds, show summary (simulate waiting for other players)
                setTimeout(() => {
                    // Create summary data
                    const summary = {
                        players: [
                            { id: playerId, name: `Player${playerId}`, score: scoreRef.current, eliminated: false, connected: true }
                        ],
                        next_round: 0,
                        status_message: "Round 3 completed"
                    };
                    setActivePlayers(summary.players);
                    setSummaryData(summary);
                    setGamePhase('summary');
                    setSummaryCountdown(5);
                }, 3000);
            }
            
            // TODO: Send result to server if needed
            // For now, backend will handle the scoring logic
        }, 4000); // 4 seconds to match CSS transition
    };
    
    //==========================================================================
    // HANDLER: Decision (Continue or Stop)
    //==========================================================================
    const handleDecision = (continueSpin) => {
        setDecisionPending(false);
        
        if (continueSpin) {
            // Continue: Allow second spin
            setGamePhase('playing');
            // Don't send to server yet - wait for second spin
        } else {
            // Stop: Calculate bonus from first spin only
            const points = getPointsFromWheelValue(firstSpinValue);
            
            console.log('[Round3] STOP decision - First spin value:', firstSpinValue, 'Points:', points);
            
            // Calculate bonus
            const calculatedBonus = points === -1 ? -scoreRef.current : points; // LOSE = reset to 0
            
            // Update score
            if (points === -1) {
                scoreRef.current = 0; // LOSE: điểm về 0
            } else {
                scoreRef.current += calculatedBonus;
            }
            
            setBonus(calculatedBonus);
            
            // Dispatch score update
            window.dispatchEvent(new CustomEvent('round3ScoreUpdate', {
                detail: { score: calculatedBonus, totalScore: scoreRef.current }
            }));
            
            // Create final result object
            const result = {
                first_spin: firstSpin,
                second_spin: 0,
                bonus: calculatedBonus,
                total_score: scoreRef.current,
                first_spin_value: firstSpinValue,
                second_spin_value: null
            };
            setFinalResult(result);
            
            // Show result screen first
            setGamePhase('result');
            
            // After 3 seconds, show summary (simulate waiting for other players)
            setTimeout(() => {
                // Create summary data
                const summary = {
                    players: [
                        { id: playerId, name: `Player${playerId}`, score: scoreRef.current, eliminated: false, connected: true }
                    ],
                    next_round: 0,
                    status_message: "Round 3 completed"
                };
                setActivePlayers(summary.players);
                setSummaryData(summary);
                setGamePhase('summary');
                setSummaryCountdown(5);
            }, 3000);
        }
    };
    
    //==========================================================================
    // Helper: Get wheel value display from server result
    //==========================================================================
    const getWheelValueDisplay = (serverResult) => {
        if (!serverResult || serverResult === 0) return "0";
        const wheelIndex = mapSpinToWheelValue(serverResult);
        return wheelValues[wheelIndex];
    };
    
    //==========================================================================
    // RENDER: Connecting screen
    //==========================================================================
    if (gamePhase === 'connecting') {
        return (
            <div className="round3-wrapper">
                <div className="connecting-content">
                    <h1 className="connecting-title">ROUND 3: BONUS WHEEL</h1>
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
    // RENDER: Decision screen (after first spin)
    //==========================================================================
    if (gamePhase === 'decision') {
        return (
            <div className="round3-wrapper">
                <div className="quiz-content">
                    <div className="question-header-row">
                        <div className="turn-indicator">SPIN 1 RESULT: {firstSpinValue}</div>
                    </div>
                    
                    <div className="decision-content">
                        <h2 className="decision-title">Make Your Decision</h2>
                        <p className="decision-subtitle">You spun: <strong>{firstSpinValue}</strong></p>
                        <p className="decision-question">Do you want to spin again?</p>
                        
                        <div className="decision-buttons">
                            <button 
                                className="decision-btn continue-btn"
                                onClick={() => handleDecision(true)}
                            >
                                CONTINUE (Spin Again)
                            </button>
                            <button 
                                className="decision-btn stop-btn"
                                onClick={() => handleDecision(false)}
                            >
                                STOP (Calculate Bonus)
                            </button>
                        </div>
                    </div>
                </div>
            </div>
        );
    }
    
    //==========================================================================
    // RENDER: Waiting screen (after decision, waiting for result)
    //==========================================================================
    if (gamePhase === 'waiting') {
        return (
            <div className="round3-wrapper">
                <div className="connecting-content">
                    <h1 className="connecting-title">CALCULATING BONUS...</h1>
                    <div className="connecting-status">
                        <div className="connecting-spinner"></div>
                        <p>Please wait...</p>
                    </div>
                </div>
            </div>
        );
    }
    
    //==========================================================================
    // RENDER: Result screen (after bonus calculated)
    //==========================================================================
    if (gamePhase === 'result' && finalResult) {
        return (
            <div className="round3-wrapper">
                <div className="quiz-content">
                    <div className="question-header-row">
                        <div className="turn-indicator">BONUS CALCULATED</div>
                    </div>
                    
                    <div className="result-content">
                        <h2 className="result-title">Your Bonus</h2>
                        <div className="spin-results">
                            <div className="spin-result-item">
                                <span>First Spin:</span>
                                <strong>{finalResult.first_spin_value || getWheelValueDisplay(finalResult.first_spin)}</strong>
                            </div>
                            {finalResult.second_spin_value && (
                                <div className="spin-result-item">
                                    <span>Second Spin:</span>
                                    <strong>{finalResult.second_spin_value}</strong>
                                </div>
                            )}
                            <div className="bonus-display">
                                <span>Bonus:</span>
                                <strong className="bonus-value">+{finalResult.bonus}</strong>
                            </div>
                            <div className="total-score">
                                <span>Total Score:</span>
                                <strong>{finalResult.total_score}</strong>
                            </div>
                        </div>
                        <p className="result-waiting">Waiting for other players...</p>
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
            <div className="round3-wrapper">
                <div className="summary-content">
                    <div className="summary-header">
                        <h1 className="summary-title">🏆 GAME COMPLETE! 🏆</h1>
                        <div className="summary-countdown">
                            Returning to lobby in: <span className="countdown-number">{summaryCountdown}s</span>
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
                </div>
            </div>
        );
    }
    
    //==========================================================================
    // RENDER: Playing (spin wheel)
    //==========================================================================
    return (
        <div className="round3-wrapper">
            <div className="quiz-content">
                <div className="question-header-row">
                    <div className="turn-indicator">SPIN {spinCount + 1} / 2</div>
                    <div className="bonus-score-badge">💰 BONUS: {bonus}</div>
                </div>

                <div className="wheel-section">
                    <div className="wheel-container">
                        <div className="wheel-pointer"></div>
                        <div 
                            className={`lucky-wheel ${isSpinning ? 'spinning' : ''}`}
                            style={{ transform: `rotate(${rotation}deg)` }}
                        >
                            {wheelValues.map((value, index) => {
                                const segmentAngle = 360 / wheelValues.length;
                                const angle = index * segmentAngle + segmentAngle / 2;
                                
                                return (
                                    <div 
                                        key={index} 
                                        className="wheel-segment" 
                                        style={{ '--angle': `${angle}deg` }}
                                    >
                                        <span>{value}</span>
                                    </div>
                                );
                            })}
                        </div>
                    </div>

                    <button 
                        className={`spin-btn ${isSpinning ? 'disabled' : ''}`} 
                        onClick={handleSpin}
                        disabled={isSpinning || spinCount >= 2}
                    >
                        {isSpinning ? 'SPINNING...' : spinCount === 0 ? '1ST SPIN' : '2ND SPIN'}
                    </button>
                    
                    {firstSpinValue && (
                        <div className="spin-history">
                            <p>First Spin: <strong>{firstSpinValue}</strong></p>
                            {secondSpinValue && <p>Second Spin: <strong>{secondSpinValue}</strong></p>}
                        </div>
                    )}
                </div>
            </div>
        </div>
    );
};

export default Round3;
