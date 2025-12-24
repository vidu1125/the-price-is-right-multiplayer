from sqlalchemy import (
    Column, Integer, String, Timestamp, ForeignKey, UniqueConstraint
)
from sqlalchemy.dialects.postgresql import JSONB
from .base import Base

class MatchQuestion(Base):
    __tablename__ = "match_question"

    id = Column(Integer, primary_key=True)
    match_id = Column(Integer, ForeignKey("matches.id"))

    round_no = Column(Integer)
    round_type = Column(String(20))

    question_idx = Column(Integer)
    question = Column(JSONB)

    created_at = Column(Timestamp)

    __table_args__ = (
        UniqueConstraint("match_id", "round_no", "question_idx"),
    )


class MatchAnswer(Base):
    __tablename__ = "match_answer"

    id = Column(Integer, primary_key=True)
    question_id = Column(Integer, ForeignKey("match_question.id"))
    player_id = Column(Integer, ForeignKey("match_players.id"))

    answer = Column(JSONB)
    score_delta = Column(Integer)
    action_idx = Column(Integer, default=1)

    created_at = Column(Timestamp)

    __table_args__ = (
        UniqueConstraint("question_id", "player_id", "action_idx"),
    )


class Question(Base):
    __tablename__ = "questions"

    id = Column(Integer, primary_key=True)
    type = Column(String(20))
    data = Column(JSONB)
    active = Column(Boolean, default=True)

    created_at = Column(Timestamp)
    updated_at = Column(Timestamp)
