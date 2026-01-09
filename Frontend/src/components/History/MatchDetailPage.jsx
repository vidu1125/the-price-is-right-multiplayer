import { useParams, useNavigate, useLocation } from "react-router-dom";
import { useEffect, useState } from "react";
import { viewMatchDetails } from "../../services/historyService";
import PageTransition from "../PageTransition";
import AppTitle from "../Lobby/AppTitle";
import UserCard from "../Lobby/UserCard";
import "./historyPage.css"; // Dùng chung các biến màu/font
import "./MatchDetailPage.css";

const mockMatchDetail = null;

export default function MatchDetailPage() {
  const { id } = useParams();
  const navigate = useNavigate();
  const location = useLocation();
  const passedMatch = location.state?.match;

  // Initialize with passed data if available, or null/loading state
  const [matchDetail, setMatchDetail] = useState(passedMatch ? {
    id: passedMatch.matchId,
    mode: passedMatch.mode,
    playerCount: 0, // Placeholder until fetch
    myStats: {
      score: passedMatch.score,
      ranking: passedMatch.ranking,
      winner: passedMatch.isWinner
    },
    questions: []
  } : null);

  const [loading, setLoading] = useState(true);

  useEffect(() => {
    // Allows instant display of basic info (from passedMatch) while fetching details
    if (passedMatch) {
      // We still want to fetch the questions!
      // But we can show what we have.
    }

    viewMatchDetails(id)
      .then(data => {
        console.log("Match Details Fetched:", data);
        setMatchDetail(prev => {
          // If we have existing state (from passedMatch), merge correctly
          // If not, we might lack myStats. 
          // TODO: If direct access, we need to handle myStats (maybe parse from match_players if we knew our name/ID)

          const base = prev || { ...data, myStats: { ranking: "?", score: 0, winner: false } };

          return {
            ...base,
            ...data, // Overwrite with server data (questions, playerCount, real mode string)
            // Ensure myStats is preserved if it existed
            myStats: base.myStats
          };
        });
        setLoading(false);
      })
      .catch((err) => {
        console.error("Failed to fetch match details", err);
        setLoading(false);
      });
  }, [id, passedMatch]);

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
                      {q.round_type !== 'WHEEL' && (
                        <>
                          <div className="question-header-info">
                            <span className="q-index">Q{q.question_idx}:</span>
                            <span className="q-text">{q.question_text}</span>
                          </div>

                          {q.question_image && (
                            <div className="q-img-wrapper">
                              <img src={q.question_image} alt="Question" />
                            </div>
                          )}
                        </>
                      )}

                      <div className="answers-stack">
                        {q.answers.map((ans, i) => (
                          <div key={i} className={`player-row ${ans.status ? 'has-status' : ''}`}>
                            <span className="p-name">{ans.player_name}:</span>

                            <span className={`p-ans ${ans.is_correct ? 'text-correct' : 'text-wrong'}`}>
                              {ans.status ? (
                                <span className={`status-badge ${ans.status}`}>
                                  {ans.status === 'eliminated' ? 'ELIMINATED' : 'FORFEIT'}
                                </span>
                              ) : (
                                ans.answer || "-"
                              )}
                            </span>

                            <span className={`p-pts ${ans.score_delta >= 0 ? 'win-text' : 'lose-text'}`}>
                              {ans.status ? "—" : (ans.score_delta !== undefined ? (ans.score_delta > 0 ? `+${ans.score_delta}` : ans.score_delta) : "—")}
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