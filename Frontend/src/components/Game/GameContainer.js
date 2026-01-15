// import React, { useState } from 'react';
// import './GameContainer.css';
// import Round1 from './Round/Round1'; 
// import Round2 from './Round/Round2'; 
// import Round3 from './Round/Round3';
// import RoundBonus from './Round/RoundBonus';

// const GameContainer = () => {
//     const [score, setScore] = useState(1250);
//     const [pointsGained, setPointsGained] = useState(150);

//     const mockDataFromBackend = {
//         questionText: "What is the price of iPhone 15?",
//         productImage: "/bg/iphone15.jpg",
//         currentQuestion: 1,
//         totalQuestions: 15
//     };

//     const [gameMode, setGameMode] = useState('SURVIVAL'); 

//     const leaderboardData = [
//         { name: "ALEX_MASTER", score: 2100, isEliminated: true },
//         { name: "PLAYER_2", score: 1850, isEliminated: true }, 
//         { name: "YOU", score: 1250, isEliminated: false },
//         { name: "CHRIS_99", score: 950, isEliminated: false },
//         { name: "CHRIS_99", score: 950, isEliminated: false },
//         { name: "CHRIS_99", score: 950, isEliminated: false }
//     ];

//     const containerStyle = {
//         width: '100vw',
//         height: '100vh',
//         backgroundImage: `url('/bg/waitingroom2.png')`,
//         backgroundSize: 'cover',
//         backgroundPosition: 'center',
//         display: 'flex',
//         flexDirection: 'column',
//         fontFamily: "'Luckiest Guy', cursive, sans-serif",
//         overflow: 'hidden' 
//     };

//     return (
//         <div style={containerStyle}>
//             <div className="game-overlay">

//                 <header className="game-header">
//                     <div className="header-left">
//                         <div className="round-badge-new">ROUND 1</div>
//                     </div>

//                     {/* Header-right giờ sẽ chứa cả Timer và Score */}
//                     <div className="header-right" style={{ display: 'flex', alignItems: 'center', gap: '20px' }}>
//                         <div className="timer-circle">
//                             <div className="timer-value">30</div>
//                         </div>

//                         <div className="score-board">
//                             <span className="score-label">YOUR SCORE</span>
//                             <span className="score-number">{score.toLocaleString()}</span>
//                             {pointsGained > 0 && <div className="score-plus">+{pointsGained}</div>}
//                         </div>
//                     </div>
//                 </header>

//                 <div className="game-body">
//                     <aside className="game-leaderboard">
//                         <h3 className="panel-title">RANKING</h3>
//                         <div className="leaderboard-list">
//                             {leaderboardData.map((player, index) => {
//                                 const isOut = gameMode === 'SURVIVAL' && player.isEliminated;

//                                 return (
//                                     <div 
//                                         key={index} 
//                                         className={`rank-item 
//                                             ${player.name === 'YOU' ? 'is-me' : ''} 
//                                             ${isOut ? 'eliminated' : ''}`
//                                         }
//                                     >
//                                         <span className="rank-name">{player.name}</span>
//                                         <span className="rank-score">
//                                             {isOut ? "OUT" : player.score.toLocaleString()}
//                                         </span>
//                                     </div>
//                                 );
//                             })}
//                         </div>

//                         {/* Nút FORFEIT nằm dưới bảng Ranking */}
//                         <button className="game-btn-exit forfeit-sidebar-btn" onClick={() => window.history.back()}>
//                             FORFEIT
//                         </button>
//                     </aside>

//                     <main className="game-play-panel">
//                        {/* <Round1 
//                             currentQuestion={mockDataFromBackend.currentQuestion}
//                             totalQuestions={mockDataFromBackend.totalQuestions}
//                             productImage={mockDataFromBackend.productImage} // Thêm dòng này
//                         /> */}
//                         {/* <Round2
//                             currentQuestion={mockDataFromBackend.currentQuestion}
//                             totalQuestions={mockDataFromBackend.totalQuestions}
//                             productImage={mockDataFromBackend.productImage}
//                             isWagerAvailable={true}
//                         /> */}
//                          {/* <Round3 totalPlayers={1} /> */}
//                          <RoundBonus 
//                             currentQuestion={1} 
//                             totalQuestions={1} 
//                             productImage={mockDataFromBackend.productImage} 
//                         />
//                     </main>
//                 </div>
//             </div>
//         </div>
//     );
// };

