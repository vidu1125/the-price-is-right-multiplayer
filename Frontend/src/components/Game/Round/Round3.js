import React, { useState, useRef } from 'react';
import './Round3.css';

const Round3 = ({ totalPlayers = 1 }) => {
    const [turn, setTurn] = useState(1);
    const [bonusScore, setBonusScore] = useState(0);
    const [isSpinning, setIsSpinning] = useState(false);
    const [rotation, setRotation] = useState(0);

    // Danh sách các giá trị trên vòng quay
    const wheelValues = ["100", "200", "500", "0", "1000", "LOSE"];

    const handleSpin = () => {
        if (isSpinning || turn > 2) return;

        setIsSpinning(true);
        // Quay ít nhất 5 vòng (1800deg) + góc ngẫu nhiên
        const newRotation = rotation + 1800 + Math.floor(Math.random() * 360);
        setRotation(newRotation);

        setTimeout(() => {
            setIsSpinning(false);
            // Giả lập tính toán điểm (sau này bạn có thể thay bằng logic tính dựa trên góc quay)
            const gainedPoints = Math.floor(Math.random() * 10) * 100;
            setBonusScore(prev => prev + gainedPoints);
            
            if (turn < 2) setTurn(prev => prev + 1);
        }, 4000);
    };

    return (
        <div className="round3-wrapper">
            <div className="quiz-content">
                <div className="question-header-row">
                    <div className="turn-indicator">TURN {turn} / 2</div>
                    <div className="bonus-score-badge">💰 BONUS: {bonusScore}</div>
                </div>

                <div className="wheel-section">
                    <div className="wheel-container">
                        <div className="wheel-pointer"></div>
                        <div 
                            className="lucky-wheel" 
                            style={{ transform: `rotate(${rotation}deg)` }}
                        >
                            {/* PHẦN THAY THẾ NẰM Ở ĐÂY */}
                            {wheelValues.map((value, index) => {
                                // 360 / 6 = 60 độ mỗi phân đoạn
                                const segmentAngle = 360 / wheelValues.length;
                                // Góc xoay = (chỉ số * 60) + 30 (để vào giữa phân đoạn)
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
                        disabled={isSpinning || turn > 2}
                    >
                        SPIN
                    </button>
                </div>
            </div>
        </div>
    );
};

export default Round3;