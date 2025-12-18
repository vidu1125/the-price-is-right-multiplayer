import React, { useState } from 'react';
import './Round2.css';

const Round2 = ({ currentQuestion, totalQuestions, productImage, isWagerAvailable }) => {
    const [userAnswer, setUserAnswer] = useState('');
    // Thêm state để quản lý trạng thái On/Off của Wager
    const [isWagerActive, setIsWagerActive] = useState(false); 

    const longQuestion = "How much does the high-end version of this product cost in the current market?";

    const handleSubmit = () => {
        console.log("Submitted Answer:", userAnswer);
        console.log("Wager Active:", isWagerActive);
    };

    const handleWagerToggle = () => {
        // Đảo ngược trạng thái On/Off
        setIsWagerActive(!isWagerActive);
    };

    return (
        <div className="round2-wrapper-quiz">
            <div className="quiz-content">
                <div className="question-header-row">
                    <div className="question-number-header">
                        QUESTION {currentQuestion} / {totalQuestions}
                    </div>
                    {isWagerAvailable && (
                        /* Thêm class 'active' nếu isWagerActive là true */
                        <button 
                        className={`wager-badge-btn ${isWagerActive ? 'active' : ''}`} 
                        onClick={handleWagerToggle}
                        >
                        ⭐ WAGER
                        </button>
                    )}
                </div>

                <h2 className="quiz-question">{longQuestion}</h2>

                <div className="product-image-container">
                    <img src={productImage} alt="Product" className="product-image" />
                </div>

                <div className="input-section">
                    <input 
                        type="text" 
                        className="answer-input" 
                        placeholder="Type your answer here..."
                        value={userAnswer}
                        onChange={(e) => setUserAnswer(e.target.value)}
                    />
                    <button className="submit-answer-btn" onClick={handleSubmit}>
                        SUBMIT
                    </button>
                </div>
            </div>
        </div>
    );
};

export default Round2;