// // export default GameContainer;
// import React, { useState, useEffect } from 'react';
// import './GameContainer.css';
// import Round1 from './Round/Round1'; 
// import Round2 from './Round/Round2'; 
// import Round3 from './Round/Round3';
// import RoundBonus from './Round/RoundBonus';

// const GameContainer = () => {
//     // ============ STATE MANAGEMENT ============
//     const [currentRound, setCurrentRound] = useState(1); // 1, 2, 3, or 'bonus'
//     const [score, setScore] = useState(0);
//     const [pointsGained, setPointsGained] = useState(0);
//     const [gameMode, setGameMode] = useState('SCORING'); // 'SCORING' or 'SURVIVAL'

//     // ============ GET MATCH ID ============
//     const urlParams = new URLSearchParams(window.location.search);
//     const matchId = urlParams.get('match_id') || 1; // Default test match

//     // ============ MOCK DATA ============
//     const mockDataFromBackend = {
//         questionText: "What is the price of iPhone 15?",
//         productImage: "/bg/iphone15.jpg",
//         currentQuestion: 1,
//         totalQuestions: 15
//     };

//     // ============ LEADERBOARD DATA ============
//     const [leaderboardData, setLeaderboardData] = useState([
//         { name: "PLAYER1", score: 0, isEliminated: false },
//         { name: "PLAYER2", score: 0, isEliminated: false },
//         { name: "PLAYER3", score: 0, isEliminated: false },
//         { name: "PLAYER4", score: 0, isEliminated: false }
//     ]);

//     // ============ LISTEN TO SCORE UPDATES FROM ROUND1 ============
//     useEffect(() => {
//         const handleScoreUpdate = (e) => {
//             const newScore = e.detail.score;
//             const gained = e.detail.gained || 0;

//             setScore(newScore);
//             setPointsGained(gained);

//             // Update leaderboard for PLAYER1 (YOU)
//             setLeaderboardData(prev => prev.map(player => 
//                 player.name === 'PLAYER1' 
//                     ? { ...player, score: newScore }
//                     : player
//             ));

//             // Clear gained points animation after 2s
//             if (gained > 0) {
//                 setTimeout(() => setPointsGained(0), 2000);
//             }
//         };

//         window.addEventListener('scoreUpdate', handleScoreUpdate);
//         return () => window.removeEventListener('scoreUpdate', handleScoreUpdate);
//     }, []);

//     // ============ HANDLE ROUND COMPLETE ============
//     const handleRoundComplete = (nextRound, data) => {
//         console.log(`[GameContainer] Round ${currentRound} complete, moving to round ${nextRound}`);
//         console.log('[GameContainer] Data:', data);

//         // Update score if provided
//         if (data.round1Score !== undefined) {
//             setScore(data.round1Score);
//         }

//         // Move to next round
//         setCurrentRound(nextRound);
//     };

//     // ============ RENDER CURRENT ROUND ============
//     const renderRound = () => {
//         switch (currentRound) {
//             case 1:
//                 return (
//                     <Round1 
//                         matchId={matchId}
//                         onRoundComplete={handleRoundComplete}
//                     />
//                 );

//             case 2:
//                 return (
//                     <Round2
//                         currentQuestion={mockDataFromBackend.currentQuestion}
//                         totalQuestions={mockDataFromBackend.totalQuestions}
//                         productImage={mockDataFromBackend.productImage}
//                         isWagerAvailable={true}
//                         previousScore={score}
//                         onRoundComplete={handleRoundComplete}
//                     />
//                 );

//             case 3:
//                 return (
//                     <Round3 
//                         totalPlayers={leaderboardData.length}
//                         previousScore={score}
//                         onRoundComplete={handleRoundComplete}
//                     />
//                 );

