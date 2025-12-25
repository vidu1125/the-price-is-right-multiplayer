from app.models import db
from .base import Base
from sqlalchemy.sql import func

class MatchQuestion(Base):
    __tablename__ = "match_question"

    id = db.Column(db.Integer, primary_key=True)

    match_id = db.Column(
        db.Integer,
        db.ForeignKey("matches.id"),
        nullable=False
    )

    round_no = db.Column(db.Integer, nullable=False)
    round_type = db.Column(db.String(20), nullable=False)

    question_idx = db.Column(db.Integer, nullable=False)

    question = db.Column(db.JSON)   # ✅ KHÔNG dùng JSONB

    created_at = db.Column(db.DateTime, server_default=func.now())

    __table_args__ = (
        db.UniqueConstraint(
            "match_id",
            "round_no",
            "question_idx",
            name="uq_match_round_question"
        ),
    )


class MatchAnswer(Base):
    __tablename__ = "match_answer"

    id = db.Column(db.Integer, primary_key=True)

    question_id = db.Column(
        db.Integer,
        db.ForeignKey("match_question.id"),
        nullable=False
    )

    player_id = db.Column(
        db.Integer,
        db.ForeignKey("match_players.id"),
        nullable=False
    )

    answer = db.Column(db.JSON)
    score_delta = db.Column(db.Integer)

    action_idx = db.Column(db.Integer, default=1)

    created_at = db.Column(db.DateTime, server_default=func.now())

    __table_args__ = (
        db.UniqueConstraint(
            "question_id",
            "player_id",
            "action_idx",
            name="uq_answer_action"
        ),
    )


class Question(Base):
    __tablename__ = "questions"

    id = db.Column(db.Integer, primary_key=True)

    type = db.Column(db.String(20), nullable=False)
    data = db.Column(db.JSON)

    active = db.Column(db.Boolean, default=True)

    created_at = db.Column(db.DateTime, server_default=func.now())
    updated_at = db.Column(db.DateTime, onupdate=func.now())
