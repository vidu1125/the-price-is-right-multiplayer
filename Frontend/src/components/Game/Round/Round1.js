import React from 'react';
import './Round1.css'; 

const Round1 = ({ currentQuestion, totalQuestions, productImage }) => {
    const answers = ["$799", "$899", "$999", "$1,099"];
    const longQuestion = "What is the official starting retail price for the iPhone 15 Pro Max with 256GB storage in the US market?";
    const correctAnswerIndex = 3;
    return (
       <div className="round1-wrapper-quiz">
            <div className="quiz-content">
                <div className="question-number-header">
                    QUESTION {currentQuestion} / {totalQuestions}
                </div>

                <h2 className="quiz-question">{longQuestion}</h2>

                <div className="product-image-container">
                    <img src={productImage} alt="Product" className="product-image" />
                </div>

                <div className="answers-grid">
                    {answers.map((text, index) => {
                        // Kiểm tra xem đây là đáp án đúng hay sai
                        const isCorrect = index === correctAnswerIndex;
                        return (
                            <button 
                                key={index} 
                                className={`quiz-answer-btn ${isCorrect ? 'correct' : 'wrong'}`}
                            >
                                {text}
                            </button>
                        );
                    })}
                </div>
            </div>
        </div>
    );
};

export default Round1;