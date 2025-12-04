from app.models import db


class FPRound(db.Model):
    __tablename__ = "fp_round"

    id = db.Column(db.Integer, primary_key=True)
    round_id = db.Column(db.Integer, db.ForeignKey("rounds.id", ondelete="CASCADE"))
    text = db.Column(db.Text)
    correct = db.Column(db.Integer)
    wrong = db.Column(db.Integer)

    def to_dict(self):
        return {
            "id": self.id,
            "round_id": self.round_id,
            "text": self.text,
            "correct": self.correct,
            "wrong": self.wrong,
        }
class FPAnswer(db.Model):
    __tablename__ = "fp_answers"

    id = db.Column(db.Integer, primary_key=True)
    round_id = db.Column(db.Integer, db.ForeignKey("rounds.id", ondelete="CASCADE"))
    player_id = db.Column(db.Integer, db.ForeignKey("match_players.id", ondelete="CASCADE"))
    choice = db.Column(db.Integer)
    correct = db.Column(db.Boolean)
    score = db.Column(db.Integer)

    def to_dict(self):
        return {
            "id": self.id,
            "round_id": self.round_id,
            "player_id": self.player_id,
            "choice": self.choice,
            "correct": self.correct,
            "score": self.score
        }
