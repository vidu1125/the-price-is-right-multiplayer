import React, { useState } from 'react';
import './RoundBonus.css';

const RoundBonus = ({ productImage }) => {
    const [selectedAnswer, setSelectedAnswer] = useState(null);

    const handleActionClick = () => {
        console.log("Tranh quyền trả lời!");
    };

    return (
        <div className="round-bonus-wrapper">
            <div className="quiz-content">
                <div className="question-header-row">
                    <div className="bonus-label">BONUS QUESTION</div>
                </div>

                <h2 className="quiz-question">Is the price higher or lower than $1,500?</h2>

                <div className="product-image-container">
                    <img src={productImage} alt="Product" className="product-image" />
                </div>

                <div className="bonus-interactive-row">
                    {/* Nút Arrow Up thuần túy */}
                    <button 
                        className={`arrow-btn up ${selectedAnswer === 'higher' ? 'selected' : ''}`}
                        onClick={() => setSelectedAnswer('higher')}
                    >
                        <svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                            <path d="M12 4L4 12H9V20H15V12H20L12 4Z" fill="currentColor"/>
                        </svg>
                    </button>

                    {/* Nút Buzzer ở giữa */}
                    <button className="buzzer-btn" onClick={handleActionClick}>
                        <div className="buzzer-red-circle">
                            <span className="star-icon">★</span>
                        </div>
                    </button>

                    {/* Nút Arrow Down thuần túy */}
                    <button 
                        className={`arrow-btn down ${selectedAnswer === 'lower' ? 'selected' : ''}`}
                        onClick={() => setSelectedAnswer('lower')}
                    >
                        <svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                            <path d="M12 20L20 12H15V4H9V12H4L12 20Z" fill="currentColor"/>
                        </svg>
                    </button>
                </div>
            </div>
        </div>
    );
};

export default RoundBonus;