import React, { useState } from 'react';
import './GameContainer.css';
import Round1 from './Round/Round1'; 
import Round2 from './Round/Round2'; 
import Round3 from './Round/Round3';

const GameContainer = () => {
    const [score, setScore] = useState(1250);
    const [pointsGained, setPointsGained] = useState(150);

    const mockDataFromBackend = {
        questionText: "What is the price of iPhone 15?",
        productImage: "/bg/iphone15.jpg",
        currentQuestion: 1,
        totalQuestions: 15
    };
    
    const [gameMode, setGameMode] = useState('SURVIVAL'); 

    const leaderboardData = [
        { name: "ALEX_MASTER", score: 2100, isEliminated: true },
        { name: "PLAYER_2", score: 1850, isEliminated: true }, 
        { name: "YOU", score: 1250, isEliminated: false },
        { name: "CHRIS_99", score: 950, isEliminated: false },
        { name: "CHRIS_99", score: 950, isEliminated: false },
        { name: "CHRIS_99", score: 950, isEliminated: false }
    ];

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

    return (
        <div style={containerStyle}>
            <div className="game-overlay">
                
                <header className="game-header">
                    <div className="header-left">
                        <div className="round-badge-new">ROUND 1</div>
                    </div>

                    {/* Header-right giờ sẽ chứa cả Timer và Score */}
                    <div className="header-right" style={{ display: 'flex', alignItems: 'center', gap: '20px' }}>
                        <div className="timer-circle">
                            <div className="timer-value">30</div>
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

                                return (
                                    <div 
                                        key={index} 
                                        className={`rank-item 
                                            ${player.name === 'YOU' ? 'is-me' : ''} 
                                            ${isOut ? 'eliminated' : ''}`
                                        }
                                    >
                                        <span className="rank-name">{player.name}</span>
                                        <span className="rank-score">
                                            {isOut ? "OUT" : player.score.toLocaleString()}
                                        </span>
                                    </div>
                                );
                            })}
                        </div>

                        {/* Nút FORFEIT nằm dưới bảng Ranking */}
                        <button className="game-btn-exit forfeit-sidebar-btn" onClick={() => window.history.back()}>
                            FORFEIT
                        </button>
                    </aside>

                    <main className="game-play-panel">
                       {/* <Round1 
                            currentQuestion={mockDataFromBackend.currentQuestion}
                            totalQuestions={mockDataFromBackend.totalQuestions}
                            productImage={mockDataFromBackend.productImage} // Thêm dòng này
                        /> */}
                        {/* <Round2
                            currentQuestion={mockDataFromBackend.currentQuestion}
                            totalQuestions={mockDataFromBackend.totalQuestions}
                            productImage={mockDataFromBackend.productImage}
                            isWagerAvailable={true}
                        /> */}
                         <Round3 totalPlayers={1} />
                    </main>
                </div>
            </div>
        </div>
    );
};

export default GameContainer;