import React, { useState, useEffect, useRef, useCallback } from 'react';
import './Round1.css';
// Import ensures handlers are registered
import '../../../services/round1Service';
import { getQuestion, submitAnswer, endRound1, playerReady } from '../../../services/round1Service';

const Round1 = ({ matchId = 1, playerId = 1, onRoundComplete }) => {
    // Game phase: 'ready' -> 'playing' -> 'waiting' -> 'summary'
    const [gamePhase, setGamePhase] = useState('ready');
    
    // Ready screen state
    const [players, setPlayers] = useState([]);
    const [readyCount, setReadyCount] = useState(0);
    const [isReady, setIsReady] = useState(false);
    
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
    const [summaryCountdown, setSummaryCountdown] = useState(20);
    
    const startTime = useRef(null);
    const timerRef = useRef(null);
    const summaryTimerRef = useRef(null);
    const questionReceivedRef = useRef(false);
    const currentIdxRef = useRef(0);
    const scoreRef = useRef(0);

    //==========================================================================
    // HANDLER: Player clicks Ready
    //==========================================================================
    const handleReadyClick = () => {
        console.log('[Round1] Clicking ready for player ' + playerId);
        setIsReady(true);
        playerReady(matchId, playerId);
    };

    //==========================================================================
    // LISTEN: Ready status update
    //==========================================================================
    useEffect(() => {
        const handleReadyStatus = (e) => {
            const data = e.detail;
            console.log('[Round1] Ready status:', data);
            setPlayers(data.players || []);
            setReadyCount(data.ready_count || 0);
            setPlayerCount(data.player_count || 0);
        };

        window.addEventListener('round1ReadyStatus', handleReadyStatus);
        return () => window.removeEventListener('round1ReadyStatus', handleReadyStatus);
    }, []);

    //==========================================================================
    // LISTEN: All players ready - Start game
    //==========================================================================
    useEffect(() => {
        const handleAllReady = (e) => {
            const data = e.detail;
            console.log('[Round1] All ready, starting game!', data);
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
            
            questionReceivedRef.current = true;
            
            setQuestionData(data);
            setSelectedAnswer(null);
            setIsAnswered(false);
            setTimeLeft(15);
            setCorrectIndex(null);
            startTime.current = Date.now();
            
            if (timerRef.current) clearInterval(timerRef.current);
            timerRef.current = setInterval(() => {
                setTimeLeft(prev => {
                    if (prev <= 1) {
                        clearInterval(timerRef.current);
                        handleTimeout();
                        return 0;
                    }
                    return prev - 1;
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
                const nextIdx = currentIdxRef.current + 1;
                
                if (nextIdx < 10) {
                    currentIdxRef.current = nextIdx;
                    setCurrentQuestionIdx(nextIdx);
                    questionReceivedRef.current = false;
                    getQuestion(matchId, nextIdx);
                } else {
                    console.log('[Round1] All 10 questions done! Final score: ' + scoreRef.current);
                    setGamePhase('waiting');
                    endRound1(matchId, playerId);
                }
            }, 2000);
        };

        window.addEventListener('round1Result', handleResult);
        return () => window.removeEventListener('round1Result', handleResult);
    }, [matchId, playerId]);

    //==========================================================================
    // LISTEN: Waiting status
    //==========================================================================
    useEffect(() => {
        const handleWaiting = (e) => {
            const data = e.detail;
            console.log('[Round1] Waiting for others:', data);
            setFinishedCount(data.finished_count || 0);
            setPlayerCount(data.player_count || 0);
        };

        window.addEventListener('round1Waiting', handleWaiting);
        return () => window.removeEventListener('round1Waiting', handleWaiting);
    }, []);

    //==========================================================================
    // LISTEN: All finished - Show summary
    //==========================================================================
    useEffect(() => {
        const handleAllFinished = (e) => {
            const data = e.detail;
            console.log('[Round1] All finished! Final scoreboard:', data);
            
            setSummaryData(data);
            setGamePhase('summary');
            setSummaryCountdown(20);
            
            if (summaryTimerRef.current) clearInterval(summaryTimerRef.current);
            summaryTimerRef.current = setInterval(() => {
                setSummaryCountdown(prev => {
                    if (prev <= 1) {
                        clearInterval(summaryTimerRef.current);
                        if (onRoundComplete) {
                            onRoundComplete(2, { score: scoreRef.current });
                        }
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
    }, [onRoundComplete]);

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
    // RENDER: Ready screen
    //==========================================================================
    if (gamePhase === 'ready') {
        return (
            <div className="round1-wrapper-quiz">
                <div className="ready-content">
                    <h1 className="ready-title">🎮 ROUND 1 - QUIZ 🎮</h1>
                    <p className="ready-subtitle">Answer 10 questions as fast as you can!</p>
                    
                    <div className="player-status-list">
                        <h3>Players ({readyCount}/{players.length || 4} Ready)</h3>
                        {players.length > 0 ? (
                            players.map((p, idx) => (
                                <div key={idx} className={'player-status-item ' + (p.is_ready ? 'ready' : 'not-ready')}>
                                    <span className="player-status-name">
                                        {p.id === playerId ? '👤 ' + p.name + ' (YOU)' : p.name}
                                    </span>
                                    <span className="player-status-badge">
                                        {p.is_ready ? '✅ READY' : '⏳ Waiting...'}
                                    </span>
                                </div>
                            ))
                        ) : (
                            <div className="waiting-for-players">
                                Waiting for players to join...
                            </div>
                        )}
                    </div>
                    
                    {!isReady ? (
                        <button className="ready-btn" onClick={handleReadyClick}>
                            CLICK TO READY
                        </button>
                    ) : (
                        <div className="ready-waiting">
                            ✅ You are ready! Waiting for other players...
                        </div>
                    )}
                    
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
    // RENDER: Summary screen
    //==========================================================================
    if (gamePhase === 'summary') {
        let playerList = summaryData?.players || [];
        
        // Update YOUR score from local state
        playerList = playerList.map(p => {
            if (p.id === playerId) {
                return { ...p, score: scoreRef.current };
            }
            return p;
        });
        
        // Sort by score
        playerList = playerList.sort((a, b) => b.score - a.score);
        
        return (
            <div className="round1-wrapper-quiz">
                <div className="summary-content">
                    <div className="summary-header">
                        <h1 className="summary-title">🏆 ROUND 1 COMPLETE! 🏆</h1>
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
                                    <div 
                                        key={index} 
                                        className={'player-row ' + (index === 0 ? 'winner ' : '') + (isMe ? 'is-you' : '')}
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
                        </div>
                    </div>

                    <button className="skip-btn" onClick={handleSkipToNextRound}>
                        CONTINUE TO ROUND 2 →
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
                        QUESTION {currentQuestionIdx + 1} / 10
                    </div>
                    <div className="timer-badge">
                        ⏱️ {timeLeft}s
                    </div>
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