//             case 'bonus':
//                 return (
//                     <RoundBonus 
//                         currentQuestion={1} 
//                         totalQuestions={1} 
//                         productImage={mockDataFromBackend.productImage}
//                         previousScore={score}
//                     />
//                 );

//             default:
//                 return <Round1 matchId={matchId} onRoundComplete={handleRoundComplete} />;
//         }
//     };

//     // ============ STYLES ============
//     const containerStyle = {
//         width: '100vw',
//         height: '100vh',
//         backgroundImage: `url('/bg/waitingroom2.png')`,
//         backgroundSize: 'cover',
//         backgroundPosition: 'center',
//         display: 'flex',
//         flexDirection: 'column',
//         fontFamily: "'Luckiest Guy', cursive, sans-serif",
//         overflow: 'hidden' 
//     };

//     // ============ RENDER ============
//     return (
//         <div style={containerStyle}>
//             <div className="game-overlay">

//                 <header className="game-header">
//                     <div className="header-left">
//                         <div className="round-badge-new">
//                             {currentRound === 'bonus' ? 'BONUS ROUND' : `ROUND ${currentRound}`}
//                         </div>
//                     </div>

//                     <div className="header-right" style={{ display: 'flex', alignItems: 'center', gap: '20px' }}>
//                         <div className="timer-circle">
//                             <div className="timer-value">15</div>
//                         </div>

//                         <div className="score-board">
//                             <span className="score-label">YOUR SCORE</span>
//                             <span className="score-number">{score.toLocaleString()}</span>
//                             {pointsGained > 0 && (
//                                 <div className="score-plus">+{pointsGained}</div>
//                             )}
//                         </div>
//                     </div>
//                 </header>

//                 <div className="game-body">
//                     <aside className="game-leaderboard">
//                         <h3 className="panel-title">RANKING</h3>
//                         <div className="leaderboard-list">
//                             {leaderboardData.map((player, index) => {
//                                 const isOut = gameMode === 'SURVIVAL' && player.isEliminated;
//                                 const isYou = player.name === 'PLAYER1';

//                                 return (
//                                     <div 
//                                         key={index} 
//                                         className={`rank-item 
//                                             ${isYou ? 'is-me' : ''} 
//                                             ${isOut ? 'eliminated' : ''}`
//                                         }
//                                     >
//                                         <span className="rank-name">
//                                             {isYou ? 'YOU' : player.name}
//                                         </span>
//                                         <span className="rank-score">
//                                             {isOut ? "OUT" : player.score.toLocaleString()}
//                                         </span>
//                                     </div>
//                                 );
//                             })}
//                         </div>

//                         <button 
//                             className="game-btn-exit forfeit-sidebar-btn" 
//                             onClick={() => window.history.back()}
//                         >
//                             FORFEIT
//                         </button>
//                     </aside>

//                     <main className="game-play-panel">
//                         {renderRound()}
//                     </main>
//                 </div>
//             </div>

//             {/* Debug info - Remove in production */}
//             <div style={{
//                 position: 'fixed',
//                 bottom: '10px',
//                 right: '10px',
//                 background: 'rgba(0,0,0,0.7)',
//                 color: 'white',
//                 padding: '10px',
//                 borderRadius: '5px',
//                 fontSize: '12px',
//                 zIndex: 9999
//             }}>
//                 <div>Match ID: {matchId}</div>
//                 <div>Round: {currentRound}</div>
//                 <div>Score: {score}</div>
//             </div>
//         </div>
//     );
// };

// export default GameContainer;

//VER3:

import React, { useState, useEffect } from 'react';
import './GameContainer.css';
import Round1 from './Round/Round1';
import Round2 from './Round/Round2';
import Round3 from './Round/Round3';
import RoundBonus from './Round/RoundBonus';
import EndGame from './Round/EndGame';
import { sendForfeit } from '../../services/forfeitService';
import { useNavigate } from 'react-router-dom';
// Import services to ensure handlers are registered
import '../../services/round1Service';
import '../../services/round2Service';
import '../../services/round3Service';
import '../../services/bonusService';
import '../../services/endGameService';

