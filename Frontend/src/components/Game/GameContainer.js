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

const GameContainer = () => {
    // ============ STATE MANAGEMENT ============
    const [currentRound, setCurrentRound] = useState(1);
    const [score, setScore] = useState(0);
    const [pointsGained, setPointsGained] = useState(0);
    const [gameMode, setGameMode] = useState('SCORING');
    const [timeLeft, setTimeLeft] = useState(15);
    
    // ============ GET MATCH ID & PLAYER ID FROM URL ============
    const urlParams = new URLSearchParams(window.location.search);
    const matchId = parseInt(urlParams.get('match_id') || '1');
    const playerId = parseInt(urlParams.get('player_id') || '1');
    const playerName = urlParams.get('name') || `PLAYER${playerId}`;
    
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

    // ============ LISTEN TO SCORE UPDATES ============
    useEffect(() => {
        let hideTimeout = null;
        
        const handleScoreUpdate = (e) => {
            const { score: addedScore, totalScore } = e.detail;
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

        window.addEventListener('round1ScoreUpdate', handleScoreUpdate);
        return () => {
            window.removeEventListener('round1ScoreUpdate', handleScoreUpdate);
            if (hideTimeout) clearTimeout(hideTimeout);
        };
    }, [playerId]);

    // ============ LISTEN TO LEADERBOARD UPDATES FROM SERVER ============
    useEffect(() => {
        const handleLeaderboardUpdate = (e) => {
            const { players } = e.detail;
            if (players && Array.isArray(players)) {
                setLeaderboardData(players.sort((a, b) => b.score - a.score));
            }
        };

        window.addEventListener('leaderboardUpdate', handleLeaderboardUpdate);
        return () => window.removeEventListener('leaderboardUpdate', handleLeaderboardUpdate);
    }, []);

    // ============ HANDLE ROUND COMPLETE ============
    const handleRoundComplete = (nextRound, data) => {
        console.log(`[GameContainer] Round complete, moving to round ${nextRound}`, data);
        setCurrentRound(nextRound);
        
        if (data?.score !== undefined) {
            setScore(data.score);
        }
    };

    // ============ RENDER CURRENT ROUND ============
    const renderRound = () => {
        switch (currentRound) {
            case 1:
                return <Round1 matchId={matchId} playerId={playerId} onRoundComplete={handleRoundComplete} />;
            case 2:
                return <Round2 
                    matchId={matchId} 
                    playerId={playerId}
                    onRoundComplete={handleRoundComplete}
                    currentQuestion={1}
                    totalQuestions={5}
                    productImage="/bg/iphone15.jpg"
                    isWagerAvailable={true}
                />;
            case 3:
                return <Round3 totalPlayers={4} onRoundComplete={handleRoundComplete} />;
            case 4:
            case 'bonus':
                return <RoundBonus 
                    currentQuestion={1} 
                    totalQuestions={1} 
                    productImage="/bg/iphone15.jpg" 
                />;
            default:
                return <Round1 matchId={matchId} playerId={playerId} onRoundComplete={handleRoundComplete} />;
        }
    };

    // ============ GET ROUND LABEL ============
    const getRoundLabel = () => {
        if (currentRound === 'bonus' || currentRound === 4) return 'BONUS';
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

                        <button className="game-btn-exit forfeit-sidebar-btn" onClick={() => window.history.back()}>
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