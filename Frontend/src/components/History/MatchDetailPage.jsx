import { useParams, useNavigate } from "react-router-dom";
import PageTransition from "../PageTransition";
import AppTitle from "../Lobby/AppTitle";
import UserCard from "../Lobby/UserCard"; // Thêm UserCard
import "./historyPage.css"; 
import "./MatchDetailPage.css";

const mockMatchDetail = {
  id: 1023,
  mode: "Scoring",
  myStats: { score: 120, ranking: "1st", winner: true },
  questions: [
    {
      round_no: 1,
      question_text: "What is 2 + 2?",
      answers: [
        { player_name: "You", answer: "4", score_delta: 10, is_correct: true },
        { player_name: "Opponent_A", answer: "5", score_delta: -5, is_correct: false }
      ]
    }
    // ... thêm các câu hỏi khác
  ]
};

export default function MatchDetailPage() {
  const { id } = useParams();
  const navigate = useNavigate();

  return (
    <PageTransition>
      <div className="history-page">
        {/* Đồng bộ UI với HistoryPage */}
        <UserCard />
        <div className="history-title-wrapper">
          <AppTitle title={`MATCH #${id}`} />
        </div>

        <div className="history-content">
          <div className="history-container detail-view">
            
            <div className="overall-stats">
              <div className="stat-box">
                <span className="stat-label">RANK</span>
                <span className="stat-value highlight">{mockMatchDetail.myStats.ranking}</span>
              </div>
              <div className="stat-box">
                <span className="stat-label">TOTAL SCORE</span>
                <span className={`stat-value ${mockMatchDetail.myStats.winner ? 'win-text' : 'lose-text'}`}>
                  {mockMatchDetail.myStats.score > 0 ? `+${mockMatchDetail.myStats.score}` : mockMatchDetail.myStats.score}
                </span>
              </div>
              <div className="stat-box">
                <span className="stat-label">MODE</span>
                <span className="stat-value" style={{color: '#3498db'}}>{mockMatchDetail.mode.toUpperCase()}</span>
              </div>
            </div>

            <div className="history-list-fixed">
               <h3 className="round-detail-title">ROUND DETAILS</h3>
              {mockMatchDetail.questions.map((q, idx) => (
                <div key={idx} className="detail-question-card">
                  <div className="question-header">
                    <span className="round-badge">Round {q.round_no}</span>
                    <p className="question-text">{q.question_text}</p>
                  </div>
                  <div className="answers-grid">
                    {q.answers.map((ans, i) => (
                      <div key={i} className={`answer-item ${ans.is_correct ? 'correct' : 'wrong'}`}>
                        <span className="p-name">{ans.player_name}:</span>
                        <span className="p-ans">"{ans.answer}"</span>
                        <span className={`p-delta ${ans.score_delta > 0 ? 'win-text' : 'lose-text'}`}>
                           ({ans.score_delta > 0 ? `+${ans.score_delta}` : ans.score_delta})
                        </span>
                      </div>
                    ))}
                  </div>
                </div>
              ))}
            </div>

            <div className="detail-footer">
              <button className="view-btn back-btn" onClick={() => navigate("/history")}>
                BACK TO HISTORY
              </button>
            </div>
          </div>
        </div>
      </div>
    </PageTransition>
  );
}