const GameContainer = () => {
    // ============ STATE MANAGEMENT ============
    const [currentRound, setCurrentRound] = useState(1);
    const [score, setScore] = useState(0);
    const [pointsGained, setPointsGained] = useState(0);
    const [gameMode, setGameMode] = useState('SCORING');
    const [timeLeft, setTimeLeft] = useState(15);


    // ============ GET MATCH ID & PLAYER ID FROM URL ============
    const urlParams = new URLSearchParams(window.location.search);
    const matchId = parseInt(urlParams.get('match_id'));
    const playerId = parseInt(urlParams.get('player_id'));
    const playerName = urlParams.get('name') || `PLAYER${playerId || '?'}`;

    // Validate match_id is required
    const navigate = useNavigate();
    useEffect(() => {
        if (!matchId || isNaN(matchId)) {
            console.error('[GameContainer] Invalid or missing match_id, redirecting to lobby');
            navigate('/lobby');
        }
    }, [matchId, navigate]);

    // ============ LEADERBOARD DATA ============
    const [leaderboardData, setLeaderboardData] = useState([
        { id: 1, name: "PLAYER1", score: 0, isEliminated: false },
        { id: 2, name: "PLAYER2", score: 0, isEliminated: false },
        { id: 3, name: "PLAYER3", score: 0, isEliminated: false },
        { id: 4, name: "PLAYER4", score: 0, isEliminated: false }
    ]);

    // ============ LISTEN TO TIMER UPDATES ============
    useEffect(() => {
        const handleTimerUpdate = (e) => {
            setTimeLeft(e.detail.timeLeft);
        };
        window.addEventListener('timerUpdate', handleTimerUpdate);
        return () => window.removeEventListener('timerUpdate', handleTimerUpdate);
    }, []);

    // ============ LISTEN TO SCORE UPDATES (Round 1 & Round 2) ============
    useEffect(() => {
        let hideTimeout = null;

        const handleScoreUpdate = (e) => {
            const { score: addedScore, totalScore } = e.detail;
            console.log('[GameContainer] Score update:', { addedScore, totalScore });
            setPointsGained(addedScore);
            setScore(totalScore);

            // Tự động ẩn popup sau 3 giây
            if (hideTimeout) clearTimeout(hideTimeout);
            hideTimeout = setTimeout(() => {
                setPointsGained(0);
            }, 3000);

            // Update leaderboard for current player
            setLeaderboardData(prev => {
                const updated = [...prev];
                const youIndex = updated.findIndex(p => p.id === playerId);
                if (youIndex >= 0) {
                    updated[youIndex].score = totalScore;
                }
                return updated.sort((a, b) => b.score - a.score);
            });
        };

        // Listen to Round 1, Round 2, and Round 3 score updates
        window.addEventListener('round1ScoreUpdate', handleScoreUpdate);
        window.addEventListener('round2ScoreUpdate', handleScoreUpdate);
        window.addEventListener('round3ScoreUpdate', handleScoreUpdate);
        return () => {
            window.removeEventListener('round1ScoreUpdate', handleScoreUpdate);
            window.removeEventListener('round2ScoreUpdate', handleScoreUpdate);
            window.removeEventListener('round3ScoreUpdate', handleScoreUpdate);
            if (hideTimeout) clearTimeout(hideTimeout);
        };
    }, [playerId]);

    // ============ LISTEN TO LEADERBOARD UPDATES FROM SERVER ============
    // ============ LISTEN TO LEADERBOARD UPDATES FROM SERVER ============
    useEffect(() => {
        const handleLeaderboardUpdate = (e) => {
            const { players, rankings } = e.detail;
            const dataToUse = rankings || players;

            if (dataToUse && Array.isArray(dataToUse)) {
                // Normalize and sort
                const normalized = dataToUse.map(p => ({
                    id: p.id || p.account_id,
                    name: p.name || `PLAYER${p.id || p.account_id}`,
                    score: p.score || 0,
                    isEliminated: p.eliminated || false
                }));
                setLeaderboardData(normalized.sort((a, b) => b.score - a.score));
            }
        };

        // Listen to generic updates
        window.addEventListener('leaderboardUpdate', handleLeaderboardUpdate);

        // Listen to Round-specific events that carry player lists/rankings
        // Round 1
        window.addEventListener('round1ReadyStatus', handleLeaderboardUpdate);
        window.addEventListener('round1AllFinished', handleLeaderboardUpdate);
        // Round 2
        window.addEventListener('round2ReadyStatus', handleLeaderboardUpdate);
        window.addEventListener('round2AllFinished', handleLeaderboardUpdate);
        // Round 3
        window.addEventListener('round3ReadyStatus', handleLeaderboardUpdate);
        window.addEventListener('round3AllFinished', handleLeaderboardUpdate);

        return () => {
            window.removeEventListener('leaderboardUpdate', handleLeaderboardUpdate);
            window.removeEventListener('round1ReadyStatus', handleLeaderboardUpdate);
            window.removeEventListener('round1AllFinished', handleLeaderboardUpdate);
            window.removeEventListener('round2ReadyStatus', handleLeaderboardUpdate);
            window.removeEventListener('round2AllFinished', handleLeaderboardUpdate);
            window.removeEventListener('round3ReadyStatus', handleLeaderboardUpdate);
            window.removeEventListener('round3AllFinished', handleLeaderboardUpdate);
        };
    }, []);

    // ============ BONUS ROUND STATE ============
    const [bonusData, setBonusData] = useState(null);

    // ============ END GAME STATE ============
    const [endGameData, setEndGameData] = useState(null);

    // ============ LISTEN TO BONUS ROUND TRIGGER ============
    useEffect(() => {
        const handleBonusParticipant = (e) => {
            const data = e.detail;
            console.log('[GameContainer] Bonus round triggered - I am participant:', data);
            setBonusData(data);
            setCurrentRound('bonus');
        };

        const handleBonusSpectator = (e) => {
            const data = e.detail;
            console.log('[GameContainer] Bonus round triggered - I am spectator:', data);
            setBonusData(data);
            setCurrentRound('bonus');
        };

        const handleBonusTransition = (e) => {
            const data = e.detail;
            console.log('[GameContainer] Bonus transition:', data);

            setBonusData(null); // Clear bonus data

            if (data.next_phase === 'NEXT_ROUND') {
                // Continue to next round after bonus
                setCurrentRound(data.next_round);
            } else if (data.next_phase === 'MATCH_ENDED') {
                // Match ended after bonus - store end game data
                setEndGameData({
                    finalScores: data.final_scores,
                    winner: data.winner,
                    bonusWinner: data.bonus_winner,
                    message: data.message
                });
                setCurrentRound('end');
            }
        };

        window.addEventListener('bonusParticipant', handleBonusParticipant);
        window.addEventListener('bonusSpectator', handleBonusSpectator);
        window.addEventListener('bonusTransition', handleBonusTransition);

        return () => {
            window.removeEventListener('bonusParticipant', handleBonusParticipant);
            window.removeEventListener('bonusSpectator', handleBonusSpectator);
            window.removeEventListener('bonusTransition', handleBonusTransition);
        };
    }, []);

    // ============ LISTEN TO END GAME RESULT ============
    useEffect(() => {
        const handleEndGameResult = (e) => {
            const data = e.detail;
            console.log('[GameContainer] End game result received:', data);
            setEndGameData(data);
            setCurrentRound('end');
        };

        window.addEventListener('endGameResult', handleEndGameResult);

        return () => {
            window.removeEventListener('endGameResult', handleEndGameResult);
        };
    }, []);

    // ============ HANDLE ROUND COMPLETE ============
    const handleRoundComplete = (nextRound, data) => {
        console.log(`[GameContainer] Round complete, moving to round ${nextRound}`, data);
        setCurrentRound(nextRound);

        if (data?.score !== undefined) {
            setScore(data.score);
        }
    };

    // ============ HANDLE FORFEIT ============
    const handleForfeit = () => {
        if (window.confirm("Are you sure you want to forfeit this match?")) {
            sendForfeit(matchId);
            window.history.back();
        }
    };

    // ============ HANDLE BACK TO LOBBY FROM END GAME ============
    const handleBackToLobby = () => {
        console.log('[GameContainer] Back to lobby from end game');
        setEndGameData(null);
        setBonusData(null);
        setCurrentRound(1);
        setScore(0);
        navigate('/lobby');
    };

    // ============ RENDER CURRENT ROUND ============
    const renderRound = () => {
        switch (currentRound) {
            case 1:
                return <Round1 matchId={matchId} playerId={playerId} previousScore={score} onRoundComplete={handleRoundComplete} />;
            case 2:
                return <Round2
                    matchId={matchId}
                    playerId={playerId}
                    previousScore={score}
                    onRoundComplete={handleRoundComplete}
                />;
            case 3:
                return <Round3
                    matchId={matchId}
                    playerId={playerId}
                    previousScore={score}
                    onRoundComplete={handleRoundComplete}
                />;
            case 4:
            case 'bonus':
                return <RoundBonus
                    matchId={matchId}
                    playerId={playerId}
                    initialData={bonusData}
                    onRoundComplete={handleRoundComplete}
                />;
            case 'end':
                return <EndGame
                    matchId={matchId}
                    playerId={playerId}
                    initialData={endGameData}
                    onBackToLobby={handleBackToLobby}
                />;
            default:
                return <Round1 matchId={matchId} playerId={playerId} onRoundComplete={handleRoundComplete} />;
        }
    };

    // ============ GET ROUND LABEL ============
    const getRoundLabel = () => {
        if (currentRound === 'bonus' || currentRound === 4) return 'BONUS';
        if (currentRound === 'end') return 'GAME OVER';
        return `ROUND ${currentRound}`;
    };

    // ============ STYLES ============
    const containerStyle = {
        width: '100vw',
        height: '100vh',
        backgroundImage: `url('/bg/waitingroom2.png')`,
        backgroundSize: 'cover',
        backgroundPosition: 'center',
        display: 'flex',
        flexDirection: 'column',
        fontFamily: "'Luckiest Guy', cursive, sans-serif",
        overflow: 'hidden'
    };

    // ============ RENDER ============
    return (
        <div style={containerStyle}>
            <div className="game-overlay">

                <header className="game-header">
                    <div className="header-left">
                        <div className="round-badge-new">{getRoundLabel()}</div>
                    </div>

                    <div className="header-right" style={{ display: 'flex', alignItems: 'center', gap: '20px' }}>
                        <div className="timer-circle">
                            <div className="timer-value">{timeLeft}</div>
                        </div>

                        <div className="score-board">
                            <span className="score-label">YOUR SCORE</span>
                            <span className="score-number">{score.toLocaleString()}</span>
                            {pointsGained > 0 && <div className="score-plus">+{pointsGained}</div>}
                        </div>
                    </div>
                </header>

                <div className="game-body">
                    <aside className="game-leaderboard">
                        <h3 className="panel-title">RANKING</h3>
                        <div className="leaderboard-list">
                            {leaderboardData.map((player, index) => {
                                const isOut = gameMode === 'SURVIVAL' && player.isEliminated;
                                const isMe = player.id === playerId;

                                return (
                                    <div
                                        key={player.id}
                                        className={`rank-item 
                                            ${isMe ? 'is-me' : ''} 
                                            ${isOut ? 'eliminated' : ''}`
                                        }
                                    >
                                        <span className="rank-name">
                                            {isMe ? `${player.name} (YOU)` : player.name}
                                        </span>
                                        <span className="rank-score">
                                            {isOut ? "OUT" : player.score.toLocaleString()}
                                        </span>
                                    </div>
                                );
                            })}
                        </div>

                        <button className="game-btn-exit forfeit-sidebar-btn" onClick={handleForfeit}>
                            FORFEIT
                        </button>
                    </aside>

                    <main className="game-play-panel">
                        {renderRound()}
                    </main>
                </div>
            </div>
        </div>
    );
};

export default GameContainer;