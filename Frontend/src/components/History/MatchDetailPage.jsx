import { useParams, useNavigate } from "react-router-dom";
import { useEffect, useState } from "react";
import { viewMatchDetails } from "../../services/historyService";
import PageTransition from "../PageTransition";
import AppTitle from "../Lobby/AppTitle";
import UserCard from "../Lobby/UserCard";
import "./historyPage.css"; // Dùng chung các biến màu/font
import "./MatchDetailPage.css";

const mockMatchDetail = {
  id: 1023,
  mode: "Scoring",
  playerCount: 4,
  myStats: {
    score: 120,
    ranking: "1st",
    winner: true
  },

  questions: [
    // ======================
    // ROUND 1 – QUIZ
    // ======================
    {
      round_no: 1,
      question_idx: 1,
      question_text: "What is this object?",
      question_image: "https://via.placeholder.com/300x150",
      answers: [
        { player_name: "You", answer: "Apple", score_delta: 10, is_correct: true },
        { player_name: "Player 2", answer: "Banana", score_delta: -5, is_correct: false },
        { player_name: "Player 3", answer: "Apple", score_delta: 10, is_correct: true },
        { player_name: "Player 4", answer: "Orange", score_delta: -5, is_correct: false }
      ]
    },
    {
      round_no: 1,
      question_idx: 2,
      question_text: "Which brand made this product?",
      question_image: "https://via.placeholder.com/300x150",
      answers: [
        { player_name: "You", answer: "Nike", score_delta: 10, is_correct: true },
        { player_name: "Player 2", answer: "Adidas", score_delta: -5, is_correct: false },
        { player_name: "Player 3", answer: "Puma", score_delta: -5, is_correct: false },
        { player_name: "Player 4", answer: "Nike", score_delta: 10, is_correct: true }
      ]
    },

    // ======================
    // ROUND 2 – BIDDING
    // ======================
    {
      round_no: 2,
      question_idx: 1,
      question_text: "What is the correct price of this item?",
      question_image: "https://via.placeholder.com/300x150",
      answers: [
        { player_name: "You", answer: "$999", score_delta: 20, is_correct: true },
        { player_name: "Player 2", answer: "$1200", score_delta: -10, is_correct: false },
        { player_name: "Player 3", answer: "$950", score_delta: 15, is_correct: false },
        { player_name: "Player 4", answer: "$1100", score_delta: -10, is_correct: false }
      ]
    },
    {
      round_no: 2,
      question_idx: 2,
      question_text: "How much does this product cost?",
      question_image: "https://via.placeholder.com/300x150",
      answers: [
        { player_name: "You", answer: "$450", score_delta: 15, is_correct: true },
        { player_name: "Player 2", answer: "$500", score_delta: -10, is_correct: false },
        { player_name: "Player 3", answer: "$430", score_delta: 10, is_correct: false },
        { player_name: "Player 4", answer: "$600", score_delta: -10, is_correct: false }
      ]
    },

    // ======================
    // ROUND 3 – BONUS WHEEL
    // ======================
    {
      round_no: 3,
      question_idx: 1,
      question_text: "Spin the bonus wheel – first spin result",
      question_image: null,
      answers: [
        { player_name: "You", answer: "x2 Multiplier", score_delta: 30, is_correct: true },
        { player_name: "Player 2", answer: "No Bonus", score_delta: 0, is_correct: false },
        { player_name: "Player 3", answer: "x1 Multiplier", score_delta: 10, is_correct: true },
        { player_name: "Player 4", answer: "Lose Turn", score_delta: -10, is_correct: false }
      ]
    },
    {
      round_no: 3,
      question_idx: 2,
      question_text: "Spin the bonus wheel – second spin result",
      question_image: null,
      answers: [
        { player_name: "You", answer: "+50 Points", score_delta: 50, is_correct: true },
        { player_name: "Player 2", answer: "+10 Points", score_delta: 10, is_correct: true },
        { player_name: "Player 3", answer: "Lose 20 Points", score_delta: -20, is_correct: false },
        { player_name: "Player 4", answer: "No Bonus", score_delta: 0, is_correct: false }
      ]
    }
  ]
};
export default function MatchDetailPage() {
  const { id } = useParams();
  const navigate = useNavigate();
  const [matchDetail, setMatchDetail] = useState(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    viewMatchDetails(id)
      .then(data => {
        setMatchDetail(data.questions ? data : mockMatchDetail);
        setLoading(false);
      })
      .catch(() => {
        setMatchDetail(mockMatchDetail);
        setLoading(false);
      });
  }, [id]);

  if (loading) return <div className="history-page">Loading...</div>;

  // Nhóm các câu hỏi theo round_no
  const groupedRounds = matchDetail.questions.reduce((acc, q) => {
    if (!acc[q.round_no]) acc[q.round_no] = [];
    acc[q.round_no].push(q);
    return acc;
  }, {});

  return (
    <PageTransition>
      <div className="history-page">
        <UserCard />
        <div className="history-title-wrapper"><AppTitle title={`MATCH #${id}`} /></div>

        <div className="history-content">
          <div className="history-container detail-view">

            {/* THÔNG TIN CHUNG NẰM NGANG */}
            <div className="overall-stats">
              <div className="stat-box">
                <span className="stat-label">RANK</span>
                <span className="stat-value highlight">{matchDetail.myStats.ranking}</span>
              </div>
              <div className="stat-box">
                <span className="stat-label">TOTAL SCORE</span>
                <span className={`stat-value ${matchDetail.myStats.score >= 0 ? 'win-text' : 'lose-text'}`}>
                  {matchDetail.myStats.score > 0 ? `+${matchDetail.myStats.score}` : matchDetail.myStats.score}
                </span>
              </div>
              <div className="stat-box">
                <span className="stat-label">MODE</span>
                <span className="stat-value mode-text">{matchDetail.mode.toUpperCase()}</span>
              </div>
              <div className="stat-box">
                <span className="stat-label">PLAYERS</span>
                <span className="stat-value">{matchDetail.playerCount}</span>
              </div>
            </div>

            {/* DANH SÁCH ROUND - CÓ SCROLL */}
            <div className="history-list-fixed detail-scroll">
              {Object.keys(groupedRounds).map((roundNum) => (
                <div key={roundNum} className="round-group-container">
                  <div className="round-header-banner">ROUND {roundNum}</div>

                  {groupedRounds[roundNum].map((q, idx) => (
                    <div key={idx} className="question-item-block">
                      <div className="question-header-info">
                        <span className="q-index">Q{q.question_idx}:</span>
                        <span className="q-text">{q.question_text}</span>
                      </div>

                      {q.question_image && (
                        <div className="q-img-wrapper">
                          <img src={q.question_image} alt="Question" />
                        </div>
                      )}

                      <div className="answers-stack">
                        {q.answers.map((ans, i) => (
                          <div key={i} className="player-row">
                            <span className="p-name">{ans.player_name}:</span>
                            <span className={`p-ans ${ans.is_correct ? 'text-correct' : 'text-wrong'}`}>
                              {ans.answer}
                            </span>
                            <span className={`p-pts ${ans.score_delta >= 0 ? 'win-text' : 'lose-text'}`}>
                              {ans.score_delta > 0 ? `+${ans.score_delta}` : ans.score_delta}
                            </span>
                          </div>
                        ))}
                      </div>
                    </div>
                  ))}
                </div>
              ))}
            </div>

            <div className="detail-footer">
              <button className="view-btn back-btn" onClick={() => navigate("/history")}>BACK TO HISTORY</button>
            </div>
          </div>
        </div>
      </div>
    </PageTransition>
  );
}